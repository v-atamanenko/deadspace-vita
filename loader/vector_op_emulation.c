#include <kubridge.h>
#include <stdbool.h>
#include <psp2/kernel/clib.h>

#ifndef NDEBUG
#define LOG(msg, ...) sceClibPrintf("%s:%d:" msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(msg, ...)
#endif

KuKernelAbortHandler nextHandler;

// All possible VFP vector operations
enum VFPOp
{
    VFP_OP_UNK,
    VFP_OP_VMOV,
    VFP_OP_VMOV_IMM,
    VFP_OP_VADD,
    VFP_OP_VSUB,
    VFP_OP_VDIV,
    VFP_OP_VMUL,
    VFP_OP_VNMUL,
    VFP_OP_VMLA,
    VFP_OP_VNMLA,
    VFP_OP_VMLS,
    VFP_OP_VNMLS,
    VFP_OP_VABS,
    VFP_OP_VNEG,
    VFP_OP_VSQRT
};

enum VFPOpInputFlags
{
    VFP_OP_INPUT_Sd = (1 << 0),
    VFP_OP_INPUT_Sn = (1 << 1),
    VFP_OP_INPUT_Sm = (1 << 2),
    VFP_OP_INPUT_IMM = (1 << 3)
};

uint32_t vfpOpInputFlags[] = 
{
    0, // VFP_OP_UNK
    VFP_OP_INPUT_Sm, // VFP_OP_VMOV
    VFP_OP_INPUT_IMM, // VFP_OP_VMOV_IMM
    VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VADD
    VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VSUB
    VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VDIV
    VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VMUL
    VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VNMUL
    VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VMLA
    VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VNMLA
    VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VMLS
    VFP_OP_INPUT_Sd | VFP_OP_INPUT_Sn | VFP_OP_INPUT_Sm, // VFP_OP_VNMLS
    VFP_OP_INPUT_Sm, // VFP_OP_VABS
    VFP_OP_INPUT_Sm, // VFP_OP_VNEG
    VFP_OP_INPUT_Sm  // VFP_OP_VSQRT
};

enum VFPOpPrecision
{
    VFP_OP_F32,
    VFP_OP_F64
};

typedef struct ArmInstruction
{
    uint8_t op;
    uint8_t precision;
    uint16_t destReg;
    uint16_t src1Reg;
    uint16_t src2Reg;
    union
    {
        float f32;
        double f64;
    } imm;
} ArmInstruction;

typedef void (*VFPOpImpl)(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2);

static void VMOV_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = src2[i];
    }
}
static void VMOV_IMM_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) 
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = instr->imm.f32;
    }
}
static void VADD_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = src1[i] + src2[i];
    }
}
static void VSUB_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = src1[i] - src2[i];
    }
}
static void VDIV_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = src1[i] / src2[i];
    }
}
static void VMUL_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = src1[i] * src2[i];
    }
}
static void VNMUL_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = -(src1[i] * src2[i]);
    }
}
static void VMLA_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = (src1[i] * src2[i]) + dest[i];
    }
}
static void VNMLA_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) 
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = -((src1[i] * src2[i]) + dest[i]);
    }
}
static void VMLS_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2)
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = dest[i] - (src1[i] * src2[i]);
    }
}
static void VNMLS_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) 
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = -(dest[i] - (src1[i] * src2[i]));
    }
}
static void VABS_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) 
{
    for (int i = 0; i < vectorLength; i++)
    {
        uint32_t tmp = ((uint32_t *)src2)[i];
        tmp &= ~0x80000000;
        dest[i] = *(float *)(&tmp);
    }
}
static void VNEG_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) 
{
    for (int i = 0; i < vectorLength; i++)
    {
        dest[i] = -src2[i];
    }
}
static void VSQRT_Impl(ArmInstruction *instr, int vectorLength, float *dest, float *src1, float *src2) {}


VFPOpImpl vfpOpFns[] = 
{
    NULL,
    VMOV_Impl,
    VMOV_IMM_Impl,
    VADD_Impl,
    VSUB_Impl,
    VDIV_Impl,
    VMUL_Impl,
    VNMUL_Impl,
    VMLA_Impl,
    VNMLA_Impl,
    VMLS_Impl,
    VNMLS_Impl,
    VABS_Impl,
    VNEG_Impl,
    VSQRT_Impl
};

