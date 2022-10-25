.arch armv7-a
.cpu cortex-a9
.fpu neon
.align 4
.syntax unified
.arm

.section .text

#define KU_KERNEL_ABORT_TYPE_DATA_ABORT 0
#define KU_KERNEL_ABORT_TYPE_PREFETCH_ABORT 1
#define KU_KERNEL_ABORT_TYPE_UNDEF_INSTR 2

@ Exception safety has 3 levels: 
@ 0 - No safety measures
@ 1 - The stack pointer validity and free space is checked
@ 2 - Copies to/from user space are exception safe
#if !defined(EXCEPTION_SAFETY)
#define EXCEPTION_SAFETY 1
#endif

.global GetProcessAbortHandler
.global GetProcessExitAddr
.global CheckStackPointer
.global ksceKernelCpuSpinLockIrqSave
.global ksceKernelCpuSpinLockIrqRestore
.global ksceKernelMemcpyToUser
.global ksceKernelMemcpyFromUser

.global userAbortBase
.global defaultUserAbortHandler
.global handlersMutex

.macro CopyFromUser
#if (EXCEPTION_SAFETY >= 2)
    mov r12, sp
    cps #0x1F
    mov sp, r12
    bl ksceKernelMemcpyFromUser
    cps #0x17
#else
    bl memcpy
    mov r0, #0
#endif
.endm

.macro CopyToUser
#if (EXCEPTION_SAFETY >= 2)
    mov r12, sp
    cps #0x1F
    mov sp, r12
    bl ksceKernelMemcpyToUser
    cps #0x17
#else
    bl memcpy
    mov r0, #0
#endif
.endm

/**
 * LockSpinLock:
 * Locks the handlersMutex spinlock
 * 
 * Output: 
 * r0 - Saved IRQ State
 */
