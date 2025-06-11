#include "ahci.h"
#include "console.h"
#include "kalloc.h"
#include "kernel_assert.h"
#include "macros.h"
#include "memlayout.h"
#include <pci.h>
#include <stdint.h>
#include <string.h>

#define SATA_SIG_ATA 0x00000101 // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101 // Enclosure management bridge
#define SATA_SIG_PM 0x96690101 // Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SATAPI 4
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3

#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1

#define HBA_PxCMD_CR (1 << 15) /* CR - Command list Running */
#define HBA_PxCMD_FR (1 << 14) /* FR - FIS receive Running */
#define HBA_PxCMD_FRE (1 << 4) /* FRE - FIS Receive Enable */
#define HBA_PxCMD_SUD (1 << 1) /* SUD - Spin-Up Device */
#define HBA_PxCMD_ST (1 << 0) /* ST - Start (command processing) */

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define HBA_PxIS_TFES (1 << 30) /* TFES - Task File Error Status */

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35

#define ATA_CMD_IDENTIFY_PIO 0xEC

static HBAMem *abar;

// Start command engine
void
start_cmd(HBAPort *port)
{
	// Wait until Cr (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR)
		;

	// Set Fre (bit4) and St (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void
stop_cmd(HBAPort *port)
{
	// Clear St (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear Fre (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until Fr (bit14), Cr (bit15) are cleared
	while (1) {
		if (port->cmd & HBA_PxCMD_FR) {
			continue;
		}
		if (port->cmd & HBA_PxCMD_CR) {
			continue;
		}
		break;
	}
}

// Find a free command list slot
int
find_cmdslot(HBAPort *port)
{
	// If not set in Sact and Ci, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < 32; i++) {
		if ((slots & (1 << i)) == 0) {
			return i;
		}
	}
	uart_printf("Cannot find free command list entry\n");
	return -1;
}

void
ahci_init(uint32_t abar_)
{
	pr_debug_file("Found AHCI device at %#x\n", abar_);
	abar = (HBAMem *)IO2V((uintptr_t)abar_);
	probe_port(abar);
}

static void
ata_clear_pending_interrupts(HBAPort *port)
{
	// Clear pending interrupt bits
	port->is = 0xFfffffff;
}

static HBACmdHeader *
ata_setup_command_header(HBAPort *port, uint32_t count, int slot, bool writing)
{
	HBACmdHeader *cmdheader =
		(HBACmdHeader *)((uintptr_t)P2V((uintptr_t)port->clb));

	cmdheader += slot;
	// Command FIS length
	cmdheader->cfl = sizeof(FISRegH2D) / sizeof(uint32_t);

	// w=1 is write, w=0 is read.
	if (writing) {
		cmdheader->w = 1;
		cmdheader->c = 1; // Read from device
		// cmdheader->p = 1; // Read from device
	} else {
		cmdheader->w = 0;
	}

	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1; // PRDT entries count

	return cmdheader;
}

static HBACmdTbl *
ata_setup_command_table(HBACmdHeader *cmdheader, uint16_t *buf, uint16_t *count)
{
	HBACmdTbl *cmdtbl = (HBACmdTbl *)P2V((uintptr_t)cmdheader->ctba |
	                                     ((uintptr_t)cmdheader->ctbau << 32));

	memset(cmdtbl, 0,
	       sizeof(HBACmdTbl) + (cmdheader->prdtl - 1) * sizeof(HBAPRDTEntry));

	int i = 0;
	// 8K bytes (16 sectors) per PRDT
	for (; i < cmdheader->prdtl - 1; i++) {
		cmdtbl->prdt_entry[i].dba = V2P((uintptr_t)buf) & 0xFFFFFFFF;
		cmdtbl->prdt_entry[i].dbau = V2P((uintptr_t)buf) >> 32 & 0xFFFFFFFF;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1; // 8K bytes (this value should
		                                          // always be set to 1 less than
		                                          // the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024; // 4K words
		*count -= 16; // 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = V2P((uintptr_t)buf) & 0xFFFFFFFF;
	cmdtbl->prdt_entry[i].dbau = V2P((uintptr_t)buf) >> 32 & 0xFFFFFFFF;

	cmdtbl->prdt_entry[i].dbc = (*count << 9) - 1; // 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1; // maybe 0? ???

	return cmdtbl;
}

static FISRegH2D *
ata_setup_command_fis(HBACmdTbl *cmdtbl, uint64_t start, uint16_t count,
                      uint8_t command)
{
	uint32_t startl = start & 0xFFFFFFFF;
	uint32_t starth = (start >> 32) & 0xFFFFFFFF;
	// Setup command
	FISRegH2D *cmdfis = (FISRegH2D *)(&cmdtbl->cfis);

	cmdfis->fis_type = FISTypeRegH2D;
	cmdfis->c = 1; // Command
	cmdfis->command = command;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl >> 8);
	cmdfis->lba2 = (uint8_t)(startl >> 16);
	cmdfis->device = 1 << 6; // Lba mode

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth >> 8);

	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	return cmdfis;
}

