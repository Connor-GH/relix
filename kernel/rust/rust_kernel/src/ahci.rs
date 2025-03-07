use crate::pci::{disable_intr, enable_command_bus_master, PCICommonHeader};
use crate::printing::*;
use alloc::{
    string::{String, ToString},
    vec::Vec,
};
use bindings::memlayout::{io2v, p2v, v2io, v2p};
use core::ffi::c_void;

macro_rules! c_enum {
    ($(enum $name:ident : $type:ty { $($variant:ident = $value:expr,)* })+) => {
        $(#[allow(non_snake_case)]
        pub mod $name {
            $(pub const $variant: $type = $value;)*
        })+
    }
}

#[derive(Debug)]
struct RegisterHostToDeviceFIS {
    pm_port_and_cr_bit: u8,
    command: u8,
    features: u8,
    lba_low: u8,
    lba_mid: u8,
    lba_high: u8,
    device: u8,
    lba_expanded_low: u8,
    lba_expanded_mid: u8,
    lba_expanded_high: u8,
    features_expanded: u8,
    count: u16,
    icc: u8,
    control: u8,
    auxillary: u32,
}
#[derive(Debug)]
struct RegisterDeviceToHostFIS {
    pm_port_and_cr_bit: u8,
    status: u8,
    error: u8,
    lba_low: u8,
    lba_mid: u8,
    lba_high: u8,
    device: u8,
    lba_expanded_low: u8,
    lba_expanded_mid: u8,
    lba_expanded_high: u8,
    count: u16,
}

#[derive(Debug)]
struct DMASetup {
    // Bit 5 is direction, bit 6 is interrupt, bit 7 is auto-activate.
    pm_port: u8,
    dma_buffer: u64,
    dma_buffer_offset: u32,
    dma_transfer_count: u32,
}

#[derive(Debug)]
struct PIOSetup {
    // Bit 5 is direction and bit 6 is interrupt bit.
    pm_port: u8,
    status: u8,
    error: u8,
    lba_low: u8,
    lba_mid: u8,
    lba_high: u8,
    device: u8,
    lba_expanded_low: u8,
    lba_expanded_mid: u8,
    lba_expanded_high: u8,
    count: u16,
    e_status: u8,
    transfer_count: u16,
}

#[derive(Debug)]
struct DataBidirectional {
    pm_port: u8,
    data: Vec<u32>,
}

