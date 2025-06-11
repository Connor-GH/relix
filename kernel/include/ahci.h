#pragma once
#if __KERNEL__
#include <stdbool.h>
#include <stdint.h>

#pragma GCC diagnostic ignored "-Wattributes"

typedef enum {
	FISTypeRegH2D = 0x27, // Register FIS - host to device
	FISTypeRegD2H = 0x34, // Register FIS - device to host
	FISTypeDMAAct = 0x39, // Dma activate FIS - device to host
	FISTypeDMASetup = 0x41, // Dma setup FIS - bidirectional
	FISTypeData = 0x46, // Data FIS - bidirectional
	FISTypeBIST = 0x58, // BIST activate FIS - bidirectional
	FISTypePIOSetup = 0x5F, // Pio setup FIS - device to host
	FISTypeDevBits = 0xA1, // Set device bits FIS - device to host
} FISType;

typedef struct FISRegH2D {
	// uint32_t 0
	uint8_t fis_type; // FISTypeRegH2D
	uint8_t pmport : 4; // Port multiplier
	uint8_t rsv0 : 3; // Reserved
	uint8_t c : 1; // 1: Command, 0: Control
	uint8_t command; // Command register
	uint8_t featurel; // Feature register, 7:0
	// uint32_t 1
	uint8_t lba0; // Lba low register, 7:0
	uint8_t lba1; // Lba mid register, 15:8
	uint8_t lba2; // Lba high register, 23:16
	uint8_t device; // Device register
	// uint32_t 2
	uint8_t lba3; // Lba register, 31:24
	uint8_t lba4; // Lba register, 39:32
	uint8_t lba5; // Lba register, 47:40
	uint8_t featureh; // Feature register, 15:8
	// uint32_t 3
	uint8_t countl; // Count register, 7:0
	uint8_t counth; // Count register, 15:8
	uint8_t icc; // Isochronous command completion
	uint8_t control; // Control register
	// uint32_t 4
	uint8_t rsv1[4]; // Reserved
} FISRegH2D __attribute__((packed));
typedef struct FISRegD2H {
	// uint32_t 0
	uint8_t fis_type; // FISTypeRegD2H
	uint8_t pmport : 4; // Port multiplier
	uint8_t rsv0 : 2; // Reserved
	uint8_t i : 1; // Interrupt bit
	uint8_t rsv1 : 1; // Reserved
	uint8_t status; // Status register
	uint8_t error; // Error register
	// uint32_t 1
	uint8_t lba0; // Lba low register, 7:0
	uint8_t lba1; // Lba mid register, 15:8
	uint8_t lba2; // Lba high register, 23:16
	uint8_t device; // Device register
	// uint32_t 2
	uint8_t lba3; // Lba register, 31:24
	uint8_t lba4; // Lba register, 39:32
	uint8_t lba5; // Lba register, 47:40
	uint8_t rsv2; // Reserved
	// uint32_t 3
	uint8_t countl; // Count register, 7:0
	uint8_t counth; // Count register, 15:8
	uint8_t rsv3[2]; // Reserved
	// uint32_t 4
	uint8_t rsv4[4]; // Reserved
} FISRegD2H __attribute__((packed));

typedef struct FISData {
	// uint32_t 0
	uint8_t fis_type; // FISTypeData
	uint8_t pmport : 4; // Port multiplier
	uint8_t rsv0 : 4; // Reserved
	uint8_t rsv1[2]; // Reserved
	// uint32_t 1 ~ N
	uint32_t data[1]; // Payload
} FISData __attribute__((packed));

typedef struct FISPIOSetup {
	// uint32_t 0
	uint8_t fis_type; // FISTypePIOSetup
	uint8_t pmport : 4; // Port multiplier
	uint8_t rsv0 : 1; // Reserved
	uint8_t d : 1; // Data transfer direction, 1 - device to host
	uint8_t i : 1; // Interrupt bit
	uint8_t rsv1 : 1;
	uint8_t status; // Status register
	uint8_t error; // Error register
	// uint32_t 1
	uint8_t lba0; // Lba low register, 7:0
	uint8_t lba1; // Lba mid register, 15:8
	uint8_t lba2; // Lba high register, 23:16
	uint8_t device; // Device register
	// uint32_t 2
	uint8_t lba3; // Lba register, 31:24
	uint8_t lba4; // Lba register, 39:32
	uint8_t lba5; // Lba register, 47:40
	uint8_t rsv2; // Reserved
	// uint32_t 3
	uint8_t countl; // Count register, 7:0
	uint8_t counth; // Count register, 15:8
	uint8_t rsv3; // Reserved
	uint8_t e_status; // New value of status register
	// uint32_t 4
	uint16_t tc; // Transfer count
	uint8_t rsv4[2]; // Reserved
} FISPIOSetup __attribute__((packed));

