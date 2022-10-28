#include "vfp_vector.h"

#include <stdbool.h>

#if defined(NO_VFP_GEN)
int GenerateVFPEmulation(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext)
{
    return 0;
}
#else
static uint32_t vfpEncodedInstr[VFP_OP_Count] =
        {
                0xEEB00A40, // VMOV
                0xEEB00A00, // VMOV_IMM
                0xEE300A00, // VADD
                0xEE300A40, // VSUB
                0xEE800A00, // VDIV
                0xEE200A00, // VMUL
                0xEE200A40, // VNMUL
                0xEE000A00, // VMLA
                0xEE100A40, // VNMLA
                0xEE000A40, // VMLS
                0xEE100A00, // VNMLS
                0xEEB00AC0, // VABS
                0xEEB10A40, // VNEG
                0xEEB10AC0, // VSQRT
        };

static void EncodeF32VFPInstr(VFPInstruction *vfpInstr, uint32_t *writeAddr, uint32_t dReg, uint32_t nReg, uint32_t mReg)
{
    uint32_t instr = vfpEncodedInstr[vfpInstr->op];

    uint32_t inputFlags = vfpOpInputFlags[vfpInstr->op];

    instr |= ((dReg & 0x1E) << 11) | ((dReg & 0x1) << 22);

    if ((inputFlags & VFP_OP_INPUT_IMM) == 0)
    {
        if (inputFlags & VFP_OP_INPUT_Sn)
            instr |= ((nReg & 0x1E) << 15) | ((nReg & 0x1) << 7);

        instr |= ((mReg & 0x1E) >> 1) | ((mReg & 0x1) << 5);
    }
    else
    {
        union
        {
            float f32;
            uint32_t u32;
        } rawFloat;

        rawFloat.f32 = vfpInstr->operands.imm.f32;
        instr |= ((rawFloat.u32 & 0x80000000) >> 12) |                // a
                 (((rawFloat.u32 & 0x40000000) >> 12) ^ 0x00040000) | // b
                 ((rawFloat.u32 & 0x01800000) >> 7) |                 // cd
                 ((rawFloat.u32 & 0x00780000) >> 19);                 // efgh
    }

    writeAddr[0] = instr;
}

static void EncodeF64VFPInstr(VFPInstruction *vfpInstr, uint32_t *writeAddr, uint32_t dReg, uint32_t nReg, uint32_t mReg)
{
    uint32_t instr = vfpEncodedInstr[vfpInstr->op];

    uint32_t inputFlags = vfpOpInputFlags[vfpInstr->op];

    instr |= ((dReg & 0x10) << 18) | ((dReg & 0xF) << 12) | 0x100; // Destination register and sz bit

    if ((inputFlags & VFP_OP_INPUT_IMM) == 0)
    {
        if (inputFlags & VFP_OP_INPUT_Sn)
            instr |= ((vfpInstr->operands.regs.n & 0x10) << 3) | ((vfpInstr->operands.regs.n & 0xF) << 16);

        instr |= ((vfpInstr->operands.regs.m & 0x10) << 1) | (vfpInstr->operands.regs.m & 0xF);
    }
    else
    {
        union
        {
            double f64;
            uint64_t u64;
        } rawFloat;

        rawFloat.f64 = vfpInstr->operands.imm.f64;
        instr |= ((rawFloat.u64 >> 63) << 19) |                 // a
                 ((((rawFloat.u64 >> 52) & 0x4) ^ 0x4) << 16) | // b
                 (((rawFloat.u64 >> 52) & 0x3) << 16) |         // cd
                 ((rawFloat.u64 >> 48) & 0xF);                  // efgh
    }

    writeAddr[0] = instr;
}

#define BRANCH(dst, pc, cond) (0x0A000000 | (cond << 28) | (((((intptr_t)dst - (intptr_t)pc) >> 2) - 2) & 0xFFFFFF))

/**
 * push {r4, r5}
 * vmrs r4, FPSCR
 * mov r5, r4
 * bic r4, #0x370000
 * vmsr FPSCR, r4
 */
uint32_t vfpPatchPrologue[] = {
        0xE92D0030,
        0XEEF14A10,
        0xE1A05004,
        0xE3C44837,
        0xEEE14A10
};

/**
 * vmsr FPSCR, r5
 * pop {r4, r5}
 * ldr pc, [pc, #-0x4]
 * .word 0x0
 */
uint32_t vfpPatchEpilogue[] = {
        0xEEE15A10,
        0xE8BD0030,
        0xE51FF004,
        0x00000000
};

