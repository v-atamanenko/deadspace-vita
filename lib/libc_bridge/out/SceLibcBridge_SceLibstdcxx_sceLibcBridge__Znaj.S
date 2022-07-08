.arch armv7a

.section .vitalink.fstubs.SceLibstdcxx,"ax",%progbits

	.align 4
	.global sceLibcBridge__Znaj
	.type sceLibcBridge__Znaj, %function
sceLibcBridge__Znaj:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xF7629240
	.word 0xE7FB2BF4
	.align 4

