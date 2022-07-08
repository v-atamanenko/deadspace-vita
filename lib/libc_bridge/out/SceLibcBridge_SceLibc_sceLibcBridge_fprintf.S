.arch armv7a

.section .vitalink.fstubs.SceLibc,"ax",%progbits

	.align 4
	.global sceLibcBridge_fprintf
	.type sceLibcBridge_fprintf, %function
sceLibcBridge_fprintf:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xBE43BB07
	.word 0xE0C79764
	.align 4

