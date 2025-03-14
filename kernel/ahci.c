#include <pci.h>
#include <stdint.h>
#include "console.h"
#include "ahci.h"
#include "memlayout.h"
#include <string.h>
#include "kernel_assert.h"
#include "kalloc.h"

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
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define HBA_PxIS_TFES (1 << 30) /* TFES - Task File Error Status */
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
char test[20] = "6317065029";
static void
start_cmd(HBA_PORT *port);
static void
stop_cmd(HBA_PORT *port);
static int
find_cmdslot(HBA_PORT *port);
static HBA_MEM *abar;
static char fs_buf[1024];
// Start command engine
void
start_cmd(HBA_PORT *port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR)
		;

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void
stop_cmd(HBA_PORT *port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1) {
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

// Find a free command list slot
int
find_cmdslot(HBA_PORT *port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < 32; i++) {
		if ((slots & (1 << i)) == 0)
			return i;
	}
	uart_cprintf("Cannot find free command list entry\n");
	return -1;
}

void
ahci_init(uint32_t abar_)
{
	uart_cprintf("Enabled AHCI\n");
	abar = (HBA_MEM *)IO2V((uintptr_t)abar_);
	// We don't want SATA interrupts yet.
	//ioapicenable(IRQ_SATA, 0);
	probe_port(abar);
}

static void
ata_clear_pending_interrupts(HBA_PORT *port)
{
	// Clear pending interrupt bits
	port->is = 0xFFFFFFFF;
}

static HBA_CMD_HEADER *
ata_setup_command_header(HBA_PORT *port, uint32_t count, int slot, bool writing)
{
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)((uintptr_t)P2V((uintptr_t)port->clb));

	cmdheader += slot;
	// Command FIS length
	cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);

	// w=1 is write, w=0 is read.
	if (writing) {
		cmdheader->w = 1;
		cmdheader->c = 1; // Read from device
		cmdheader->p = 1; // Read from device
	} else {
		cmdheader->w = 0;
	}

	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1; // PRDT entries count

	return cmdheader;
}

static HBA_CMD_TBL *
ata_setup_command_table(HBA_CMD_HEADER *cmdheader, uint16_t *buf, uint16_t *count)
{
	HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)P2V((uintptr_t)cmdheader->ctba | ((uintptr_t)cmdheader->ctbau << 32));

	memset(cmdtbl, 0,
				 sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

	int i = 0;
	// 8K bytes (16 sectors) per PRDT
	for (; i < cmdheader->prdtl - 1; i++) {
		cmdtbl->prdt_entry[i].dba = V2P((uintptr_t)buf) & 0xFFFFFFFF;
		cmdtbl->prdt_entry[i].dbau = V2P((uintptr_t)buf) >> 32 & 0xFFFFFFFF;
		cmdtbl->prdt_entry[i].dbc =
			8 * 1024 -
			1; // 8K bytes (this value should always be set to 1 less than the actual value)
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

static FIS_REG_H2D *
ata_setup_command_fis(HBA_CMD_TBL *cmdtbl, uint64_t start, uint16_t count, uint8_t command)
{
	uint32_t startl = start & 0xFFFFFFFF;
	uint32_t starth = (start >> 32) & 0xFFFFFFFF;
	// Setup command
	FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1; // Command
	//cmdfis->command = ATA_CMD_READ_DMA_EX;
	cmdfis->command = command;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl >> 8);
	cmdfis->lba2 = (uint8_t)(startl >> 16);
	cmdfis->device = 1 << 6; // LBA mode

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth >> 8);

	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	return cmdfis;
}

static bool
wait_on_disk(HBA_PORT *port, int slot)
{
	int spin = 0;
	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		uart_cprintf("Port has hung\n");
		return false;
	}

	port->ci = 1 << slot;

	while (1) {

		// In some longer duration reads, it may be helpful to spin on the DPS bit
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0)
			break;

		// Task file error status
		if (port->is & HBA_PxIS_TFES)
		{
			uart_cprintf("Read disk error\n");
			return false;
		}
	}
	// Check again
	if (port->is & HBA_PxIS_TFES) {
		uart_cprintf("Read disk error\n");
		return false;
	}
	return true;
}

bool
read_port(HBA_PORT *port, uint64_t start, uint16_t count,
		 uint16_t *buf)
{
	ata_clear_pending_interrupts(port);
	// Spin lock timeout counter
	int spin = 0;
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;

	HBA_CMD_HEADER *cmdheader = ata_setup_command_header(port, count, slot, false);

	HBA_CMD_TBL *cmdtbl = ata_setup_command_table(cmdheader, buf, &count);

	FIS_REG_H2D *cmdfis = ata_setup_command_fis(cmdtbl, start, count, ATA_CMD_READ_DMA_EX);

	return wait_on_disk(port, slot);
}

bool
write_port(HBA_PORT *port, uint64_t start, uint16_t count,
		 uint16_t *buf)
{
	ata_clear_pending_interrupts(port);
	// Spin lock timeout counter
	int spin = 0;
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;

	HBA_CMD_HEADER *cmdheader = ata_setup_command_header(port, count, slot, true);

	HBA_CMD_TBL *cmdtbl = ata_setup_command_table(cmdheader, buf, &count);

	FIS_REG_H2D *cmdfis = ata_setup_command_fis(cmdtbl, start, count, ATA_CMD_WRITE_DMA_EX);

	return wait_on_disk(port, slot);
}

// Check device type
static int
check_type(HBA_PORT *port)
{
	uint32_t ssts = port->ssts;
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT) // Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;
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
 Code Reference : OSDEV
 */
void
port_rebase(HBA_PORT *port, int portno)
{
	stop_cmd(port); // Stop command engine


	/*
	 * TODO:
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
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)((uintptr_t)P2V((uintptr_t)port->clb));
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

void
probe_port(HBA_MEM *abar_)
{
	// Search disk in implemented ports
	uint32_t pi = abar_->pi;
	for (int i = 0; i < 32; i++) {
		if ((pi & (1 << i)) == (1 << i)) {
			int dt = check_type(&abar_->ports[i]);
			if (dt == AHCI_DEV_SATA) {
				uart_cprintf("SATA drive found at port %d\n", i);
				port_rebase(&abar_->ports[i], i);
				uint16_t buf[256] = {1, 2, 3, 0};
				read_port(&abar_->ports[i], 0, 1, buf);
				for (int j = 0; j < 256; j++) {
					uart_cprintf("%#x ", buf[j]);
				}
				uart_cprintf("\n");
				for (int j = 0; j < 256; j++) {
					buf[j] = 2*j;
				}
				uart_cprintf("Writing...\n");
				write_port(&abar->ports[i], 0, 1, buf);
				uart_cprintf("Wrote.\n");
				for (int j = 0; j < 256; j++) {
					uart_cprintf("%#x ", buf[j]);
				}
				uart_cprintf("\n");

			} else if (dt == AHCI_DEV_SATAPI) {
				uart_cprintf("SATAPI drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_SEMB) {
				uart_cprintf("SEMB drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_PM) {
				uart_cprintf("PM drive found at port %d\n", i);
			} else {
				uart_cprintf("No drive found at port %d\n", i);
			}
		}
	}
}