LLockSpinLock:
    str lr, [sp, #-0x4]!
    ldr r0, =handlersMutex
    bl ksceKernelCpuSpinLockIrqSave
    ldr pc, [sp], #0x4

/**
 * UnlockSpinLock:
 * Unlocks the handlersMutex spinlock 
 *
 * Inputs:
 * r0 - IRQ state to restor
 */
LUnlockSpinLock:
    str lr, [sp, #-0x4]!
    mov r1, r0
    ldr r0, =handlersMutex
    bl ksceKernelCpuSpinLockIrqRestore
    ldr pc, [sp], #0x4

/**
 * FatalError:
 * Causes the thread to execute sceKernelExitProcess
 */
LFatalError:
    bl GetProcessExitAddr
    bic r2, r0, #0x1
    mov r0, #0xFFFFFFFF
    mov r1, #0x30 @ Return to user mode in thumb execution state
    str r1, [sp, #-0x4]!
    str r2, [sp, #-0x4]!
    clrex
    rfeia sp!

/**
 * SaveRegisterContext
 * Saves the exception register context on the stack.
 *
 * Inputs:
 * [sp, #0x0] - abortType
 * [sp, #0x4] - Exception PC
 */
LSaveRegisterContext:
    sub sp, #0x158 @ Reserve space on the stack
    stmia sp, {r0 - r12} @ Save r0 - r12
    add r4, sp, #0x34
    stmia r4, {sp, lr}^ @ Save user-space sp and lr
    add r4, #0xC
    mrs r0, SPSR
    vmrs r1, FPSCR
    vmrs r2, FPEXC
    vstmia r4!, {d0 - d15} @ Save d0 - d15
    vstmia r4!, {d16 - d31} @ Save d16 - d31
    stmia r4!, {r0 - r2} @ Save SPSR, FPSCR and FPEXC
    ldrd r2, r3, [r4, #0x158 - 0x14C] @ Load the passed abort type
    cmp r2, #KU_KERNEL_ABORT_TYPE_UNDEF_INSTR @ Get the relevant FSR and FAR
    beq LSaveRegisterContext_UndefInstr
    cmp r2, #KU_KERNEL_ABORT_TYPE_DATA_ABORT
    mrceq p15, #0, r0, c5, c0, #0 @ DFSR
    mrceq p15, #0, r1, c6, c0, #0 @ DFAR
    mrcne p15, #0, r0, c5, c0, #1 @ IFSR
    mrcne p15, #0, r1, c6, c0, #2 @ IFAR
    stmia r4, {r0 - r2} @ Store FSR, FAR and abortType
    subeq r0, r3, #0x8
    subne r0, r3, #0x4
    str r0, [sp, #0x3C]
    mov r0, sp
    bx lr
LSaveRegisterContext_UndefInstr:
    movw r0, #0x0
    movt r0, #0x0
    mov r1, r0
    stmia r4, {r0 - r2} @ Store FSR, FAR and abortType
    ldr r0, [sp, #0x140] @ Load SPSR
    ands r0, #0x20 @ Test for T bit
    subeq r0, r3, #0x4
    subne r0, r3, #0x2
    str r0, [sp, #0x3C]
    mov r0, sp
    bx lr

LReturnFromAbortHandler:
    mov r1, #0 @ Clear FSR and FAR registers
    mcr p15, #0, r1, c5, c0, #0 
    mcr p15, #0, r1, c5, c0, #1 
    mcr p15, #0, r1, c5, c1, #0 
    mcr p15, #0, r1, c5, c1, #1 
    mcr p15, #0, r1, c6, c0, #0 
    mcr p15, #0, r1, c6, c0, #2
    sub sp, #0x160 @ 0x158 for the abort context, 0x8 for the PC and SPSR 
    mov r1, r0
    mov r0, sp
    mov r2, #0x158
    CopyFromUser
    cmp r0, #0
    addne sp, #0x160
    bne LFatalError
    mov r4, sp
    ldr r0, [r4, #0x144]
    ldr r1, [r4, #0x148]
    vmsr FPSCR, r0
    vmsr FPEXC, r1
    add r0, r4, #0x40
    vldmia r0!, {d0 - d15}
    vldmia r0!, {d16 - d31}
    mov r12, r4
    ldmia r12!, {r0 - r11}
    add r12, #0x4
    ldmia r12, {sp, lr}^
    sub r12, #0x34
    add sp, #0x158
    ldr lr, [r12, #0x140]
    str lr, [sp, #0x4]
    ldr lr, [r12, #0x3C]
    str lr, [sp]
    ldr r12, [r12, #0x30]
    clrex
    rfeia sp!

LReturnFromDefaultAbortHandler:
    mov r1, #0 @ Clear FSR and FAR registers
    mcr p15, #0, r1, c5, c0, #0 
    mcr p15, #0, r1, c5, c0, #1 
    mcr p15, #0, r1, c5, c1, #0 
    mcr p15, #0, r1, c5, c1, #1 
    mcr p15, #0, r1, c6, c0, #0 
    mcr p15, #0, r1, c6, c0, #2
    sub sp, #0x160 @ 0x158 for the abort context, 8 bytes for the address of the next handler and alignment
    mov r1, r0
    mov r0, sp
    mov r2, #0x158
    CopyFromUser
    cmp r0, #0
    addne sp, #0x160
    bne LFatalError
    mov r4, sp
    ldr r2, [r4, #0x154] @ abortType
    ldr lr, [r4, #0x3C]
    cmp r2, #KU_KERNEL_ABORT_TYPE_UNDEF_INSTR @ Set the relevant FSR and FAR
    beq LReturnFromDefaultAbortHandler_Undef
    add r5, r4, #0x14C
    ldrd r0, r1, [r5]
    mcreq p15, #0, r0, c5, c0, #0
    mcreq p15, #0, r1, c6, c0, #0
    mcrne p15, #0, r0, c5, c0, #1
    mcrne p15, #0, r1, c6, c0, #2
    ldreq r2, DabtExceptionHandler_lvl0 @ Branch to next exception handler
    ldrne r2, PabtExceptionHandler_lvl0
    addeq lr, #0x8
    addne lr, #0x4
    b LReturnFromDefaultAbortHandler_continued
LReturnFromDefaultAbortHandler_Undef:
    ldr r2, UndefExceptionHandler_lvl0
    ldr r0, [r4, #0x140]
    ands r0, #0x20
    addeq lr, #0x4
    addne lr, #0x2
LReturnFromDefaultAbortHandler_continued:
    add r2, #8
    str r2, [sp, #0x15C]!
    ldr r0, [r4, #0x144]
    ldr r1, [r4, #0x148]
    vmsr FPSCR, r0
    vmsr FPEXC, r1
    ldr r0, [r4, #0x140]
    msr SPSR, r0
    add r0, r4, #0x40
    vldmia r0!, {d0 - d15}
    vldmia r0!, {d16 - d31}
    mov r12, r4
    ldmia r12!, {r0 - r11}
    add r12, #0x4
    ldmia r12, {sp, lr}^
    sub r12, #0x34
    ldr r12, [r12, #0x30]
    ldr pc, [sp], #0x4

/**
 * LReturnToAbortHandler:
 * Saves register context to thread stack, and returns to user-space to execute the abort handler
 * 
 * Inputs:
 * [sp, #0x0] - abortType
 * [sp, #0x4] - Exception PC
 */
LReturnToAbortHandler:
    bl LSaveRegisterContext
    bl LLockSpinLock
    mov r6, r0
    mov r0, #0xFFFFFFFF
    bl GetProcessAbortHandler
    cmp r0, #0
    beq LReturnToAbortHandler_return @ No process abort handler. Go to the next exception handler
    ldr r5, [r0, #0x8]
    ldr r1, =defaultUserAbortHandler
    ldr r1, [r1]
    cmp r5, r1
    beq LReturnToAbortHandler_return @ Process abort handler is default, Go to the next exception handler
#if (EXCEPTION_SAFETY >= 1)
    ldr r0, [sp, #0x34]
#if (EXCEPTION_SAFETY < 2)
    bl CheckStackPointer
#else
    mov r12, sp
    cps #0x1F
    mov sp, r12
    bl CheckStackPointer
    cps #0x17
#endif
    cmp r0, #0x1
    bne LReturnToAbortHandler_return @ Go to next abort handler if the user stack isn't usable
#endif
    ldr r0, [sp, #0x34]
    sub r0, #0x158
    mov r4, r0
    mov r1, sp
    mov r2, #0x158
    CopyToUser
    cmp r0, #0
    bne LReturnToAbortHandler_return @ If copying fails, go to the next exception handler
    mov r0, r6
    bl LUnlockSpinLock
    mov r0, r4
    mov r1, r5
    add sp, #0x158
    str r0, [sp]
    ldmia sp, {sp}^
    ldr r2, =userAbortBase
    ldr r2, [r2]
    mov r3, #0x10 @ CPSR value (USR mode)
    strd r2, r3, [sp]
    clrex
    rfeia sp!
LReturnToAbortHandler_return:
    mov r0, r6
    bl LUnlockSpinLock
    mov   r1, #0 @ Clear FSR and FAR registers
    mcr   p15, #0, r1, c5, c0, #0 
    mcr   p15, #0, r1, c5, c0, #1 
    mcr   p15, #0, r1, c5, c1, #0 
    mcr   p15, #0, r1, c5, c1, #1 
    mcr   p15, #0, r1, c6, c0, #0 
    mcr   p15, #0, r1, c6, c0, #2
    add r4, sp, #0x40
    vldmia r4!, {d0 - d15}
    vldmia r4!, {d16 - d31}
    ldmia r4!, {r0 - r2}
    msr SPSR, r0
    vmsr FPSCR, r1
    vmsr FPEXC, r2
    ldmia r4!, {r0 - r2} @ Load the abort type
    cmp r2, #KU_KERNEL_ABORT_TYPE_DATA_ABORT @ Get the relevant FSR and FAR
    mcreq p15, #0, r0, c5, c0, #0
    mcreq p15, #0, r1, c6, c0, #0
    mcrne p15, #0, r0, c5, c0, #1
    mcrne p15, #0, r1, c6, c0, #2
    ldreq r2, DabtExceptionHandler_lvl0 @ Branch to next exception handler
    ldrne r2, PabtExceptionHandler_lvl0
    ldr lr, [sp, #0x3C]
    addeq lr, #0x8
    addne lr, #0x4
    add r2, #8
    str r2, [sp, #0x15C]
    add r2, sp, #0x34
    ldmia r2, {sp, lr}^
    ldmia sp, {r0 - r12}
    add sp, #0x15C
    ldr pc, [sp], #0x4

.global DabtExceptionHandler_lvl0
DabtExceptionHandler_lvl0:
    .word 0x0
    .word 0x0
    push {r2, r3, r4, lr} @ Save r2 and r3 for later, r4 for extra space, and r5 for alignment
    mrs r2, SPSR
    and r2, #0x1F
    cmp r2, #0x10 @ User mode
    bne LDabtExceptionHandler_lvl0_continue
    ldr r2, =userAbortBase
    ldr r2, [r2]
    lsr r3, lr, #0xC
    cmp r3, r2, lsr #0xC
    beq LDabtExceptionHandler_lvl0_ReturnFromHandler
    mov r2, #KU_KERNEL_ABORT_TYPE_DATA_ABORT
    str r2, [sp, #0x8]
    pop {r2, r3}
    b LReturnToAbortHandler @ Go to ReturnToAbortHandler with abortType in [sp, #0x0]
LDabtExceptionHandler_lvl0_ReturnFromHandler:
    add sp, #0x10
    cmp r1, #0
    beq LReturnFromAbortHandler
    bne LReturnFromDefaultAbortHandler
LDabtExceptionHandler_lvl0_continue:
    ldr r2, DabtExceptionHandler_lvl0
    add r2, #8
    str r2, [sp, #0xC]
    pop {r2 - r4, pc} @ Reload saved registers and branch to the regular lvl0 handler

.global PabtExceptionHandler_lvl0
PabtExceptionHandler_lvl0:
    .word 0x0
    .word 0x0
    push {r2, r3, r4, lr}
    mrs r2, SPSR
    and r2, #0x1F
    cmp r2, #0x10
    bne LPabtExceptionHandler_lvl0_continue
    mov r2, #KU_KERNEL_ABORT_TYPE_PREFETCH_ABORT
    str r2, [sp, #0x8]
    pop {r2, r3}
    b LReturnToAbortHandler
LPabtExceptionHandler_lvl0_continue:
    ldr r2, PabtExceptionHandler_lvl0
    add r2, #8
    str r2, [sp, #0xC]
    pop {r2 - r4, pc}

.global UndefExceptionHandler_lvl0
UndefExceptionHandler_lvl0:
    .word 0x0
    .word 0x0
    push {r2, r3, r4, lr} @ Save r2 and r3 for later, r4 for extra space, and r5 for alignment
    mrs r2, SPSR
    and r2, #0x1F
    cmp r2, #0x10 @ User mode
    bne LUndefExceptionHandler_lvl0_continue
    mov r2, #KU_KERNEL_ABORT_TYPE_UNDEF_INSTR
    str r2, [sp, #0x8]
    pop {r2, r3}
    b LReturnToAbortHandler @ Go to ReturnToAbortHandler with abortType in [sp, #0x0]
LUndefExceptionHandler_lvl0_continue:
    ldr r2, DabtExceptionHandler_lvl0
    add r2, #8
    str r2, [sp, #0xC]
    pop {r2 - r4, pc} @ Reload saved registers and branch to the regular lvl0 handler