typedef struct FISDMASetup {
	// uint32_t 0
	uint8_t fis_type; // FISTypeDMASetup
	uint8_t pmport : 4; // Port multiplier
	uint8_t rsv0 : 1; // Reserved
	uint8_t d : 1; // Data transfer direction, 1 - device to host
	uint8_t i : 1; // Interrupt bit
	uint8_t a : 1; // Auto-activate. Specifies if DMA Activate FIS is needed
	uint8_t rsved[2]; // Reserved
	// uint32_t 1-2
	// Dma Buffer Identifier.
	// Used to Identify Dma buffer in host memory.
	// SATA spec says host specific and not in spec.
	// Trying AHCI spec might work.
	uint64_t dma_buffer_id;
	// uint32_t 3
	uint32_t rsvd; // More reserved
	// uint32_t 4
	// Byte offset into buffer. First 2 bits must be 0
	uint32_t dma_buf_offset;
	// uint32_t 5
	// Number of bytes to transfer. Bit 0 must be 0
	uint32_t transfer_count;
	// uint32_t 6
	uint32_t reserved;
} FISDMASetup __attribute__((packed));

typedef volatile struct HBAPort {
	uint32_t clb; // 0x00, command list base address, 1K-byte aligned
	uint32_t clbu; // 0x04, command list base address upper 32 bits
	uint32_t fb; // 0x08, FIS base address, 256-byte aligned
	uint32_t fbu; // 0x0C, FIS base address upper 32 bits
	uint32_t is; // 0x10, interrupt status
	uint32_t ie; // 0x14, interrupt enable
	uint32_t cmd; // 0x18, command and status
	uint32_t rsv0; // 0x1C, Reserved
	uint32_t tfd; // 0x20, task file data
	uint32_t sig; // 0x24, signature
	uint32_t ssts; // 0x28, Sata status (Scr0:Sstatus)
	uint32_t sctl; // 0x2C, Sata control (Scr2:Scontrol)
	uint32_t serr; // 0x30, Sata error (Scr1:Serror)
	uint32_t sact; // 0x34, Sata active (Scr3:Sactive)
	uint32_t ci; // 0x38, command issue
	uint32_t sntf; // 0x3C, Sata notification (Scr4:Snotification)
	uint32_t fbs; // 0x40, FIS-based switch control
	uint32_t rsv1[11]; // 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4]; // 0x70 ~ 0x7F, vendor specific
} HBAPort __attribute__((packed));

typedef volatile struct HBAMem {
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap; // 0x00, Host capability
	uint32_t ghc; // 0x04, Global host control
	uint32_t is; // 0x08, Interrupt status
	uint32_t pi; // 0x0C, Port implemented
	uint32_t vs; // 0x10, Version
	uint32_t ccc_ctl; // 0x14, Command completion coalescing control
	uint32_t ccc_pts; // 0x18, Command completion coalescing ports
	uint32_t em_loc; // 0x1C, Enclosure management location
	uint32_t em_ctl; // 0x20, Enclosure management control
	uint32_t cap2; // 0x24, Host capabilities extended
	uint32_t bohc; // 0x28, Bios/Os handoff control and status
	// 0x2C - 0x9F, Reserved
	uint8_t reserved[0xA0 - 0x2C];
	// 0xA0 - 0xFf, Vendor specific registers
	uint8_t vendor[0x100 - 0xA0];
	// 0x100 - 0x10Ff, Port control registers
	HBAPort ports[1]; // 1 ~ 32
} HBAMem __attribute__((packed));