const IDENTIFY_SEP: u32 = 0xEC;
c_enum! {
    enum Cap: u32 {
        A64 = 1 << 31, // 64-bit addressing
        CQA = 1 << 30, // command queue acceleration
        CD = 1 << 29, // cold presence detect
        IS = 1 << 28, // interlock switch
        SSU = 1 << 27, // Staggered spin-up
        ALPM = 1 << 26, // Aggressive link power management.
        AL = 1 << 25, // Activity LED.
        RAWFIS = 1 << 24, // Raw FIS mode.
        ISS = 0b1111 << 20, // Interface Speed support.
        NZDO = 1 << 19, // Nonzero DMA offsets.
        PSSA = 1 << 18, // Port selector acceleration
        PM = 1 << 17, // Port multiplier.
        PMFS = 1 << 16, // Port multiplier FIS based switching.
        PMD = 1 << 15, // PIO Multiple DRQ Block.
        SSC = 1 << 14, // Slumber state.
        PS = 1 << 13, // Partial state
        NCS = 0b11111 << 8, // Number of command slots.
        CCCS = 1 << 7,
        EMS = 1 << 6,
        SXS = 1 << 5,
        NP = 0b1111 << 0, // Number of ports.
    }

    enum GlobalHBAControl: u32 {
        AE = 1 << 31, // AHCI Enable.
        MRSM = 1 << 2, // MSI Revert to Single Message.
        IE = 1 << 1, // Interrupt Enable.
        HR = 1 << 0, // HBA Reset.
    }
    enum CCCCtl: u32 {
        TV = 0b1111111111111111 << 16, // Timeout value.
        CC = 0b1111 << 8, // Command completions.
        INT = 0b1111 << 3, // Interrupt.
        EN = 1 << 0, // Enable.
    }
    enum EnclosureManagementLocation: u32 {
        OFST = 0xFFFF << 16, // Offset
        SZ = 0xFFFF << 0, // Size
    }
    enum EnclosureManagementControl: u32 {
        PM = 1 << 27, // Port multiplier support.
        ALHD = 1 << 26, // Activity LED Hardware Driven.
        XMT = 1 << 25, // Transmit only
        SMB = 1 << 24, // Single message buffer.
        SGPIO = 1 << 19,
        SES_2 = 1 << 18,
        SAF_TE = 1 << 17,
        LED = 1 << 16, // LED Message types.
        RESET = 1 << 9,
        TM = 1 << 8, // Transmit message.
        MR = 1 << 0, // Message recieved.
    }
    enum HBACapabilitiesExtended: u32 {
        DESO = 1 << 5, // DevSleep Entrace from Slumber only.
        ADSM = 1 << 4, // Aggressive device sleep management.
        DS = 1 << 3, // Device sleep.
        APST = 1 << 2, // Automatic partial to slumber transitions.
        NVMP = 1 << 1, // NVMHCI
        BOH = 1 << 0, // BIOS/OS Handoff
    }
    enum BiosOSHandoff: u32 {
        BB = 1 << 4, // Bios busy
        OOC = 1 << 3, // OS Ownership Change.
        SOOE = 1 << 2, // SMI on OOC enable.
        OOS = 1 << 1, // OS-owned semaphore.
        BOS = 1 << 0, // BIOS-owned semaphore.
    }
    enum PortxInterruptStatus: u32 {
        CDPS = 1 << 31, // Cold Port Detect Status
        TFES = 1 << 30, // Task File Error Status
        HBFS = 1 << 29, // Host Bus Fatal Error Status
        HBDS = 1 << 28, // Host Bus Data Error Status
        IFS = 1 << 27, // Interface Fatal Error Status
        INFS = 1 << 26, // Interface Non-fatal Error Status
        OFS = 1 << 24, // Overflow Status
        IPMS = 1 << 23, // Incorrect Port Multiplier Status
        PRCS = 1 << 22, // PhyRdy Change Status (Read-Only)
        DMPS = 1 << 07, // Device Mechanical Presence Status
        PCS = 1 << 06, // Port Connect Change Status (Read-Only)
        DPS = 1 << 05, // Descriptor Processed
        UFS = 1 << 04, // Unknown FIS Interrupt (Read-Only)
        SDBS = 1 << 03, // Set Device Bits Interrupt
        DSS = 1 << 02, // DMA Setup FIS Interrupt
        PSS = 1 << 01, // PIO Setup FIS Interrupt
        DHRS = 1 << 0, // Device to Host Register FIS Interrupt
    }
    enum PortxInterruptEnable: u32 {
        CPDE = 1 << 31, // Cold Presence Detect Enable (Read-Write/Read-Only)
        TFEE = 1 << 30, // Task File Error Enable (Read-Write)
        HBFE = 1 << 29, // Host Bus Fatal Error Enable (Read-Write)
        HBDE = 1 << 28, // Host Bus Data Error Enable (Read-Write)
        IFE = 1 << 27, // Interface Fatal Error Enable (Read-Write)
        INFE = 1 << 26, // Interface Non-fatal Error Enable (Read-Write)
        OFE = 1 << 24, // Overflow Enable (Read-Write)
        IPME = 1 << 23, // Incorrect Port Multiplier Enable (Read-Write)
        PRCE = 1 << 22, // PhyRdy Change Interrupt Enable (Read-Write)
        DMPE = 1 << 07, // Device Mechanical Presence Enable (Read-Write/Read-Only)
        PCE = 1 << 06, // Port Change Interrupt Enable (Read-Write)
        DPE = 1 << 05, // Descriptor Processed Interrupt Enable (Read-Write)
        UFE = 1 << 04, // Unknown FIS Interrupt Enable (Read-Write)
        SDBE = 1 << 03, // Set Device Bits FIS Interrupt Enable (Read-Write)
        DSE = 1 << 02, // DMA Setup FIS Interrupt Enable (Read-Write)
        PSE = 1 << 01, // PIO Setup FIS Interrupt Enable (Read-Write)
        DHRE = 1 << 00, // Device to Host Register FIS Interrupt Enable (Read-Write)
    }
    enum PortxCommandStatus: u32 {
        ICC = 0b1111 << 28, // Interface Communication Control
        ASP = 1 << 27, // Aggressive Slumber / Partial (Read-Write/Read-Only)
        ALPE = 1 << 26, // Aggressive Link Power Management Enable (Read-Write/Read-Only)
        DLAE = 1 << 25, // Drive LED on ATAPI Enable (Read-Write)
        ATAPI = 1 << 24, // Device is ATAPI (Read-Write)
        APSTE = 1 << 23, // Automatic Partial to Slumber Transitions Enabled (Read-Write)
        FBSCP = 1 << 22, // FIS-based Switching Capable Port (HwInit)
        ESP = 1 << 21, // External SATA Port (HwInit)
        CPD = 1 << 20, // Cold Presence Detection (HwInit)
        MPSP = 1 << 19, // Mechanical Presence Switch Attached to Port (HwInit)
        HPCP = 1 << 18, // Hot Plug Capable Port (HwInit)
        PMA = 1 << 17, // Port Multiplier Attached (Read-Write/Read-Only)
        CPS = 1 << 16, // Cold Presence State (Read-Only)
        CR = 1 << 15, // Command List Running (Read-Only)
        FR = 1 << 14, // FIS Receive Running (Read-Only)
        MPSS = 1 << 13, // Mechanical Presence Switch State (Read-Only)
        CCS = 0b1111 << 08, // Current Command Slot (Read-Only)
        FRE = 1 << 04, // FIS Receive Enable (Read-Write)
        CLO = 1 << 03, // Command List Override
        POD = 1 << 02, // Power On Device (Read-Write/Read-Only)
        SUD = 1 << 01, // Spin-Up Device (Read-Write/Read-Only)
        ST = 1 << 00, // Start (Read-Write)
    }
}

