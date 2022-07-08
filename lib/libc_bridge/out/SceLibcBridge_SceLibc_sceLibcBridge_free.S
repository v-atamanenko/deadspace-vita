.arch armv7a

.section .vitalink.fstubs.SceLibc,"ax",%progbits

	.align 4
	.global sceLibcBridge_free
	.type sceLibcBridge_free, %function
sceLibcBridge_free:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xBE43BB07
	.word 0x5B9BB802
	.align 4