typedef struct HBACmdHeader {
	// DW0
	uint8_t cfl : 5; // Command FIS length in uint32_tS, 2 ~ 16
	uint8_t a : 1; // ATAPI
	uint8_t w : 1; // Write, 1: H2D, 0: D2H
	uint8_t p : 1; // Prefetchable
	uint8_t r : 1; // Reset
	uint8_t b : 1; // BIST
	uint8_t c : 1; // Clear busy upon ROk
	uint8_t rsv0 : 1; // Reserved
	uint8_t pmp : 4; // Port multiplier port
	uint16_t prdtl; // Physical region descriptor table length in entries
	// DW1
	volatile uint32_t prdbc; // Physical region descriptor byte count transferred
	// DW2, 3
	uint32_t ctba; // Command table descriptor base address
	uint32_t ctbau; // Command table descriptor base address upper 32 bits
	// DW4 - 7
	uint32_t rsv1[4]; // Reserved
} HBACmdHeader __attribute__((packed));

void probe_port(HBAMem *abar);
typedef struct HBAPRDTEntry {
	uint32_t dba; // Data base address
	uint32_t dbau; // Data base address upper 32 bits
	uint32_t rsv0; // Reserved
	// DW3
	uint32_t dbc : 22; // Byte count, 4M max
	uint32_t rsv1 : 9; // Reserved
	uint32_t i : 1; // Interrupt on completion
} HBAPRDTEntry __attribute__((packed));

typedef struct HBACmdTbl {
	// 0x00
	uint8_t cfis[64]; // Command FIS
	// 0x40
	uint8_t acmd[16]; // ATAPI command, 12 or 16 bytes
	// 0x50
	uint8_t rsv[48]; // Reserved
	// 0x80
	// Physical region descriptor table entries, 0 ~ 65535
	HBAPRDTEntry prdt_entry[1];
} HBACmdTbl __attribute__((packed));

