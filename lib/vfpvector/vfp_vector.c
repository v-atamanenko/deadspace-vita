#include "vfp_vector.h"

#include <stdbool.h>
#include <psp2/kernel/clib.h>

KuKernelAbortHandler nextHandler;

// All possible VFP vector operations

#if 1
static char *vfpOpNames[] =
    {
        "vmov",
        "vmov",
        "vadd",
        "vsub",
        "vdiv",
        "vmul",
        "vnmul",
        "vmla",
        "vnmla",
        "vmls",
        "vnmls",
        "vabs",
        "vneg",
        "vsqrt"};

static void PrintVFPInstr(VFPInstruction *vfpInstr)
{
    char *opName = vfpOpNames[vfpInstr->op];

    uint32_t inputFlags = vfpOpInputFlags[vfpInstr->op];
    if (vfpInstr->precision == VFP_OP_F32)
    {
        if (inputFlags & VFP_OP_INPUT_IMM)
            LOG("Emulating %s.f32 s%d, %f", opName, vfpInstr->dReg, vfpInstr->operands.imm.f32);
        else
        {
            if ((inputFlags & (VFP_OP_INPUT_Sm | VFP_OP_INPUT_Sn)) == (VFP_OP_INPUT_Sm | VFP_OP_INPUT_Sn))
                LOG("Emulating %s.f32 s%d, s%d, s%d", opName, vfpInstr->dReg, vfpInstr->operands.regs.n, vfpInstr->operands.regs.m);
            else // VFP_OP_INPUT_Sm
                LOG("Emulating %s.f32 s%d, s%d", opName, vfpInstr->dReg, vfpInstr->operands.regs.m);
        }
    }
    else
    {
        if (inputFlags & VFP_OP_INPUT_IMM)
            LOG("Emulating %s.f64 d%d, %f", opName, vfpInstr->dReg, vfpInstr->operands.imm.f64);
        else
        {
            if ((inputFlags & (VFP_OP_INPUT_Sm | VFP_OP_INPUT_Sn)) == (VFP_OP_INPUT_Sm | VFP_OP_INPUT_Sn))
                LOG("Emulating %s.f64 d%d, d%d, d%d", opName, vfpInstr->dReg, vfpInstr->operands.regs.n, vfpInstr->operands.regs.m);
            else // VFP_OP_INPUT_Sm
                LOG("Emulating %s.f64 d%d, d%d", opName, vfpInstr->dReg, vfpInstr->operands.regs.m);
        }
    }
}
#else
static void PrintVFPInstr(VFPInstruction *vfpInstr)
{
}
#endif

uint32_t vfpOpInputFlags[VFP_OP_Count] =
    {
        VFP_OP_INPUT_Sm,                                     // VFP_OP_VMOV
        VFP_OP_INPUT_IMM,                                    // VFP_OP_VMOV_IMM
        VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm,                   // VFP_OP_VADD
        VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm,                   // VFP_OP_VSUB
        VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm,                   // VFP_OP_VDIV
        VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm,                   // VFP_OP_VMUL
        VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm,                   // VFP_OP_VNMUL
        VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VMLA
        VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VNMLA
        VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VMLS
        VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VNMLS
        VFP_OP_INPUT_Sm,                                     // VFP_OP_VABS
        VFP_OP_INPUT_Sm,                                     // VFP_OP_VNEG
        VFP_OP_INPUT_Sm                                      // VFP_OP_VSQRT
};

static void DecodeVFPInstrRegs(uint32_t rawInstr, VFPInstruction *vfpInstr)
{
    uint32_t inputFlags = vfpOpInputFlags[vfpInstr->op];

    if ((rawInstr & 0x00000100) == 0) // sz bit
    {
        vfpInstr->precision = VFP_OP_F32;
        vfpInstr->dReg = ((rawInstr & 0x0000F000) >> 11) | ((rawInstr & 0x00400000) >> 22);
        if ((inputFlags & VFP_OP_INPUT_IMM) == 0)
        {
            if (inputFlags & VFP_OP_INPUT_Sn)
                vfpInstr->operands.regs.n = ((rawInstr & 0x000F0000) >> 15) | ((rawInstr & 0x00000080) >> 7);
            vfpInstr->operands.regs.m = ((rawInstr & 0x0000000F) << 1) | ((rawInstr & 0x00000020) >> 5);
        }
        else
        {
            union
            {
                float f32;
                uint32_t u32;
            } rawFloat;
            uint32_t opc2 = (rawInstr & 0x000F0000) >> 16;
            uint32_t opc4 = rawInstr & 0x0000000F;
            rawFloat.u32 = ((opc2 & 0x8) << 28) |                                // Sign bit
                           (((opc2 & 0x3) | (opc2 & 0x4 ? 0x7C : 0x80)) << 23) | // Exponent
                           (opc4 << 19);                                         // Mantissa

            vfpInstr->operands.imm.f32 = rawFloat.f32;
        }
    }
    else
    {
        vfpInstr->precision = VFP_OP_F64;
        vfpInstr->dReg = ((rawInstr & 0x00400000) >> 18) | ((rawInstr & 0x0000F000) >> 12);
        if ((inputFlags & VFP_OP_INPUT_IMM) == 0)
        {
            if (inputFlags & VFP_OP_INPUT_Sn)
                vfpInstr->operands.regs.n = ((rawInstr & 0x00000080) >> 3) | ((rawInstr & 0x000F0000) >> 16);
            vfpInstr->operands.regs.m = ((rawInstr & 0x00000020) >> 1) | (rawInstr & 0x0000000F);
        }
        else
        {
            union
            {
                double f64;
                uint64_t u64;
            } rawFloat;
            uint64_t opc2 = (rawInstr & 0x000F0000) >> 16;
            uint64_t opc4 = rawInstr & 0x0000000F;
            rawFloat.u64 = ((opc2 & 0x8) << 48) |                                  // Sign bit
                           (((opc2 & 0x3) | (opc2 & 0x4 ? 0x3FC : 0x400)) << 52) | // Exponent
                           (opc4 << 48);                                           // Mantissa

            vfpInstr->operands.imm.f64 = rawFloat.f64;
        }
    }
}