struct RegisterHBAMemory {
    host_cap: u32,
    global_hba_control: u32,
    interrupt_status: u32,
    ports_implemented: u32,
    // 0..15 = minor, 16..31 = major
    version: u32,
    ccc_ctl: u32,   // Command completion coalescing control.
    ccc_ports: u32, // Command completion coalescing ports.
    em_loc: u32,    // Enclosure management location.
    em_ctl: u32,    // Enclosure management control.
    cap2: u32,      // Host capabilities extended
    bohc: u32,      // BIOS/OS handoff control and status.
    vendor: [u8; 0x100 - 0xA0],
    ports: Vec<PortRegister>,
}

impl core::fmt::Debug for RegisterHBAMemory {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("RegisterHBAMemory")
            .field("cap", &self.cap())
            .field("ghc", &self.ghc())
            .field("is", &self.interrupt_status)
            .field_with("ports implemented", |s| {
                s.write_fmt(format_args!("{}/{}", self.ports_implemented.trailing_ones(), self.ports()))
            })
            .field("version", &self.version())
            .field("ccc_ctl", &self.ccc_ctl)
            .field("ccc_ports", &self.ccc_ports)
            .field("em_loc", &self.em_loc)
            .field("em_ctl", &self.em_ctl)
            .field("cap2", &self.cap2)
            .field("bohc", &self.bohc)
            .field("ports", &self.ports)
            .finish()
    }
}

macro_rules! off {
    ($name:ident, $offset:expr, $t:ty) => {
        unsafe { *($name.byte_offset($offset) as *const $t) }
    };
}
fn is_set(bits: u32, bit: u32) -> bool {
    bits & bit == bit
}

