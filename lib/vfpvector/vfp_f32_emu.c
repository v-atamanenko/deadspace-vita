#include "vfp_vector.h"

#include <stdbool.h>

static inline void VMOV_F32(float *sd, float sn, float sm)
{
    *sd = sm;
}
static inline void VADD_F32(float *sd, float sn, float sm)
{
    *sd = sn + sm;
}
static inline void VSUB_F32(float *sd, float sn, float sm)
{
    *sd = sn - sm;
}
static inline void VDIV_F32(float *sd, float sn, float sm)
{
    *sd = sn / sm;
}
static inline void VMUL_F32(float *sd, float sn, float sm)
{
    *sd = sn * sm;
}
static inline void VNMUL_F32(float *sd, float sn, float sm)
{
    *sd = -(sn * sm);
}
static inline void VMLA_F32(float *sd, float sn, float sm)
{
    *sd = (sn * sm) + *sd;
}
static inline void VNMLA_F32(float *sd, float sn, float sm)
{
    *sd = -((sn * sm) + *sd);
}
static inline void VMLS_F32(float *sd, float sn, float sm)
{
    *sd = *sd - (sn * sm);
}
static inline void VNMLS_F32(float *sd, float sn, float sm)
{
    *sd = -(*sd - (sn * sm));
}
static inline void VABS_F32(float *sd, float sn, float sm)
{
    union
    {
        float f32;
        uint32_t u32;
    } tmp;

    tmp.f32 = sm;
    tmp.u32 &= ~(1 << 31);
    *sd = tmp.f32;
}
static inline void VNEG_F32(float *sd, float sn, float sm)
{
    *sd = -sm;
}
static inline void VSQRT_F32(float *sd, float sn, float sm)
{
    __asm__ volatile(
    "vmov s1, %1\n"
    "vsqrt.f32 s0, s1\n"
    "vstr s0, [%0]\n" ::"r"(sd),
    "r"(sm));
}

void EmulateF32VFPInstr(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext)
{
    float *registerBuffer = (float *)(&abortContext->vfpRegisters[0]);
    const float imm = vfpInstr->operands.imm.f32;

    int dReg = vfpInstr->dReg, nReg = vfpInstr->operands.regs.n, mReg = vfpInstr->operands.regs.m;
    int dRegBank = dReg & 0x18, nRegBank = nReg & 0x18, mRegBank = mReg & 0x18;
    int mStride = mRegBank == 0 ? 0 : vfpInstr->vectorStride; // m may be a scalar register
    for (int i = 0; i < vfpInstr->vectorLength; i++)
    {
        float *sd = &registerBuffer[dReg];
        float sn = registerBuffer[nReg];
        float sm = registerBuffer[mReg];
        switch (vfpInstr->op)
        {
            case VFP_OP_VMOV:
                VMOV_F32(sd, 0, sm);
                break;
            case VFP_OP_VMOV_IMM:
                VMOV_F32(sd, 0, imm);
                break;
            case VFP_OP_VADD:
                VADD_F32(sd, sn, sm);
                break;
            case VFP_OP_VSUB:
                VSUB_F32(sd, sn, sm);
                break;
            case VFP_OP_VDIV:
                VDIV_F32(sd, sn, sm);
                break;
            case VFP_OP_VMUL:
                VMUL_F32(sd, sn, sm);
                break;
            case VFP_OP_VNMUL:
                VNMUL_F32(sd, sn, sm);
                break;
            case VFP_OP_VMLA:
                VMLA_F32(sd, sn, sm);
                break;
            case VFP_OP_VNMLA:
                VNMLA_F32(sd, sn, sm);
                break;
            case VFP_OP_VMLS:
                VMLS_F32(sd, sn, sm);
                break;
            case VFP_OP_VNMLS:
                VNMLS_F32(sd, sn, sm);
                break;
            case VFP_OP_VABS:
                VABS_F32(sd, 0, sm);
                break;
            case VFP_OP_VNEG:
                VNEG_F32(sd, 0, sm);
                break;
            case VFP_OP_VSQRT:
                VSQRT_F32(sd, 0, sm);
                break;
        }

        dReg = ((dReg + vfpInstr->vectorStride) & 0x7) | dRegBank; // Ensure they wrap around in the regbank (not sure if this is correct).
        nReg = ((nReg + vfpInstr->vectorStride) & 0x7) | nRegBank;
        mReg = ((mReg + mStride) & 0x7) | mRegBank;
    }
}