.arch armv7a

.section .vitalink.fstubs.SceLibstdcxx,"ax",%progbits

	.align 4
	.global sceLibcBridge__ZdlPv
	.type sceLibcBridge__ZdlPv, %function
sceLibcBridge__ZdlPv:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xF7629240
	.word 0x72293931
	.align 4