impl RegisterHBAMemory {
    fn ports(&self) -> u32 {
        (self.host_cap & Cap::NP) as u32 + 1
    }
    fn version(&self) -> String {
        format_args!("{}.{}",
            ((self.version & 0xFF000000) >> 24) * 10 +
            ((self.version & 0x00FF0000) >> 16),
            ((self.version & 0x0000FF00) >> 8) * 10 +
            (self.version & 0x000000FF),
        ).to_string()
    }
    fn cap(&self) -> Vec<String> {
        let mut ret: Vec<String> = Vec::new();
        if is_set(self.host_cap, Cap::A64) {
            ret.push("64bit".to_string());
        }
        if is_set(self.host_cap, Cap::CQA) {
            // Command Queue Acceleration and
            // Native Command Queue are the same.
            ret.push("ncq".to_string());
        }
        if is_set(self.host_cap, Cap::CD) {
            ret.push("cpd".to_string());
        }
        if is_set(self.host_cap, Cap::SSU) {
            ret.push("ssu".to_string());
        }
        if is_set(self.host_cap, Cap::ALPM) {
            ret.push("alpm".to_string());
        }
        if is_set(self.host_cap, Cap::AL) {
            ret.push("al".to_string());
        }
        if is_set(self.host_cap, Cap::RAWFIS) {
            ret.push("rawfis".to_string());
        }
        if (self.host_cap & Cap::ISS) >> 20 as u32 != 0 {
            ret.push(match (self.host_cap & Cap::ISS) >> 20 as u32 {
                0b0001 => "Gen 1 (1.5Gbps)".to_string(),
                0b0010 => "Gen 2 (3 Gbps)".to_string(),
                0b0011 => "Gen 3 (6 Gbps)".to_string(),
                s => alloc::format!("Gen {} (? Gbps)", s),
            });
        }
        // Also known as "Nonzero DMA offset"
        if is_set(self.host_cap, Cap::NZDO) {
            ret.push("ahci_only".to_string());
        }
        if is_set(self.host_cap, Cap::PM) {
            ret.push("pm".to_string());
        }
        if is_set(self.host_cap, Cap::PMFS) {
            ret.push("fis_switching".to_string());
        }
        if is_set(self.host_cap, Cap::PMD) {
            ret.push("pmd".to_string());
        }
        if is_set(self.host_cap, Cap::SSC) {
            ret.push("ssc".to_string());
        }
        if is_set(self.host_cap, Cap::PS) {
            ret.push("ps".to_string());
        }
        if (self.host_cap & Cap::NCS) >> 8 as u32 != 0 {
            let mut tmp = "slots=".to_string();
            // "+1" because this is zero-based numbering.
            tmp.push_str(((((self.host_cap & Cap::NCS) >> 8) as u32 + 1).to_string()).as_str());
            ret.push(tmp);
        }
        if is_set(self.host_cap, Cap::CCCS) {
            ret.push("cccs".to_string());
        }
        if is_set(self.host_cap, Cap::EMS) {
            ret.push("ems".to_string());
        }
        if is_set(self.host_cap, Cap::SXS) {
            ret.push("sxs".to_string());
        }
        if self.host_cap & Cap::NP as u32 != 0 {
            let mut tmp1 = "ports=".to_string();
            // "+1" needed because this is a 0's based value.
            tmp1.push_str(((self.host_cap & Cap::NP) as u32 + 1).to_string().as_str());
            ret.push(tmp1);
        }
        ret
    }
    fn ghc(&self) -> Vec<String> {
        let mut vec = Vec::new();
        if is_set(self.global_hba_control, GlobalHBAControl::AE) {
            vec.push("ae".to_string());
        }
        if is_set(self.global_hba_control, GlobalHBAControl::MRSM) {
            vec.push("msrm".to_string());
        }
        if is_set(self.global_hba_control, GlobalHBAControl::IE) {
            vec.push("int_enable".to_string());
        }
        if is_set(self.global_hba_control, GlobalHBAControl::HR) {
            vec.push("hba_reset".to_string());
        }
        vec
    }
    fn pi(&self) -> u32 {
        self.ports_implemented.trailing_ones()
    }
    fn from_abar(virt_abar: usize) -> Self {
        let virt_abar = virt_abar as *const c_void;
        let cap = off!(virt_abar, 0, u32);
        let ghc = off!(virt_abar, 0x4, u32);
        let is = off!(virt_abar, 0x8, u32);
        let pi = off!(virt_abar, 0xC, u32);
        let vs = off!(virt_abar, 0x10, u32);
        let ccc_ctl = off!(virt_abar, 0x14, u32);
        let ccc_ports = off!(virt_abar, 0x18, u32);
        let em_loc = off!(virt_abar, 0x1C, u32);
        let em_ctl = off!(virt_abar, 0x20, u32);
        let cap2 = off!(virt_abar, 0x24, u32);
        let bohc = off!(virt_abar, 0x28, u32);

        let mut ports: Vec<PortRegister> = Vec::new();
        let mut pi_bits = pi;
        let mut bit_position = 0;
        while pi_bits != 0 {
            if pi_bits & 1 != 0 {
                ports.push(PortRegister::from_ptr(unsafe {
                    (virt_abar.byte_offset(0x100 + (bit_position * 0x80)))
                }));
            }
            pi_bits >>= 1;
            bit_position += 1;
        }
        Self {
            host_cap: cap,
            global_hba_control: ghc,
            interrupt_status: is,
            ports_implemented: pi,
            version: vs,
            ccc_ctl,
            ccc_ports,
            em_loc,
            em_ctl,
            cap2,
            bohc,
            vendor: [0; 96],
            ports,
        }
    }
}

