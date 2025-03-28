#pragma once
#include <stdint.h>
/*
 * Same as the PCI spec for the common header,
 * except stripped of pointers and information
 * otherwise useless to userspace or malicious
 * in the context of userspace.
 *
 * "function" and "bus" were also added for
 * logging purposes.
 */
struct pci_conf {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint8_t revision_id;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t base_class;
	uint8_t cacheline_size;
	uint8_t header_type;
	uint8_t function;
	uint8_t device;
	uint8_t bus;
};

// Kernel implementation details
#if __KERNEL__
#include <stddef.h>
struct FatPointerArray_pci_conf {
    struct pci_conf *ptr;
    size_t len;
};
extern void
pci_init(void);
extern struct FatPointerArray_pci_conf
pci_get_conf(void);
#endif
