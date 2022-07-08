.arch armv7a

.section .vitalink.fstubs.SceLibstdcxx,"ax",%progbits

	.align 4
	.global sceLibcBridge__Znwj
	.type sceLibcBridge__Znwj, %function
sceLibcBridge__Znwj:
.if GEN_WEAK_EXPORTS
	.word 0x00000008
.else
	.word 0x00000000
.endif //GEN_WEAK_EXPORTS
	.word 0xF7629240
	.word 0xF99ED5AC
	.align 4

