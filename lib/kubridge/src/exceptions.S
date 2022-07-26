.arch armv7-a
.cpu cortex-a9
.fpu neon
.align 4
.syntax unified
.arm

.section .text

.global CopyFromExcp
.global GetProcessAbortHandler
.global ReturnFromException
.global CheckStackPointer
.global ksceKernelCpuSpinLockIrqSave
.global ksceKernelCpuSpinLockIrqRestore

.global userAbortBase
.global defaultUserAbortHandler
.global handlersMutex

LReturnFromAbortHandler:
    mov   r1, #0 @ Clear FSR and FAR registers
    mcr   p15, #0, r1, c5, c0, #0 
    mcr   p15, #0, r1, c5, c0, #1 
    mcr   p15, #0, r1, c5, c1, #0 
    mcr   p15, #0, r1, c5, c1, #1 
    mcr   p15, #0, r1, c6, c0, #0 
    mcr   p15, #0, r1, c6, c0, #2
    ldr r0, [r4, #0x144]
    ldr r1, [r4, #0x148]
    vmsr fpscr, r0
    vmsr fpexc, r1
    add r0, r4, #0x40
    vldmia r0!, {d0 - d15}
    vldmia r0!, {d16 - d31}
    mov r12, r4
    ldmia r12!, {r0 - r11}
    add r12, #0x4
    ldmia r12, {sp, lr}^
    sub r12, #0x34
    ldr lr, [r12, #0x140]
    str lr, [sp, #-0x4]!
    ldr lr, [r12, #0x3C]
    str lr, [sp, #-0x4]!
    ldr r12, [r12, #0x30]
    clrex
    rfeia sp!

LReturnFromDefaultAbortHandler:
    mov r4, r0
    ldr r0, =handlersMutex
    ldr r1, =ksceKernelCpuSpinLockIrqSave
    blx r1
    mov r5, r0
    mov r0, #0
    bl GetProcessAbortHandler
    ldr r1, =defaultUserAbortHandler
    ldr r1, [r1]
    str r1, [r0, #0x8]
    ldr r1, =defaultUserAbortHandler
    ldr r1, [r1]
    str r1, [r0, #0x8]
    ldr r0, =handlersMutex
    mov r1, r5
    ldr r2, =ksceKernelCpuSpinLockIrqRestore
    blx r2
    ldr r2, [r4, #0x154] @ abortType
    add r5, r4, #0x14C
    ldrd r0, r1, [r5]
    cmp r2, #0 @ Set the relevant FSR and FAR
    mcreq p15, #0, r0, c5, c0, #0
    mcreq p15, #0, r1, c6, c0, #0
    mcrne p15, #0, r0, c5, c0, #1
    mcrne p15, #0, r1, c6, c0, #2
    ldr r0, [r4, #0x144]
    ldr r1, [r4, #0x148]
    vmsr fpscr, r0
    vmsr fpexc, r1
    add r0, r4, #0x40
    vldmia r0!, {d0 - d15}
    vldmia r0!, {d16 - d31}
    mov r12, r4
    ldmia r12!, {r0 - r11}
    add r12, #0x4
    ldmia r12, {sp, lr}^
    sub r12, #0x34
    ldr lr, [r12, #0x140]
    msr SPSR, lr
    sub sp, #0x400 @ Prepare to jump to the Priority 0 exception handler
    str r0, [sp, #0]
    str r1, [sp, #0x4]
    ldr lr, =DabtExceptionHandler_lvl0_retAddr
    ldr lr, [lr]
    str lr, [sp, #-0x4]!
    ldr lr, [r12, #0x3C]
    str lr, [sp, #-0x4]!
    ldr r12, [r12, #0x30]
    ldmia sp!, {lr, pc}

LSetupAbortContext:
    stmdb sp!, {r4 - r8, lr}
    sub sp, #0x24
    mov r4, r0
    mov r5, r1
    ldr r0, =handlersMutex
    ldr r1, =ksceKernelCpuSpinLockIrqSave
    blx r1
    mov r8, r0
    mov r0, #0xFFFFFFFF
    bl GetProcessAbortHandler
    cmp r0, #0
    beq LSetupAbortContext_return
    ldr r7, [r0, #0x8]
    ldr r1, =defaultUserAbortHandler
    ldr r1, [r1]
    cmp r7, r1
    moveq r0, #0
    beq LSetupAbortContext_return
    ldr r6, [r4, #0x34] @ Load sp_usr
    mov r0, r6
    bl CheckStackPointer
    cmp r0, #0x1
    bne LSetupAbortContext_return
    sub r0, r6, #0x160 @ One abort context is 0x158 bytes, with 8 bytes extra for padding
    mov r1, r4
    mov r2, #0x40
    bl CopyFromExcp @ Copy r0 - r15 to user stack
    add r1, r4, #0x100 @ vfpRegisters
    mov r2, #0x100
    bl CopyFromExcp @ Copy d0 - d31 to user stack
    mov r1, sp
    ldr r2, [r4, #0x44] @ Load SPSR
    ldr r3, [r4, #0x4C] @ Load FPSCR
    strd r2, r3, [r1], #0x8
    ldr r3, [r4, #0x50] @ Load FPEXC
    str r3, [r1], #0x4 @ Store FPEXC
    cmp r5, #0 @ Load the relevant FSR and FAR
    ldreq r2, [r4, #0x70] 
    ldreq r3, [r4, #0x78]
    ldrne r2, [r4, #0x74]
    ldrne r3, [r4, #0x7C]
    strd r2, r3, [r1], #0x8
    str r5, [r1] @ Store the abortType
    mov r1, sp
    mov r2, #0x18
    bl CopyFromExcp @ Copy the SPSR, FPSCR, FPEXC, FAR, FSR and abortType
    sub r1, r6, #0x160
    str r1, [r4, #0]  @ r0 - abort context
    str r7, [r4, #0x4]  @ r1 - user abort handler address
    str r1, [r4, #0x10] @ r4 - abort context
    str r1, [r4, #0x34] @ sp - above abort context
    mov r1, #0x10
    str r1, [r4, #0x44] @ Set the SPSR
    ldr r1, =userAbortBase
    ldr r1, [r1]
    str r1, [r4, #0x3C] @ Set the pc to the user abort dispatch
    mov r0, r4
LSetupAbortContext_return:
    mov r4, r0
    ldr r0, =handlersMutex
    mov r1, r8
    ldr r2, =ksceKernelCpuSpinLockIrqRestore
    blx r2
    mov r0, r4
    add sp, #0x24
    ldmia sp!, {r4 - r8, pc}

.global DabtExceptionHandler_lvl0
.global DabtExceptionHandler_lvl0_retAddr
DabtExceptionHandler_lvl0:
    stmdb sp!, {r2, r3}
    ldr r2, =userAbortBase
    ldr r2, [r2]
    lsr r3, lr, #0xc
    cmp r3, r2, lsr #0xc
    bne DabtExceptionHandler_lvl0_continue
    addeq sp, #0x8
    cmp r1, #0
    beq LReturnFromAbortHandler
    bne LReturnFromDefaultAbortHandler
DabtExceptionHandler_lvl0_continue:
    ldmia sp!, {r2, r3} @Reload r2 and r3 and prepare to branch to the lvl0 handler
    sub sp, #0x400
    str r0, [sp, #0]
    str r1, [sp, #0x4]
    ldr pc, [pc, #-0x4]
DabtExceptionHandler_lvl0_retAddr:
    .word 0x0

.global DabtExceptionHandler_lvl1
DabtExceptionHandler_lvl1:
    .word 0x0
    .word 0x0
    mov r4, r0
    mov r5, r1
    ldr r2, [r4, #0x44] @ SPSR
    and r2, #0x1F
    cmp r2, #0x10 @ USR mode
    bne LDabtExceptionHandler_next
    mov r0, r4
    mov r1, #0
    bl LSetupAbortContext
    cmp r0, #0
    bne ReturnFromException
LDabtExceptionHandler_next:
    mov r0, r4
    mov r1, r5
    ldr r2, DabtExceptionHandler_lvl1
    add r2, #8
    cpy pc, r2

.global PabtExceptionHandler_lvl1
PabtExceptionHandler_lvl1:
    .word 0x0
    .word 0x0
    mov r4, r0
    mov r5, r1
    ldr r2, [r4, #0x44] @ SPSR
    and r2, #0x1F
    cmp r2, #0x10 @ USR mode
    bne LPabtExceptionHandler_next
    mov r0, r4
    mov r1, #0x1
    bl LSetupAbortContext
    cmp r0, #0
    bne ReturnFromException
LPabtExceptionHandler_next:
    mov r0, r4
    mov r1, r5
    ldr r2, PabtExceptionHandler_lvl1
    add r2, #8
    cpy pc, r2