static bool
wait_on_disk(HBAPort *port, int slot)
{
	int spin = 0;
	// The below loop waits until the port is no longer busy before issuing a new
	// command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		uart_printf("Port has hung\n");
		return false;
	}

	port->ci = 1 << slot;

	while (1) {
		// In some longer duration reads, it may be helpful to spin on the Dps bit
		// in the PxIs port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0) {
			break;
		}

		// Task file error status
		if (port->is & HBA_PxIS_TFES) {
			uart_printf("Read disk error\n");
			return false;
		}
	}
	// Check again
	if (port->is & HBA_PxIS_TFES) {
		uart_printf("Read disk error\n");
		return false;
	}
	return true;
}

bool
read_port(HBAPort *port, uint64_t start, uint16_t count, uint16_t *buf)
{
	ata_clear_pending_interrupts(port);
	// Spin lock timeout counter
	int spin = 0;
	int slot = find_cmdslot(port);
	if (slot == -1) {
		return false;
	}

	HBACmdHeader *cmdheader = ata_setup_command_header(port, count, slot, false);

	HBACmdTbl *cmdtbl = ata_setup_command_table(cmdheader, buf, &count);

	FISRegH2D *cmdfis =
		ata_setup_command_fis(cmdtbl, start, count, ATA_CMD_READ_DMA_EX);

	return wait_on_disk(port, slot);
}

bool
write_port(HBAPort *port, uint64_t start, uint16_t count, uint16_t *buf)
{
	ata_clear_pending_interrupts(port);
	// Spin lock timeout counter
	int spin = 0;
	int slot = find_cmdslot(port);
	if (slot == -1) {
		return false;
	}

	HBACmdHeader *cmdheader = ata_setup_command_header(port, count, slot, true);

	HBACmdTbl *cmdtbl = ata_setup_command_table(cmdheader, buf, &count);

	FISRegH2D *cmdfis =
		ata_setup_command_fis(cmdtbl, start, count, ATA_CMD_WRITE_DMA_EX);

	return wait_on_disk(port, slot);
}

bool
disk_identify(HBAPort *port, IdentifyDevicePIO *buf)
{
	ata_clear_pending_interrupts(port);
	// Spin lock timeout counter
	int spin = 0;
	uint16_t count = 1;
	uint64_t start = 0;
	int slot = find_cmdslot(port);
	if (slot == -1) {
		return false;
	}

	HBACmdHeader *cmdheader = ata_setup_command_header(port, count, slot, false);

	cmdheader->prdtl = 1;
	HBACmdTbl *cmdtbl =
		ata_setup_command_table(cmdheader, (uint16_t *)buf, &count);

	FISRegH2D *cmdfis =
		ata_setup_command_fis(cmdtbl, start, count, ATA_CMD_IDENTIFY_PIO);

	return wait_on_disk(port, slot);
}

