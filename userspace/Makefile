UDIR = userspace

USER_EXTRAS = -D__RELIX_USER__

# CFLAGS already includes all floating point off so
# we opt into what we want here.
UCFLAGS := -fno-lto -m80387 -msse $(USER_EXTRAS) -I$(UDIR)/include  $(IVARS)

include $(UDIR)/libc/Makefile
include $(UDIR)/rust/Makefile
include $(UDIR)/programs/Makefile
