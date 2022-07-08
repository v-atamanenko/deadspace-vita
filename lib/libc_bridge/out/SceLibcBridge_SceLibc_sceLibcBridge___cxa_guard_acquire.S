.arch armv7a

.section .vitalink.fstubs.SceLibc,"ax",%progbits

	.align 4
	.global sceLibcBridge___cxa_guard_acquire
	.type sceLibcBridge___cxa_guard_acquire, %function
sceLibcBridge___cxa_guard_acquire:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xBE43BB07
	.word 0xD0310E31
	.align 4