// Check device type
static int
check_type(HBAPort *port)
{
	uint32_t ssts = port->ssts;
	uint8_t ipm = (ssts >> 8) & 0xF;
	uint8_t det = ssts & 0xF;

	if (det != HBA_PORT_DET_PRESENT) { // Check drive status
		return AHCI_DEV_NULL;
	}
	if (ipm != HBA_PORT_IPM_ACTIVE) {
		return AHCI_DEV_NULL;
	}
	switch (port->sig) {
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
	return 0;
}
/*
 Code Reference : Osdev
 */
void
port_rebase(HBAPort *port, int portno)
{
	stop_cmd(port); // Stop command engine

	/*
	 * Todo:
	 * Implement aligned_alloc and use it here instead.
	 */

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	uintptr_t clb_mem = V2P(kpage_alloc());
	port->clb = clb_mem >> 0;
	kernel_assert(port->clb % 1024 == 0);
	port->clbu = clb_mem >> 32;

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = V2P(kpage_alloc());
	port->fbu = 0;
	kernel_assert(port->fbu % 256 == 0);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	HBACmdHeader *cmdheader =
		(HBACmdHeader *)((uintptr_t)P2V((uintptr_t)port->clb));
	kernel_assert((uintptr_t)cmdheader % 128 == 0);
	memset(cmdheader, 0, sizeof(*cmdheader) * 32);
	for (int i = 0; i < 32; i++) {
		// 8 prdt entries per command table
		cmdheader[i].prdtl = 8;
		// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = V2P(kpage_alloc());
		cmdheader[i].ctbau = 0;
		kernel_assert(cmdheader[i].ctba % 256 == 0);
	}

	start_cmd(port); // Start command engine
}

static void
ata_string_to_cstring(char *buf, size_t size)
{
	for (size_t i = 0; i < size - 1; i += 2) {
		swap(buf[i], buf[i + 1]);
	}
}

static void
ata_parse_identify_device_info(IdentifyDevicePIO info)
{
	// Assures that ATA_CMD_READ_DMA_EX and ATA_CMD_WRITE_DMA_EX are aupported.
	kernel_assert(info.commands_feature_sets_supported2 & (1 << 10));

	char model_num_buf[41];
	memcpy(model_num_buf, info.model_number, 40);
	ata_string_to_cstring(model_num_buf, 40);
	model_num_buf[40] = '\0';

	uart_printf("SATA disk name: %s\n", model_num_buf);

	char additional_product_id[9];
	memcpy(additional_product_id, info.additional_product_id, 8);
	ata_string_to_cstring(additional_product_id, 8);
	additional_product_id[8] = '\0';

	// This field is optional.
	if (additional_product_id[0] != '\0') {
		pr_debug_file("Additional id: %s\n", additional_product_id);
	}

	// Trivial disk info
	pr_debug_file("is_ata_device=(%s), %d:1 logical:physical sectors\n",
	              BOOL_STRING(IS_ATA_DEVICE(info.general_configuration)),
	              1 << (info.physical_or_logical_sector_size & 0xF));

#if defined(SATA_MAJOR_AND_MINOR) && SATA_MAJOR_AND_MINOR
	pr_debug_file("Transport Major: ");
	switch (info.transport_major_version_number & (0b1111 << 12)) {
	// Parallel
	case 0x0: {
		if (info.transport_major_version_number & (1 << 0)) {
			pr_debug("ATA8-APT");
		} else if (info.transport_major_version_number & (1 << 1)) {
			pr_debug("ATA/ATAPI-7");
		} else {
			pr_debug("(unknown PATA %x)",
			         info.transport_major_version_number & ~(0b1111 << 12));
		}
		break;
	}
	// Serial
	case 0x1: {
		if (info.transport_major_version_number & (1 << 0)) {
			pr_debug("ATA8-AST");
		} else if (info.transport_major_version_number & (1 << 1)) {
			pr_debug("SATA 1.0a");
		} else if (info.transport_major_version_number & (1 << 2)) {
			pr_debug("SATA II: Extensions");
		} else if (info.transport_major_version_number & (1 << 3)) {
			pr_debug("SATA 2.5");
		} else if (info.transport_major_version_number & (1 << 4)) {
			pr_debug("SATA 2.6");
		} else if (info.transport_major_version_number & (1 << 5)) {
			pr_debug("SATA 3.0");
		} else if (info.transport_major_version_number & (1 << 6)) {
			pr_debug("SATA 3.1");
		} else if (info.transport_major_version_number & (1 << 7)) {
			pr_debug("SATA 3.2");
		} else if (info.transport_major_version_number & (1 << 8)) {
			pr_debug("SATA 3.3");
		} else {
			pr_debug("(unknown SATA)");
		}
		break;
	}
	// PCIe
	case 0xE: {
		break;
	}
	default: {
		pr_debug("Unknown");
		break;
	}
	}
	pr_debug("\n");

	pr_debug_file("ACS Major: ");
	if (info.major_version_number & (1 << 11)) {
		pr_debug_file("ACS-4");
	} else if (info.major_version_number & (1 << 10)) {
		pr_debug_file("ACS-3");
	} else if (info.major_version_number & (1 << 9)) {
		pr_debug_file("ACS-2");
	} else if (info.major_version_number & (1 << 8)) {
		pr_debug_file("ATA8-ACS");
	} else {
		pr_debug_file("(unknown)");
	}
	pr_debug_file("\n");

	pr_debug_file("ACS Minor: ");
	switch (info.minor_version_number) {
	case 0xFFFF:
	case 0:
		pr_debug_file("(not reported)");
		break;
	case 0x1F:
		pr_debug_file("ACS-3 revision 3b");
		break;
	case 0x27:
		pr_debug_file("ATA8-ACS version 3c");
		break;
	case 0x28:
		pr_debug_file("ATA8-ACS version 6");
		break;
	case 0x29:
		pr_debug_file("ATA8-ACS version 4");
		break;
	case 0x31:
		pr_debug_file("ACS-2 revision 2");
		break;
	case 0x33:
		pr_debug_file("ATA8-ACS version 3e");
		break;
	case 0x39:
		pr_debug_file("ATA8-ACS version 4c");
		break;
	case 0x42:
		pr_debug_file("ATA8-ACS version 3f");
		break;
	case 0x52:
		pr_debug_file("ATA8-ACS version 3b");
		break;
	case 0x5e:
		pr_debug_file("ACS-4 revision 5");
		break;
	case 0x6d:
		pr_debug_file("ACS-3 revision 5");
		break;
	case 0x82:
		pr_debug_file("ACS-2 ANSI INCITS 482-2012");
		break;
	case 0x107:
		pr_debug_file("ATA8-ACS version 2d");
		break;
	case 0x10a:
		pr_debug_file("ACS-3 ANSI INCITS 522-2014");
		break;
	case 0x110:
		pr_debug_file("ACS-2 revision 3");
		break;
	case 0x11b:
		pr_debug_file("ACS-3 revision 4");
		break;
	default:
		pr_debug_file("(unknown)");
		break;
	}
	pr_debug_file("\n");
#endif
}

void
probe_port(HBAMem *abar_)
{
	// Search disk in implemented ports
	uint32_t pi = abar_->pi;
	for (unsigned int i = 0; i < 32; i++) {
		if ((pi & (1U << i)) == (1U << i)) {
			int dt = check_type(&abar_->ports[i]);
			if (dt == AHCI_DEV_SATA) {
				uart_printf("Sata drive found at port %d\n", i);
				port_rebase(&abar_->ports[i], i);
				IdentifyDevicePIO pio = { 0 };
				disk_identify(&abar->ports[i], &pio);
				ata_parse_identify_device_info(pio);

#if __KERNEL_DEBUG__
				uint16_t buf[256] = { 1, 2, 3, 0 };
				pr_debug_file("Reading first 512 bytes...\n");
				read_port(&abar_->ports[i], 0, 1, buf);

				for (int j = 0; j < 256; j++) {
					pr_debug("%#x ", buf[j]);
				}
				pr_debug("\n");
#endif
			} else if (dt == AHCI_DEV_SATAPI) {
				uart_printf("Satapi drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_SEMB) {
				uart_printf("Semb drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_PM) {
				uart_printf("Pm drive found at port %d\n", i);
			} else {
				// No drive found here, we should not log it.
			}
		}
	}
}
