UDIR = userspace

USER_EXTRAS = -D__RELIX_USER__

# CFLAGS already includes all floating point off so
# we opt into what we want here.
UCFLAGS := -fno-lto -m80387 -msse $(USER_EXTRAS) -I$(UDIR)/include  $(IVARS)
ULDFLAGS := $(LINKER_FLAGS) -Wl,-z,noexecstack,-O1 -Wl,-N,-e,_start,--section-start=.text=0x1000

ifneq ($(RELEASE),)
	UCFLAGS += -flto -O3
else
	UCFLAGS += -D__RELIX_MEMORY_GUARDS__
endif

LIBM_STATIC := $(BIN)/libm.a

libm_clean:
	$(MAKE) -C $(UDIR)/openlibm clean
	rm -f include/math.h
	rm -f include/complex.h
	rm -f include/fenv.h

$(LIBM_STATIC): include/math.h include/fenv.h include/complex.h $(UDIR)/include/openlibm_defs.h
	env CFLAGS="$(CFLAGS) $(UCFLAGS) -Wno-error -I$(realpath $(UDIR)/../include)" LDFLAGS="$(LDFLAGS)" $(MAKE) -C $(UDIR)/openlibm libopenlibm.a -j$(nproc)
	cp $(UDIR)/openlibm/libopenlibm.a $(BIN)/libm.a

$(UDIR)/include/openlibm_defs.h:
	cp $(UDIR)/openlibm/include/openlibm_defs.h $@

include/math.h:
	cp $(UDIR)/openlibm/include/openlibm_math.h $@

include/complex.h:
	cp $(UDIR)/openlibm/include/openlibm_complex.h $@

include/fenv.h:
	cp $(UDIR)/openlibm/include/openlibm_fenv.h $@

include $(UDIR)/libc/Makefile
include $(UDIR)/rust/Makefile
include ports/Makefile
include $(UDIR)/programs/Makefile
