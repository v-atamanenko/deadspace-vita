.arch armv7-a
.cpu cortex-a9
.fpu neon
.align 4
.syntax unified
.arm

UserAbortDispatch:
    mov r4, r0
    blx r1
    mov r0, r4
    mov r1, #0x0
    ldr r1, [r1] @ Trigger dabt to return to PL1

DefaultUserAbortHandler:
    mov r1, #0x1
    ldr r1, [r1] @ Trigger dabt to return to PL1
