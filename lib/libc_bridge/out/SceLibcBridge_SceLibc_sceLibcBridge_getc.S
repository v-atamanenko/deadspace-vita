.arch armv7a

.section .vitalink.fstubs.SceLibc,"ax",%progbits

	.align 4
	.global sceLibcBridge_getc
	.type sceLibcBridge_getc, %function
sceLibcBridge_getc:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xBE43BB07
	.word 0x4BD5212E
	.align 4