struct PortRegister {
    clb: u64,     // Command List Base Address.
    fb: u64,      // FIS Base Address.
    is: u32,      // Interrupt Status.
    ie: u32,      // Interrupt Enable.
    cmd: u32,     // Command and Status.
    tfd: u32,     // Task File Data.
    sig: u32,     // Signature.
    ssts: u32,    // Serial ATA Status. (SCR0: SStatus).
    sctl: u32,    // Serial ATA Control (SCR2: SControl).
    serr: u32,    // Serial ATA Error (SCR1: SError).
    sact: u32,    // Serial ATA Active (SCR3: SActive).
    ci: u32,      // Comamnd Issue.
    sntf: u32,    // Serial ATA Notification (SCR4: SNotification).
    fbs: u32,     // FIS-based Switching Control.
    devslp: u32,  // Device Sleep.
    vs: [u32; 4], // Vendor Specific.
}
impl core::fmt::Debug for PortRegister {

    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        fn check_type(port: &PortRegister) -> u32 {
            let ssts = port.ssts;
            let ipm = (ssts >> 8) & 0xF;
            let det = ssts & 0x0F;

            if det != PORT_DET_PRESENT || ipm != PORT_IPM_ACTIVE {
                return AHCI_DEV_NULL;
            }
            return match (port.sig) {
                SATA_SIG_ATAPI => AHCI_DEV_SATAPI,
                SATA_SIG_SEMB => AHCI_DEV_SEMB,
                SATA_SIG_PM => AHCI_DEV_PM,
                SATA_SIG_ATA => AHCI_DEV_SATA,
                t => t,
            }
        }
        let dt = check_type(self);
        let s = match (dt) {
            AHCI_DEV_SATA => "SATA",
            AHCI_DEV_SATAPI => "SATAPI",
            AHCI_DEV_SEMB => "SEMB",
            AHCI_DEV_PM => "PM",
            AHCI_DEV_NULL => "NULL",
            _ => "Unknown",
        };
        f.write_fmt(format_args!("AHCI Port Device {} ", s))?;
        if (s == "Unknown") {
            f.write_fmt(format_args!(" ({}) ", dt))?;
        }
        f.debug_struct("PortRegister")
            .field("clb", &self.clb)
            .field("fb", &self.fb)
            .field("is", &self.is)
            .field("ie", &self.ie)
            .field("cmd", &self.cmd)
            .field("tfd", &self.tfd)
            .field("sig", &self.sig)
            .field("ssts", &self.ssts)
            .field("sctl", &self.sctl)
            .field("serr", &self.serr)
            .field("sact", &self.sact)
            .field("ci", &self.ci)
            .field("sntf", &self.sntf)
            .field("fbs", &self.fbs)
            .field("devslp", &self.devslp)
            .field("vs", &self.vs)
            .finish()
    }
}