void DecodeArmVFPInstrRegs(uint32_t rawInstr, ArmInstruction *armInstr)
{
    armInstr->precision = rawInstr & 0x00000100;

    uint32_t inputFlags = vfpOpInputFlags[armInstr->op];

    if (armInstr->precision == VFP_OP_F32)
    {
        armInstr->destReg = ((rawInstr & 0x0000F000) >> 11) | ((rawInstr & 0x00400000) >> 22);
        if (inputFlags & VFP_OP_INPUT_Sn)
            armInstr->src1Reg = ((rawInstr & 0x000F0000) >> 15) | ((rawInstr & 0x00000080) >> 7);
        if (inputFlags & VFP_OP_INPUT_Sm)
            armInstr->src2Reg = ((rawInstr & 0x0000000F) << 1) | ((rawInstr & 0x00000020) >> 5);
        if (inputFlags & VFP_OP_INPUT_IMM)
        {
            uint32_t opc2 = (rawInstr & 0x000F0000) >> 16;
            uint32_t opc4 = rawInstr & 0x0000000F;
            uint32_t rawFloat = ((opc2 & 0x8) << 28) | // Sign bit
                                (((opc2 & 0x3) | (opc2 & 0x4 ? 0x7C : 0x80)) << 23) | // Exponent
                                (opc4 << 19); // Mantissa

            armInstr->imm.f32 = *(float *)&rawFloat;
        }
    }
    else
    {
        armInstr->destReg = ((rawInstr & 0x00400000) >> 18) | ((rawInstr & 0x0000F000) >> 12);
        if (inputFlags & VFP_OP_INPUT_Sn)
            armInstr->src1Reg = ((rawInstr & 0x00000080) >> 3) | ((rawInstr & 0x000F0000) >> 16);
        if (inputFlags & VFP_OP_INPUT_Sm)
            armInstr->src2Reg = ((rawInstr & 0x00000020) >> 1) | ((rawInstr & 0x0000000F) << 1);
    }
}

int DecodeArmVFPInstr(uint32_t rawInstr, ArmInstruction *armInstr)
{
    int opc1, opc2, opc3;

    if ((rawInstr & 0x0F000E10) != 0x0E000A00) // Checking for invalid bits
        return 0;

    opc1 = (rawInstr & 0x00B00000) >> 20; // Bits 23, 21 - 20
    opc2 = (rawInstr & 0x000F0000) >> 16; // Bits 17 - 16
    opc3 = (rawInstr & 0x00000040) >> 6; // Bit 6, Bit 7 is assumed

    switch (opc1)
    {
    case 0b0000: // VMLA / VMLS
        armInstr->op = opc3 == 0 ? VFP_OP_VMLA : VFP_OP_VMLS;
        break;
    case 0b0001: // VNMLS / VNMLA
        armInstr->op = opc3 == 0 ? VFP_OP_VNMLS : VFP_OP_VNMLA;
        break;
    case 0b0010: // VMUL / VNMUL
        armInstr->op = opc3 == 0 ? VFP_OP_VMUL : VFP_OP_VNMUL;
        break;
    case 0b0011: // VADD / VSUB
        armInstr->op = opc3 == 0 ? VFP_OP_VADD : VFP_OP_VSUB;
        break;
    case 0b1000: // VDIV
        if (opc3 != 0) // Invalid opc3
            return 0;
        armInstr->op = VFP_OP_VDIV;
        break;
    case 0b1011: // Special Ops
        if (opc3 == 0) //VMOV Immediate
        {
            armInstr->op = VFP_OP_VMOV_IMM;
            break;
        }

        switch (opc2)
        {
        case 0b0000:
            armInstr->op = opc3 == 0b01 ? VFP_OP_VMOV : VFP_OP_VABS;
            break;
        case 0b0001:
            armInstr->op = opc3 == 0b01 ? VFP_OP_VNEG : VFP_OP_VSQRT;
            break;
        }
        break;
    default:
        return 0;
        break;
    }

    DecodeArmVFPInstrRegs(rawInstr, armInstr);

    return 1;
}

