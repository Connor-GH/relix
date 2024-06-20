#pragma once
#include <stdint.h>
#define x86_64_BIT_FULLY_READY 0 /* change when ready */
// References: ACPI 5.0 Errata A
// http://acpi.info/spec.htm

// 5.2.5.3
#define SIG_RDSP "RSD PTR "
struct acpi_rdsp {
  uint8_t signature[8];
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t revision;
  uint32_t rsdt_addr_phys;
  uint32_t length;
#if x86_64_BIT_FULLY_READY
	uint64_t xsdt_addr_phys;
#endif
  uint8_t xchecksum;
  uint8_t reserved[3];
} __attribute__((__packed__));

// 5.2.6
struct acpi_desc_header {
  uint8_t signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t oem_tableid[8];
  uint32_t oem_revision;
  uint8_t creator_id[4];
  uint32_t creator_revision;
} __attribute__((__packed__));

// 5.2.7
struct acpi_rsdt {
  struct acpi_desc_header header;
  uint32_t entry[0];
} __attribute__((__packed__));

#define TYPE_LAPIC 0
#define TYPE_IOAPIC 1
#define TYPE_INT_SRC_OVERRIDE 2
#define TYPE_NMI_INT_SRC 3
#define TYPE_LAPIC_NMI 4

// 5.2.12 Multiple APIC Description Table
#define SIG_MADT "APIC"
struct acpi_madt {
  struct acpi_desc_header header;
  uint32_t lapic_addr_phys;
  uint32_t flags;
  uint8_t table[0];
} __attribute__((__packed__));

// 5.2.12.2
#define APIC_LAPIC_ENABLED 1
struct madt_lapic {
  uint8_t type;
  uint8_t length;
  uint8_t acpi_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((__packed__));

// 5.2.12.3
struct madt_ioapic {
  uint8_t type;
  uint8_t length;
  uint8_t id;
  uint8_t reserved;
  uint32_t addr;
  uint32_t interrupt_base;
} __attribute__((__packed__));


int
acpiinit(void);

