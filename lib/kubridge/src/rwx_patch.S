.arch armv7-a
.cpu cortex-a9
.fpu neon
.align 4
.syntax unified
.arm
.text

.global MemBlockTypeToL1PTE_UserRWX
.global MemBlockTypeToL2PTESmallPage_UserRWX
.global MemBlockTypeToL2PTELargePage_UserRWX

MemBlockTypeToL1PTE_UserRWX:
    cmp    r0, #0x70
    beq    MemBlockTypeToL1PTE_UserRWX_RWX
    cmp    r0, #0x0
    beq    MemBlockTypeToL1PTE_UserRWX_NONE
    b      MemBlockTypeToL1PTE_UserRWX_kpanic
MemBlockTypeToL1PTE_UserRWX_RWX:
    orr    r3, r3, #0xC00 @ User and Priviledged Read-Write-Execute
MemBlockTypeToL1PTE_UserRWX_NONE:
    lsls   r1, r1, #0x5
    and    r0, r1, #0x1E0
    orr    r0, r3, r0
    pop    {r3, pc}
MemBlockTypeToL1PTE_UserRWX_kpanic:
    mov    r1, lr
    ldr    r0, =MemBlockTypeToL1PTE_DebugContext
    ldr    r0, [r0]
    bl    ksceDebugPrintKernelPanic

MemBlockTypeToL2PTESmallPage_UserRWX:
    cmp    r0, #0x70
    beq    MemBlockTypeToL2PTESmallPage_UserRWX_RWX
    cmp    r0, #0x0
    beq    MemBlockTypeToL2PTESmallPage_UserRWX_NONE
    b      MemBlockTypeToL2PTESmallPage_UserRWX_kpanic
MemBlockTypeToL2PTESmallPage_UserRWX_RWX:
    orr    r0, r2, #0x30 @ User and Priviledged Read-Write-Execute
    pop    {r4, pc}
MemBlockTypeToL2PTESmallPage_UserRWX_NONE:
    mov    r0, r2
    pop    {r4, pc}
MemBlockTypeToL2PTESmallPage_UserRWX_kpanic:
    mov    r1, lr
    ldr    r0, =MemBlockTypeToL2PTESmallPage_DebugContext
    ldr    r0, [r0]
    bl    ksceDebugPrintKernelPanic

MemBlockTypeToL2PTELargePage_UserRWX:
    cmp    r2, #0x70
    beq    MemBlockTypeToL2PTELargePage_UserRWX_RWX
    cmp    r2, #0x0
    beq    MemBlockTypeToL2PTELargePage_UserRWX_NONE
    b      MemBlockTypeToL2PTELargePage_UserRWX_kpanic
MemBlockTypeToL2PTELargePage_UserRWX_RWX:
    orr    r6, r6, #0x30 @ User and Priviledged Read-Write-Execute
MemBlockTypeToL2PTELargePage_UserRWX_NONE:
    ldr    r12, =sceKernelMMUMapLargePages_cont
    ldr    r12, [r12]
    bx     r12
MemBlockTypeToL2PTELargePage_UserRWX_kpanic:
    mov    r1, lr
    ldr    r0, =sceKernelMMUMapLargePages_DebugContext
    ldr    r0, [r0]
    bl    ksceDebugPrintKernelPanic