static int DecodeVFPInstr(uint32_t rawInstr, VFPInstruction *vfpInstr, bool thumb)
{
    int opc1, opc2, opc3;
    if (thumb)
        rawInstr = ((rawInstr & 0xFFFF) << 16) | ((rawInstr & 0xFFFF0000) >> 16);

    if ((rawInstr & 0x0F000E10) != 0x0E000A00) // Checking for invalid bits
        return 0;

    sceClibMemset(vfpInstr, 0, sizeof(VFPInstruction));
    vfpInstr->cond = rawInstr >> 28;

    opc1 = (rawInstr & 0x00B00000) >> 20; // Bits 23, 21 - 20
    opc2 = (rawInstr & 0x000F0000) >> 16; // Bits 17 - 16
    opc3 = (rawInstr & 0x00000040) >> 6;  // Bit 6, Bit 7 is assumed

    switch (opc1)
    {
    case 0b0000: // VMLA / VMLS
        vfpInstr->op = opc3 == 0 ? VFP_OP_VMLA : VFP_OP_VMLS;
        break;
    case 0b0001: // VNMLS / VNMLA
        vfpInstr->op = opc3 == 0 ? VFP_OP_VNMLS : VFP_OP_VNMLA;
        break;
    case 0b0010: // VMUL / VNMUL
        vfpInstr->op = opc3 == 0 ? VFP_OP_VMUL : VFP_OP_VNMUL;
        break;
    case 0b0011: // VADD / VSUB
        vfpInstr->op = opc3 == 0 ? VFP_OP_VADD : VFP_OP_VSUB;
        break;
    case 0b1000:       // VDIV
        if (opc3 != 0) // Invalid opc3
            return 0;
        vfpInstr->op = VFP_OP_VDIV;
        break;
    case 0b1011:       // Special Ops
        if (opc3 == 0) // VMOV Immediate
        {
            vfpInstr->op = VFP_OP_VMOV_IMM;
            break;
        }

        switch (opc2)
        {
        case 0b0000:
            vfpInstr->op = opc3 == 0b01 ? VFP_OP_VMOV : VFP_OP_VABS;
            break;
        case 0b0001:
            vfpInstr->op = opc3 == 0b01 ? VFP_OP_VNEG : VFP_OP_VSQRT;
            break;
        }
        break;
    default:
        return 0;
        break;
    }

    DecodeVFPInstrRegs(rawInstr, vfpInstr);

    return 1;
}

void UndefInstrHandler(KuKernelAbortContext *abortContext)
{
    VFPInstruction instr;
    uint32_t FPSCR;

    if (abortContext->abortType != KU_KERNEL_ABORT_TYPE_UNDEF_INSTR)
    {
        LOG("Not an undefined instruction exception");
        nextHandler(abortContext);
        return;
    }

    if ((abortContext->FPEXC & 0x20000000) == 0) // Ignore if not a VFP exception
    {
        LOG("Not a VFP vector exception");
        nextHandler(abortContext);
        return;
    }

    if (DecodeVFPInstr(*(uint32_t *)(abortContext->pc), &instr, abortContext->SPSR & 0x20) == 0)
    {
        LOG("Failed to decode instruction (0x%08X)", *(uint32_t *)(abortContext->pc));
        nextHandler(abortContext);
        return;
    }

    __asm__ volatile("vmrs %0, FPSCR"
                     : "=r"(FPSCR));
    FPSCR &= ~0x370000;
    __asm__ volatile("vmsr FPSCR, %0" ::"r"(FPSCR));
    instr.vectorLength = ((abortContext->FPSCR & 0x70000) >> 16) + 1;
    instr.vectorStride = ((abortContext->FPSCR & 0x300000) >> 20) + 1;

    if (GenerateVFPEmulation(&instr, abortContext) == 0)
    {
        if (instr.precision == VFP_OP_F32)
            EmulateF32VFPInstr(&instr, abortContext);
        else
            EmulateF64VFPInstr(&instr, abortContext);

        abortContext->pc += 4;
    }

    abortContext->FPEXC &= ~0x20000000;

    return;
}

void RegisterHandler()
{
    LOG("Registering handler.");
    kuKernelRegisterAbortHandler(UndefInstrHandler, &nextHandler, NULL);
}