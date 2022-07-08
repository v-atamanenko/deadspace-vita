ifdef VITASDK
PREFIX = $(VITASDK)/bin/
endif

ARCH ?= $(PREFIX)arm-vita-eabi
AS = $(ARCH)-as
AR = $(ARCH)-ar
RANLIB = $(ARCH)-ranlib

TARGETS = libSceLibcBridge_stub.a
TARGETS_WEAK = libSceLibcBridge_stub_weak.a

SceLibcBridge_OBJS = SceLibcBridge_SceLibc_sceLibcBridge___cxa_guard_acquire.o SceLibcBridge_SceLibc_sceLibcBridge___cxa_guard_release.o SceLibcBridge_SceLibc_sceLibcBridge_rand.o SceLibcBridge_SceLibc_sceLibcBridge_srand.o SceLibcBridge_SceLibc_sceLibcBridge_qsort.o SceLibcBridge_SceLibc_sceLibcBridge_sscanf.o SceLibcBridge_SceLibc_sceLibcBridge_calloc.o SceLibcBridge_SceLibc_sceLibcBridge_free.o SceLibcBridge_SceLibc_sceLibcBridge_malloc.o SceLibcBridge_SceLibc_sceLibcBridge_memalign.o SceLibcBridge_SceLibc_sceLibcBridge_realloc.o SceLibcBridge_SceLibc_sceLibcBridge_fclose.o SceLibcBridge_SceLibc_sceLibcBridge_feof.o SceLibcBridge_SceLibc_sceLibcBridge_ferror.o SceLibcBridge_SceLibc_sceLibcBridge_fopen.o SceLibcBridge_SceLibc_sceLibcBridge_fread.o SceLibcBridge_SceLibc_sceLibcBridge_fseek.o SceLibcBridge_SceLibc_sceLibcBridge_ftell.o SceLibcBridge_SceLibc_sceLibcBridge_fwrite.o SceLibcBridge_SceLibc_sceLibcBridge_fprintf.o SceLibcBridge_SceLibc_sceLibcBridge_fputc.o SceLibcBridge_SceLibc_sceLibcBridge_setjmp.o SceLibcBridge_SceLibc_sceLibcBridge_longjmp.o SceLibcBridge_SceLibc_sceLibcBridge_getc.o SceLibcBridge_SceLibc_sceLibcBridge_ungetc.o SceLibcBridge_SceLibstdcxx_sceLibcBridge__ZdaPv.o SceLibcBridge_SceLibstdcxx_sceLibcBridge__ZdlPv.o SceLibcBridge_SceLibstdcxx_sceLibcBridge__Znaj.o SceLibcBridge_SceLibstdcxx_sceLibcBridge__Znwj.o
SceLibcBridge_weak_OBJS = SceLibcBridge_SceLibc_sceLibcBridge___cxa_guard_acquire.wo SceLibcBridge_SceLibc_sceLibcBridge___cxa_guard_release.wo SceLibcBridge_SceLibc_sceLibcBridge_rand.wo SceLibcBridge_SceLibc_sceLibcBridge_srand.wo SceLibcBridge_SceLibc_sceLibcBridge_qsort.wo SceLibcBridge_SceLibc_sceLibcBridge_sscanf.wo SceLibcBridge_SceLibc_sceLibcBridge_calloc.wo SceLibcBridge_SceLibc_sceLibcBridge_free.wo SceLibcBridge_SceLibc_sceLibcBridge_malloc.wo SceLibcBridge_SceLibc_sceLibcBridge_memalign.wo SceLibcBridge_SceLibc_sceLibcBridge_realloc.wo SceLibcBridge_SceLibc_sceLibcBridge_fclose.wo SceLibcBridge_SceLibc_sceLibcBridge_feof.wo SceLibcBridge_SceLibc_sceLibcBridge_ferror.wo SceLibcBridge_SceLibc_sceLibcBridge_fopen.wo SceLibcBridge_SceLibc_sceLibcBridge_fread.wo SceLibcBridge_SceLibc_sceLibcBridge_fseek.wo SceLibcBridge_SceLibc_sceLibcBridge_ftell.wo SceLibcBridge_SceLibc_sceLibcBridge_fwrite.wo SceLibcBridge_SceLibc_sceLibcBridge_fprintf.wo SceLibcBridge_SceLibc_sceLibcBridge_fputc.wo SceLibcBridge_SceLibc_sceLibcBridge_setjmp.wo SceLibcBridge_SceLibc_sceLibcBridge_longjmp.wo SceLibcBridge_SceLibc_sceLibcBridge_getc.wo SceLibcBridge_SceLibc_sceLibcBridge_ungetc.wo SceLibcBridge_SceLibstdcxx_sceLibcBridge__ZdaPv.wo SceLibcBridge_SceLibstdcxx_sceLibcBridge__ZdlPv.wo SceLibcBridge_SceLibstdcxx_sceLibcBridge__Znaj.wo SceLibcBridge_SceLibstdcxx_sceLibcBridge__Znwj.wo
SceKernel_OBJS =
ALL_OBJS=

all: $(TARGETS) $(TARGETS_WEAK)

define LIBRARY_template
 $(1): $$($(1:lib%_stub.a=%)_OBJS)
 ALL_OBJS += $$($(1:lib%_stub.a=%)_OBJS)
endef
define LIBRARY_WEAK_template
 $(1): $$($(1:lib%_stub_weak.a=%)_weak_OBJS)
 ALL_OBJS += $$($(1:lib%_stub_weak.a=%)_weak_OBJS)
endef

$(foreach library,$(TARGETS),$(eval $(call LIBRARY_template,$(library))))
$(foreach library,$(TARGETS_WEAK),$(eval $(call LIBRARY_WEAK_template,$(library))))

install: $(TARGETS) $(TARGETS_WEAK)
	cp $(TARGETS) $(VITASDK)/arm-vita-eabi/lib
	cp $(TARGETS_WEAK) $(VITASDK)/arm-vita-eabi/lib

clean:
	rm -f $(TARGETS) $(TARGETS_WEAK) $(ALL_OBJS)

$(TARGETS) $(TARGETS_WEAK):
	$(AR) cru $@ $?
	$(RANLIB) $@

%.o: %.S
	$(AS) --defsym GEN_WEAK_EXPORTS=0 $< -o $@

%.wo: %.S
	$(AS) --defsym GEN_WEAK_EXPORTS=1 $< -o $@
