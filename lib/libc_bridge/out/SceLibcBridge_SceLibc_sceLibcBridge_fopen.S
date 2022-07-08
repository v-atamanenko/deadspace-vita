.arch armv7a

.section .vitalink.fstubs.SceLibc,"ax",%progbits

	.align 4
	.global sceLibcBridge_fopen
	.type sceLibcBridge_fopen, %function
sceLibcBridge_fopen:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xBE43BB07
	.word 0xFFFBE239
	.align 4