impl PortRegister {
    fn from_ptr(ptr: *const c_void) -> Self {
        let clb = off!(ptr, 0x0, u64);
        let fb = off!(ptr, 0x8, u64);
        let is = off!(ptr, 0x10, u32);
        let ie = off!(ptr, 0x14, u32);
        let cmd = off!(ptr, 0x18, u32);
        let tfd = off!(ptr, 0x20, u32);
        let sig = off!(ptr, 0x24, u32);
        let ssts = off!(ptr, 0x28, u32);
        let sctl = off!(ptr, 0x2C, u32);
        let serr = off!(ptr, 0x40, u32);
        let sact = off!(ptr, 0x34, u32);
        let ci = off!(ptr, 0x38, u32);
        let sntf = off!(ptr, 0x3C, u32);
        let fbs = off!(ptr, 0x40, u32);
        let devslp = off!(ptr, 0x44, u32);
        let vendor1 = off!(ptr, 0x70, u32);
        let vendor2 = off!(ptr, 0x74, u32);
        let vendor3 = off!(ptr, 0x78, u32);
        let vendor4 = off!(ptr, 0x7C, u32);
        let vs = [vendor1, vendor2, vendor3, vendor4];
        Self {
            clb,
            fb,
            is,
            ie,
            cmd,
            tfd,
            sig,
            ssts,
            sctl,
            serr,
            sact,
            ci,
            sntf,
            fbs,
            devslp,
            vs,
        }
    }
}

struct HbaFIS {
    dsfis: DMASetup,
    psfis: PIOSetup,
    rfis: RegisterDeviceToHostFIS,
    sdbfis: (),
    ufis: [u8; 64],
}
struct CommandList {
    // bit 5: ATAPI
    // bit 6: Write
    // bit 7: Prefetchable
    cfl: u8,
    // 0=Reset, 1=BIST, 2 = Clear, 4-7: Port mulitplier.
    byte2: u8,
    prdtl: u16,
    prdbc: u32,
    ctba: u32,
    ctbau: u32,
}
struct HbaPRDTEntry {
    dba: u32,
    dbau: u32,
    // 0-21 Byte count
    // 22-30 Reserved
    // 30-31 Interrupt on Completion
    dword3: u32,
}
struct HdaCommandTable {
    cfis: [u8; 64],
    acmd: [u8; 16],
    prdt_entry: Vec<HbaPRDTEntry>,
}
c_enum! {

    enum FIS : u32 {
        REG_H2D = 0x27,
        REG_D2H = 0x34,
        DMA_ACT = 0x39,
        DMA_SETUP = 0x41,
        DATA = 0x46,
        BIST = 0x58,
        PIO_SETUP = 0x5F,
        DEV_BITS = 0xA1,
    }
}
const SATA_SIG_ATA: u32 = 0x00000101;
const SATA_SIG_ATAPI: u32 = 0xEB140101;
const SATA_SIG_SEMB: u32 = 0xC33C0101;
const SATA_SIG_PM: u32 = 0x96690101;

const AHCI_DEV_NULL: u32 = 0;
const AHCI_DEV_SATA: u32 = 1;
const AHCI_DEV_SEMB: u32 = 2;
const AHCI_DEV_PM: u32 = 3;
const AHCI_DEV_SATAPI: u32 = 4;
const PORT_IPM_ACTIVE: u32 = 1;
const PORT_DET_PRESENT: u32 = 3;



pub fn ahci_init(hdr: &mut PCICommonHeader, abar_addr: u32) {
    // Enable "bus master"
    enable_command_bus_master(hdr);
    disable_intr(hdr);
    let virt_ptr = io2v(abar_addr as usize) as usize;
    println!("{:#x?}", &RegisterHBAMemory::from_abar(virt_ptr));
}