static void GenerateF32VFPInstr(VFPInstruction *vfpInstr, uint32_t **instrBuffer)
{
    int dReg = vfpInstr->dReg, nReg = vfpInstr->operands.regs.n, mReg = vfpInstr->operands.regs.m;
    int dRegBank = vfpInstr->dReg & 0x18, nRegBank = nReg & 0x18, mRegBank = mReg & 0x18;
    int mStride = mRegBank == 0 ? 0 : vfpInstr->vectorStride; // m may be a scalar register
    uint32_t *writeAddr = *instrBuffer;
    for (int i = 0; i < vfpInstr->vectorLength; i++)
    {
        EncodeF32VFPInstr(vfpInstr, writeAddr++, dReg, nReg, mReg);

        dReg = ((dReg + vfpInstr->vectorStride) & 0x7) | dRegBank; // Ensure they wrap around in the regbank (not sure if this is correct).
        nReg = ((nReg + vfpInstr->vectorStride) & 0x7) | nRegBank;
        mReg = ((mReg + mStride) & 0x7) | mRegBank;
    }
    *instrBuffer = writeAddr;
}

static void GenerateF64VFPInstr(VFPInstruction *vfpInstr, uint32_t **instrBuffer)
{
    int dReg = vfpInstr->dReg, nReg = vfpInstr->operands.regs.n, mReg = vfpInstr->operands.regs.m;
    int dRegBank = dReg & 0x1C, nRegBank = nReg & 0x1C, mRegBank = mReg & 0x1C;
    int mStride = mRegBank == 0 ? 0 : vfpInstr->vectorStride; // m may be a scalar register
    uint32_t *writeAddr = *instrBuffer;
    for (int i = 0; i < vfpInstr->vectorLength; i++)
    {
        EncodeF64VFPInstr(vfpInstr, writeAddr++, dReg, nReg, mReg);

        dReg = ((dReg + vfpInstr->vectorStride) & 0x3) | dRegBank; // Ensure they wrap around in the regbank (not sure if this is correct).
        nReg = ((nReg + vfpInstr->vectorStride) & 0x3) | nRegBank;
        mReg = ((mReg + mStride) & 0x3) | mRegBank;
    }

    *instrBuffer = writeAddr;
}

typedef struct so_module so_module;
void *so_alloc_arena(so_module *mod, uintptr_t range, uintptr_t dst, size_t sz); // Defined in so_util

so_module *so_find_module_by_addr(uintptr_t addr); // Pls implement me

int GenerateVFPEmulation(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext)
{
    size_t patchSize = sizeof(vfpPatchPrologue) + sizeof(vfpPatchEpilogue) + (sizeof(uint32_t) * vfpInstr->vectorLength);
    void *patchBase;
    uint32_t patchBuffer[32];
    uint32_t *instrBuffer = &patchBuffer[0];

    if (abortContext->SPSR & 0x20)
    {
        LOG("Thumb is not supported");
        return 0;
    }

    so_module *mod = so_find_module_by_addr(abortContext->pc);
    if (mod == NULL)
    {
        LOG("No SO module found for PC");
        return 0;
    }

    patchBase = so_alloc_arena(mod, 0x1FFFFFF, abortContext->pc, patchSize);
    if (patchBase == NULL)
        return 0;

    sceClibMemcpy(&instrBuffer[0], &vfpPatchPrologue, sizeof(vfpPatchPrologue));
    instrBuffer += (sizeof(vfpPatchPrologue) / sizeof(uint32_t));

    if (vfpInstr->precision == VFP_OP_F32)
        GenerateF32VFPInstr(vfpInstr, &instrBuffer);
    else
        GenerateF64VFPInstr(vfpInstr, &instrBuffer);

    sceClibMemcpy(&instrBuffer[0], &vfpPatchEpilogue, sizeof(vfpPatchEpilogue));
    instrBuffer += (sizeof(vfpPatchEpilogue) / sizeof(uint32_t)) - 1;

    *instrBuffer++ = abortContext->pc + 4;

    uint32_t branch = BRANCH(patchBase, abortContext->pc, vfpInstr->cond);

    kuKernelCpuUnrestrictedMemcpy(patchBase, &patchBuffer[0], patchSize);
    kuKernelFlushCaches(patchBase, patchSize);
    kuKernelCpuUnrestrictedMemcpy((uint32_t *)abortContext->pc, &branch, sizeof(branch));
    kuKernelFlushCaches((uint32_t *)abortContext->pc, sizeof(branch));

    return 1;
}

#endif