// detailed in Acs-4
typedef struct IdentifyDevicePIO {
	// DW0
	uint16_t general_configuration;
#define INCOMPLETE_RESPONSE (1 << 2)
#define IS_ATA_DEVICE(general_config) (((1 << 15) & general_config) == 0)

	// DW1
	uint16_t obsolete3;

	// DW2
	uint16_t specific_configuration;

	// DW3
	uint16_t obsolete4;

	// DW4-5
	uint16_t retired4[5 - 4 + 1];

	// DW6
	uint16_t obsolete5;

	// DW7-8
	uint16_t reserved_for_cfa[8 - 7 + 1];

	// DW9
	uint16_t retired5;

	// DW10-19
	uint16_t serial_number[19 - 10 + 1];

	// DW20-21
	uint16_t retired6[21 - 20 + 1];

	// DW22
	uint16_t obsolete6;

	// DW23-26
	uint16_t firmware_version[26 - 23 + 1];

	// DW27-46
	uint16_t model_number[46 - 27 + 1];

	// DW47
	uint16_t obsolete7;

	// DW48
	uint16_t trusted_computing_features;

	// DW49
	uint16_t capabilities1;

	// DW50
	uint16_t capabilities2;

	// DW51-52
	uint16_t obsolete8[52 - 51 + 1];

	// DW53
	uint16_t dword53;

	// DW54-58
	uint16_t obsolete9[58 - 54 + 1];

	// DW59
	uint16_t dword59;

	// DW60-61
	uint16_t total_num_addressable_log_sectors[61 - 60 + 1];

	// DW62
	uint16_t obsolete10;

	// DW63
	uint16_t dword63;

	// DW64
	uint16_t dword64;

	// DW65
	uint16_t min_multiword_dma_transfer_time;

	// DW66
	uint16_t recommended_multiword_dma_transfer_time;

	// DW67
	uint16_t min_pio_transfer_time;

	// DW68
	uint16_t min_pio_transfer_time_iordy;

	// DW69
	uint16_t additional_supported;

	// DW70
	uint16_t reserved2;

	// DW71-74
	uint16_t reserved_for_identify_packet[74 - 71 + 1];

	// DW75
	uint16_t max_queue_depth;

	// DW76
	uint16_t sata_capabilities;

	// DW77
	uint16_t sata_additional_capabilities;

	// DW78
	uint16_t sata_features_supported;

	// DW79
	uint16_t sata_features_enabled;

	// DW80
	uint16_t major_version_number;

	// DW81
	uint16_t minor_version_number;

	// DW82
	uint16_t commands_feature_sets_supported1;

	// DW83
	uint16_t commands_feature_sets_supported2;

	// DW84
	uint16_t commands_feature_sets_supported3;

	// DW85
	uint16_t commands_feature_sets_supported_or_enabled1;

	// DW86
	uint16_t commands_feature_sets_supported_or_enabled2;

	// DW87
	uint16_t commands_feature_sets_supported_or_enabled3;

	// DW88
	uint16_t ultra_dma_modes;

	// DW89
	uint16_t time_normal_erase;

	// DW90
	uint16_t time_enhanced_erase;

	// DW91
	uint16_t dword91;
#define apm_level_value(dw) (x & 0b11111111)

	// DW92
	uint16_t master_password_identifier;

	// DW93
	uint16_t hw_reset_results;

	// Dw 94
	uint16_t obsolete11;

	// DW95
	uint16_t stream_min_req_size;

	// DW96
	uint16_t stream_transfer_time_dma;

	// DW97
	uint16_t stream_access_latency_dma_and_pio;

	// DW98-99
	uint16_t stream_performance_granularity[99 - 98 + 1];

	// DW100-103
	uint16_t num_user_addressable_logical_sectors[103 - 100 + 1];

	// DW104
	uint16_t stream_transfer_time_pio;

	// DW105
	uint16_t max_num_512byte_blocks_per_data_set_mgmt;

	// DW106
	uint16_t physical_or_logical_sector_size;

	// DW107
	uint16_t inter_seek_delay_iso7779;

	// DW108-111
	uint16_t world_wide_name[111 - 108 + 1];

	// Dw 112-115
	uint16_t reserved3[115 - 112 + 1];

	// Dw 116
	uint16_t obsolete12;

	// Dw 117-118
	uint16_t logical_sector_size_dword[118 - 117 + 1];

	// DW119
	uint16_t commands_feature_sets_supported4;

	// DW120
	uint16_t commands_feature_sets_supported_or_enabled4;

	// DW121-126
	uint16_t reserved_for_expanded_supported_and_enabled_settings[126 - 121 + 1];

	// DW127
	uint16_t obsolete13;

	// DW128
	uint16_t security_status;

	// DW129-159
	uint16_t vendor_specific[159 - 129 + 1];

	// DW160-167
	uint16_t reserved_for_cfa2[167 - 160 + 1];

	// DW168
	uint16_t dword168;

	// DW169
	uint16_t data_set_mgmt_command_support;

	// DW170-173
	uint16_t additional_product_id[173 - 170 + 1];

	// DW174-175
	uint16_t reserved4[175 - 174 + 1];

	// DW176-205
	uint16_t current_media_serial_number[205 - 176 + 1];

	// DW206
	uint16_t sct_command_transport;

	// DW207-208
	uint16_t reserved5[208 - 207 + 1];

	// DW209
	uint16_t alignment_of_logical_sectors_in_phys_sector;

	// DW210-211
	uint16_t wr_verify_sector_mode_3_count[211 - 210 + 1];

	// DW212-213
	uint16_t wr_verify_sector_mode_2_count[213 - 212 + 1];

	// DW214-216
	uint16_t obsolete14[216 - 214 + 1];

	// DW217
	uint16_t nominal_media_rotation_rate;

	// DW218
	uint16_t reserved6;

	// DW219
	uint16_t obsolete15;

	// DW220
	uint16_t dword220;

	// DW221
	uint16_t reserved7;

	// DW222
	uint16_t transport_major_version_number;

	// DW223
	uint16_t transport_minor_version_number;

	// DW222-229
	uint16_t reserved8[229 - 224 + 1];

	// DW230-233
	uint16_t extended_num_user_addressable_sectors[233 - 230 + 1];

	// DW234
	uint16_t min_num_512byte_blocks_per_download_microcode;

	// DW235
	uint16_t max_num_512byte_blocks_per_download_microcode;

	// DW236-254
	uint16_t reserved9[254 - 236 + 1];

	// DW255
	uint16_t integrity_word;

} IdentifyDevicePIO;

bool read_port(HBAPort *port, uint64_t start, uint16_t count, uint16_t *buf);
bool write_port(HBAPort *port, uint64_t start, uint16_t count, uint16_t *buf);
void ahci_init(uint32_t abar_);
#endif
