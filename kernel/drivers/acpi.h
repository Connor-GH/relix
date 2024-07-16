#pragma once
#include <stdint.h>
#define x86_64_BIT_FULLY_READY 0 /* change when ready */
#define ACPI_VERSION_2_0 0
// References: ACPI 5.0 Errata A
// http://acpi.info/spec.htm

// 5.2.5.3
#define SIG_RSDP "RSD PTR "
struct acpi_rsdp {
  uint8_t signature[8];
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t revision;
  uint32_t rsdt_addr_phys;
#if ACPI_VERSION_2_0
  uint32_t length;
#if x86_64_BIT_FULLY_READY
	uint64_t xsdt_addr_phys;
#endif
  uint8_t xchecksum;
  uint8_t reserved[3];
#endif
} __attribute__((__packed__));

// 5.2.6
struct acpi_desc_header {
  uint8_t signature[4]; // "APIC"
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
  uint32_t entry[];
} __attribute__((__packed__));

#define TYPE_LAPIC 0
#define TYPE_IOAPIC 1
#define TYPE_INT_SRC_OVERRIDE 2
#define TYPE_NMI_INT_SRC 3
#define TYPE_LAPIC_NMI 4

// 5.2.12 Multiple APIC Description Table (MADT)
#define SIG_MADT "APIC"
struct acpi_madt {
  struct acpi_desc_header header;
  uint32_t lapic_addr_phys;
  uint32_t flags; // 1 = legacy pics installed.
  uint8_t table[];
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

// signature: "FACP"
struct acpi_fadt {
	struct acpi_desc_header header;
	uint32_t firmware_ctrl_phys_addr;
	uint32_t dsdt_phs_addr;
	uint8_t reserved;
	uint8_t preferred_PM_profile; // power management
	uint16_t sci_int;
	uint32_t smi_cmd; // SMI command port
	// value to write to smi_cmd to give up control over the ACPI registers
	uint8_t acpi_enable;
	uint8_t acpi_disable; // opposite of above
	uint8_t s4bios_req; // val to write to smi_cmd to enter S4BIOS mode
	// if != 0, contains the val OSPM writes to smi to control cpu performance state
	uint8_t pstate_cnt;
	uint32_t pm1a_evt_blk; // port address of pm1a event register
	uint32_t pm1b_evt_blk; // port address of pm1b event register
	uint32_t pm1a_cnt_blk; // port address of pm1a control register
	uint32_t pm1b_cnt_blk; // port address of pm1b control register
	uint32_t pm2_cnt_blk; // port address of pm2 control register
	uint32_t pm_tmr_blk; // power management timer crb
	uint32_t gpe0_blk; // GPE0 register
	uint32_t gpe1_blk; // GPE1 register
	uint8_t pm1_evt_len;
	uint8_t pm1_cnt_len;
	uint8_t pm2_cnt_len;
	uint8_t mp_tmr_len;
	uint8_t gpe0_blk_len;
	uint8_t gpe1_blk_len;
	uint8_t gpe1_base;
	uint8_t cst_cnt; // C cstates changed
	uint16_t p_lvl2_lat; // latency to exit C2 state
	uint16_t p_lvl3_lat; // latency to exit C3 state
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alrm;
	uint8_t month_alarm;
	uint8_t century; // century register in CMOS if not zero
	uint16_t iapc_boot_arch_flags;
	uint8_t reserved1;
	uint32_t feature_flags;
	uint8_t reset_reg[12];
	uint8_t reset_value;
	uint16_t arm_boot_arch;
	uint8_t fadt_minor_ver;
	uint8_t X_fw_ctrl[8];
	uint8_t X_dsdt[8];
	uint8_t X_pm1a_evt_blk[12];
	uint8_t X_pm1b_evt_blk[12];
	uint8_t X_pm1a_cnt_blk[12];
	uint8_t X_pm1b_cnt_blk[12];
	uint8_t X_pm2_cnt_blk[12];
	uint8_t X_pm_tmr_blk[12];
	uint8_t X_gpe0_blk[12];
	uint8_t X_gpe1_blk[12];
	uint8_t sleep_ctrl_reg[12];
	uint8_t sleep_status_reg[12];
	uint8_t hypervisor_vender_id[8];
} __attribute__((__packed__));
_Static_assert(sizeof(struct acpi_fadt) == 276, "ACPI FADT Struct malformed");

int
acpiinit(void);