static inline void CopySourceVector(float *registerBuffer, int registerIndex, int vectorLength, int vectorStride, float* outputBuffer)
{
    int regBufferOffset = registerIndex;
    int regBankBase = registerIndex & 0x18;
    for (int i = 0; i < vectorLength; i++)
    {
        outputBuffer[i] = registerBuffer[regBufferOffset];

        regBufferOffset += vectorStride;

        regBufferOffset = (regBufferOffset & 0x7) | regBankBase; // Ensure it wraps around in the regbank (not sure if this is correct).
    }
}

static inline void CopyDestVector(float *vectorBuffer, int registerIndex, int vectorLength, int vectorStride, float* registerBuffer)
{
    int regBufferOffset = registerIndex;
    int regBankBase = registerIndex & 0x18;
    for (int i = 0; i < vectorLength; i++)
    {
        registerBuffer[regBufferOffset] = vectorBuffer[i];

        regBufferOffset += vectorStride;

        regBufferOffset = (regBufferOffset & 0x7) | regBankBase; // Ensure it wraps around in the regbank (not sure if this is correct).
    }
}

void LoadSourceVFPRegisters(ArmInstruction *armInstr, KuKernelAbortContext *abortContext, float *src0Buffer, float *src1Buffer, float *src2Buffer, int vectorLength, int vectorStride)
{
    bool src1Scalar = armInstr->src2Reg < 8;

    uint32_t inputFlags = vfpOpInputFlags[armInstr->op];

    if (inputFlags & VFP_OP_INPUT_Sn)
        CopySourceVector((float *)(&abortContext->vfpRegisters[0]), armInstr->src1Reg, vectorLength, vectorStride, src0Buffer);
    if (inputFlags & VFP_OP_INPUT_Sm)
        CopySourceVector((float *)(&abortContext->vfpRegisters[0]), armInstr->src2Reg, vectorLength, src1Scalar ? 0 : vectorStride, src1Buffer);
    if (inputFlags & VFP_OP_INPUT_Sd)
        CopySourceVector((float *)(&abortContext->vfpRegisters[0]), armInstr->destReg, vectorLength, vectorStride, src2Buffer);
}

void UndefInstrHandler(KuKernelAbortContext *abortContext)
{
    ArmInstruction instr;
    uint32_t FPSCR;
    int vectorLength, vectorStride;
    float src0[8], src1[8], dst[8];

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

    if ((abortContext->SPSR & 0x20) != 0) // Thumb variants currently unsupported
    {
        LOG("Thumb not supported");
        nextHandler(abortContext);
        return;
    }

    if (DecodeArmVFPInstr(*(uint32_t *)(abortContext->pc), &instr) == 0)
    {
        LOG("Failed to decode instruction (0x%08X)", *(uint32_t *)(abortContext->pc));
        nextHandler(abortContext);
        return;
    }

    if (instr.precision == VFP_OP_F64) // Double precision emulation not supported for now
    {
        LOG("Double precision emulation not supported");
        nextHandler(abortContext);
        return;
    }

    __asm__ volatile ("vmrs %0, FPSCR" : "=r"(FPSCR));
    FPSCR &= ~0x370000;
    __asm__ volatile ("vmsr FPSCR, %0" :: "r"(FPSCR));

    //LOG("Emulating %S Op. Sd: s%d Sn: s%d, Sm: s%d\n", instr.op, instr.destReg, instr.src1Reg, instr.src2Reg);

    vectorLength = ((abortContext->FPSCR & 0x70000) >> 16) + 1;
    vectorStride = ((abortContext->FPSCR & 0x300000) >> 20) + 1;
    LoadSourceVFPRegisters(&instr, abortContext, &src0[0], &src1[0], &dst[0], vectorLength, vectorStride);

    VFPOpImpl fn = vfpOpFns[instr.op];
    fn(&instr, vectorLength, dst, src0, src1);

    CopyDestVector(dst, instr.destReg, vectorLength, vectorStride, (float *)(&abortContext->vfpRegisters[0]));

    abortContext->FPEXC &= ~0x20000000;
    abortContext->pc += 4;

    return;
}

void RegisterHandler()
{
    kuKernelRegisterAbortHandler(UndefInstrHandler, &nextHandler, NULL);
}
