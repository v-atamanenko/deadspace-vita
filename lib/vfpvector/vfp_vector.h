#ifndef VFP_VECTOR_H_
#define VFP_VECTOR_H_

#include <stdint.h>
#include "kubridge/kubridge.h"
#include <psp2/kernel/clib.h>

#if 1
#define LOG(msg, ...) sceClibPrintf("%s:%d:" msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(msg, ...)
#endif

// #define NO_VFP_GEN // Uncomment to disable VFP code gen

// All possible VFP vector operations
enum VFPOp
{
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
    VFP_OP_VSQRT,

    VFP_OP_Count
};

enum VFPOpPrecision
{
    VFP_OP_F32,
    VFP_OP_F64
};

enum VFPOpInputFlags
{
    VFP_OP_INPUT_Sd = (1 << 0),
    VFP_OP_INPUT_Sn = (1 << 1),
    VFP_OP_INPUT_Sm = (1 << 2),
    VFP_OP_INPUT_IMM = (1 << 3)
};

typedef struct VFPInstruction
{
    uint8_t op;
    uint8_t cond;
    uint8_t precision;
    uint8_t dReg; // Sd / Dd
    union 
    {
        struct
        {
            uint16_t n; // Sn / Dn
            uint16_t m; // Sm / Dm
        } regs;

        union
        {
            float f32;
            double f64;
        } imm;
    } operands;

    uint16_t vectorLength;
    uint16_t vectorStride;
} VFPInstruction;

extern uint32_t vfpOpInputFlags[VFP_OP_Count];

void EmulateF32VFPInstr(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext);
void EmulateF64VFPInstr(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext);

int GenerateVFPEmulation(VFPInstruction *vfpInstr, KuKernelAbortContext *abortContext);

void RegisterHandler();

#endif