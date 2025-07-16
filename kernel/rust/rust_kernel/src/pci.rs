// https://wiki.osdev.org/PCI
use crate::printing::*;
use alloc::vec::Vec;
use core::ffi::c_void;
use kernel_bindings::bindings::{inl, outl};
use spin::Mutex;
const CONFIG_ADDRESS: u16 = 0xCF8;
const CONFIG_DATA: u16 = 0xCFC;

const COMMAND_MEMORY_SPACE_RESPONSE: u16 = 1 << 1;
const COMMAND_BUS_MASTER: u16 = 1 << 2;
const COMMAND_INTERRUPT_DISABLE: u16 = 1 << 10;

const MSI_PER_VECTOR_MASKING: u32 = 1 << 8;
const MSI_64BIT: u32 = 1 << 7;

const CAPABILITY_MSI: u8 = 0x5;
const CAPABILITY_MSI_X: u8 = 0x11;
const CAPABILITY_SATA: u8 = 0x12;

static PCI_CONFS: Mutex<Vec<PciConf>> = Mutex::new(Vec::new());

pub struct PCICommonHeader {
    vendor_id: u16,
    device_id: u16,
    command: u16,
    status: u16,
    revision_id: u8,
    prog_if: u8,
    subclass: u8,
    base_class: u8,
    cacheline_size: u8,
    latency_timer: u8,
    header_type: u8,
    bist: u8,
    base_address_registers: [u32; 6],
    cardbus_cis_pointer: u32,
    subsystem_vendor_id: u16,
    subsystem_id: u16,
    expansion_rom_base_address: u32,
    capabilities_pointer: u8,
    interrupt_line: u8,
    interrupt_pin: u8,
    min_gnt: u8,
    max_latency: u8,
    bus: u8,
    device: u8,
    function: u8,
}
#[derive(Clone, Debug)]
#[repr(C)]
pub struct PciConf {
    vendor_id: u16,
    device_id: u16,
    subsystem_vendor_id: u16,
    subsystem_id: u16,
    revision_id: u8,
    prog_if: u8,
    subclass: u8,
    base_class: u8,
    cacheline_size: u8,
    header_type: u8,
    function: u8,
    device: u8,
    bus: u8,
}
/*
 * We only support 64-bit MSI's.
 * Additionally, we support them
 * having masks and pending bits
 * or not.
 */
enum MsiType {
    WithMask(Msi),
    WithoutMask(MsiNoMask),
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
struct Msi {
    cap_id: u8,
    next_ptr: u8,
    message_control: u16,

    message_address: u32,
    message_upper_address: u32,

    message_data: u16,
    mask_bits: u32,
    pending_bits: u32,
}

impl Msi {
    fn from_cap_ptr((bus, slot, func): (u8, u8, u8), offset: u8) -> Self {
        let tuple = (bus, slot, func);
        let cap_id = pci_config_read_word(tuple, offset) as u8;
        assert!(cap_id == CAPABILITY_MSI, "CapID does not match with MSI!");
        let next_ptr = (pci_config_read_word(tuple, offset + 0x1) >> 8) as u8;
        let message_control = pci_config_read_word(tuple, offset + 0x2);
        assert!(
            message_control & MSI_64BIT as u16 != 0,
            "MSI is not 64 bit!"
        );
        assert!(
            message_control & MSI_PER_VECTOR_MASKING as u16 != 0,
            "MSI does not mask vectors!"
        );
        let message_address = pci_config_read_long(tuple, offset + 0x4);
        let message_upper_address = pci_config_read_long(tuple, offset + 0x8);
        let message_data = pci_config_read_word(tuple, offset + 0xC);
        let mask_bits = pci_config_read_long(tuple, offset + 0x10);
        let pending_bits = pci_config_read_long(tuple, offset + 0x14);
        Msi {
            cap_id,
            next_ptr,
            message_control,
            message_address,
            message_upper_address,
            message_data,
            mask_bits,
            pending_bits,
        }
    }
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
struct MsiNoMask {
    cap_id: u8,
    next_ptr: u8,
    message_control: u16,

    message_address: u32,
    message_upper_address: u32,

    message_data: u16,
}

impl MsiNoMask {
    fn from_cap_ptr((bus, slot, func): (u8, u8, u8), offset: u8) -> Self {
        let tuple = (bus, slot, func);
        let cap_id = pci_config_read_word(tuple, offset) as u8;
        assert!(cap_id == CAPABILITY_MSI, "CapID does not match with MSI!");
        let next_ptr = (pci_config_read_word(tuple, offset + 0x1) >> 8) as u8;
        let message_control = pci_config_read_word(tuple, offset + 0x2);
        assert!(
            message_control & MSI_64BIT as u16 != 0,
            "MSI is not 64 bit!"
        );
        let message_address = pci_config_read_long(tuple, offset + 0x4);
        let message_upper_address = pci_config_read_long(tuple, offset + 0x8);
        let message_data = pci_config_read_word(tuple, offset + 0xC);
        MsiNoMask {
            cap_id,
            next_ptr,
            message_control,
            message_address,
            message_upper_address,
            message_data,
        }
    }
}
#[derive(Debug, Clone, Copy)]
#[allow(non_camel_case_types)]
struct Msi_X {
    cap_id: u8,
    next_ptr: u8,
    message_control: u16,

    bir: u8,
    table_offset: u32,
    pending_bit_bir: u8,
    pending_bit_offset: u32,
}

impl Msi_X {
    fn from_cap_ptr((bus, slot, func): (u8, u8, u8), offset: u8) -> Self {
        let tuple = (bus, slot, func);
        let cap_id = pci_config_read_word(tuple, offset) as u8;
        assert!(
            cap_id == CAPABILITY_MSI_X,
            "CapID does not match with MSI-X!"
        );
        let next_ptr = (pci_config_read_word(tuple, offset + 0x1) >> 8) as u8;
        let message_control = pci_config_read_word(tuple, offset + 0x2);

        let bir = pci_config_read_byte(tuple, offset + 4);
        let table_offset = pci_config_read_long(tuple, offset + 4) >> 3;
        let pending_bit_bir = pci_config_read_byte(tuple, offset + 0x8);
        let pending_bit_offset = pci_config_read_long(tuple, offset + 0x8) >> 3;
        Msi_X {
            cap_id,
            next_ptr,
            message_control,
            bir,
            table_offset,
            pending_bit_bir,
            pending_bit_offset,
        }
    }
}

const SATAR0_MINOR_REVISION_MASK: u8 = 0b00001111;
const SATAR0_MAJOR_REVISION_MASK: u8 = 0b11110000;
const SATAR1_BAR_LOCATION: u32 = 0b000000000000000000001111;
const SATAR1_BAR_OFFSET: u32 = 0b111111111111111111110000;
// serial ata ahci spec 2.4.1
// SATA Capability Registers 0 and 1
struct Sata {
    cap_id: u8,
    next_ptr: u8,
    minor0to4major4to7: u8,
    sata_r1: u32,
    // Offset that this sits at.
    // Used for BAR location.
    offset: u8,
}

impl Sata {
    fn from_cap_ptr(tuple: (u8, u8, u8), offset: u8) -> Self {
        let cap_id = pci_config_read_word(tuple, offset) as u8;
        assert!(cap_id == CAPABILITY_SATA, "CapID does not match with SATA!");
        let next_ptr = (pci_config_read_word(tuple, offset + 0x1) >> 8) as u8;
        let minor_revision =
            pci_config_read_word(tuple, offset + 0x2) as u8 & SATAR0_MINOR_REVISION_MASK;
        let major_revision =
            pci_config_read_word(tuple, offset + 0x2) as u8 & SATAR0_MAJOR_REVISION_MASK;
        let r1 = pci_config_read_long(tuple, offset + 0x4);
        Sata {
            cap_id,
            next_ptr,
            minor0to4major4to7: minor_revision | major_revision,
            sata_r1: r1,
            offset,
        }
    }
    fn minor_revision(&self) -> u8 {
        self.minor0to4major4to7 & SATAR0_MINOR_REVISION_MASK
    }
    fn major_revision(&self) -> u8 {
        (self.minor0to4major4to7 & SATAR0_MAJOR_REVISION_MASK) >> 4
    }
    fn bar_location_raw(&self) -> u8 {
        (self.sata_r1 & SATAR1_BAR_LOCATION) as u8
    }
    fn bar_offset_raw(&self) -> u32 {
        (self.sata_r1 & SATAR1_BAR_OFFSET) >> 4
    }
    // Can be anywhere from 0-64KB or 0-(1MB-4) depending on the setup.
    fn bar_offset(&self) -> u32 {
        self.bar_offset_raw() * 4
    }
    fn bar_location(&self) -> (u8, &str) {
        match self.bar_location_raw() {
            0b0100 => (0x10, "BAR0"), // BAR0
            0b0101 => (0x14, "BAR1"), // BAR1
            0b0110 => (0x18, "BAR2"), // BAR2
            0b0111 => (0x1C, "BAR3"), // BAR3
            0b1000 => (0x20, "BAR4"), // BAR4
            0b1001 => (0x24, "BAR5"), // BAR5
            0b1111 => (self.offset + 8, "After SATACR1"),
            _ => (0x10, "unknown (using BAR0)"),
        }
    }
}
impl core::fmt::Debug for Sata {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Sata")
            .field("capability id", &self.cap_id)
            .field("next ptr", &self.next_ptr)
            .field("minor revision", &self.minor_revision())
            .field("major revision", &self.major_revision())
            .field("bar location", &self.bar_location())
            .field("bar offset", &self.bar_offset())
            .finish()
    }
}

unsafe extern "C" {
    pub fn ahci_init(abar: u32) -> c_void;
}

fn dispatch_sata(hdr: &mut PCICommonHeader, offset: u32) {
    if hdr.vendor_id == 0x8086
        && hdr.device_id == 0x2922
        && hdr.subsystem_vendor_id == 0x1af4
        && hdr.subsystem_id == 0x1100
    {
        debugln!("sata at {:x}", hdr.base_address_registers[5] >> 0);
        unsafe { ahci_init(hdr.base_address_registers[5]) };
    }
}
fn info_headers((bus, slot, func): (u8, u8, u8), offset: u8, hdr: &mut PCICommonHeader) {
    debugln!("{}", hdr);
    command_and_status_registers(hdr);
    traverse_headers((bus, slot, func), offset, hdr);
}

fn traverse_headers((bus, slot, func): (u8, u8, u8), offset: u8, hdr: &mut PCICommonHeader) {
    let tuple = (bus, slot, func);

    // Only things certain about a PCI configuration header.
    let cap_id = pci_config_read_word(tuple, offset) as u8;
    let offset = match cap_id {
        CAPABILITY_MSI => {
            let control = pci_config_read_word(tuple, offset + 0x2);
            // Per-vector masking.
            if control & MSI_PER_VECTOR_MASKING as u16 != 0 {
                let msi = Msi::from_cap_ptr((bus, slot, func), offset);
                debugln!("{:#x?}", msi);
                msi.next_ptr
            } else {
                let msi = MsiNoMask::from_cap_ptr((bus, slot, func), offset);
                debugln!("{:#x?}", msi);
                msi.next_ptr
            }
        }
        CAPABILITY_SATA => {
            let sata = Sata::from_cap_ptr((bus, slot, func), offset);
            debugln!("{:#x?}", sata);
            dispatch_sata(hdr, sata.bar_offset());

            sata.next_ptr
        }
        CAPABILITY_MSI_X => {
            let msi_x = Msi_X::from_cap_ptr((bus, slot, func), offset);
            debugln!("{:x?}", msi_x);
            msi_x.next_ptr
        }
        _ => 0,
    };
    if offset != 0 {
        traverse_headers((bus, slot, func), offset, hdr);
    }
}
impl core::fmt::Display for PCICommonHeader {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_fmt(format_args!(
            "{:x}:{:x} {:x}:{:x} {:x}:{:x}:{:x} (rev {:x}) header type {:x}",
            self.vendor_id,
            self.device_id,
            self.subsystem_vendor_id,
            self.subsystem_id,
            self.base_class,
            self.subclass,
            self.prog_if,
            self.revision_id,
            self.header_type
        ))
    }
}
impl core::fmt::Debug for PCICommonHeader {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("PCICommonHeader")
            .field("vendor_id", &self.vendor_id)
            .field("device_id", &self.device_id)
            /* command */
            .field("status register", &self.status)
            .field("revision_id", &self.revision_id)
            .field("prog_if", &self.prog_if)
            .field("subclass", &self.subclass)
            .field("base_class", &self.base_class)
            /* cacheline size */
            .field("latency_timer", &self.latency_timer)
            .field("header_type", &self.header_type)
            /* BIST */
            .field("BAR", &self.base_address_registers)
            /* CardBus cis pointer */
            .field("subsystem_vendor_id", &self.subsystem_vendor_id)
            .field("subsystem_id", &self.subsystem_id)
            /* expansion ROM base address */
            .field("capabilities_pointer", &self.capabilities_pointer)
            /* Skip reserved */
            .field("interrupt_line", &self.interrupt_line)
            .field("interrupt_pin", &self.interrupt_pin)
            .field("min_gnt", &self.min_gnt)
            .field("max_latency", &self.max_latency)
            .finish()
    }
}
impl PCICommonHeader {
    fn from(bus: u8, device: u8, function: u8) -> Option<Self> {
        let tuple = (bus, device, function);
        let vendor_id = pci_get_vendor_id(tuple);
        if vendor_id.is_none() {
            return None;
        }
        let vendor_id = vendor_id.unwrap();
        let device_id = pci_get_device_id(bus, device);
        let command = pci_config_read_word(tuple, 0x4);
        let status = pci_config_read_word(tuple, 0x6);
        let revision_id = get_revision_id(tuple);
        let prog_if = get_prog_if(tuple);
        let base_class = get_base_class(tuple);
        let subclass = get_subclass(tuple);
        let cacheline_size = pci_config_read_byte(tuple, 0xC);
        let latency_timer = pci_config_read_byte(tuple, 0xD);
        let header_type = get_header_type(tuple);
        let bist = pci_config_read_byte(tuple, 0xF);
        let base_address_registers = [
            pci_config_read_long(tuple, 0x10),
            pci_config_read_long(tuple, 0x14),
            pci_config_read_long(tuple, 0x18),
            pci_config_read_long(tuple, 0x1C),
            pci_config_read_long(tuple, 0x20),
            pci_config_read_long(tuple, 0x24),
        ];
        let cardbus_cis_pointer = pci_config_read_long(tuple, 0x28);
        let subsystem_vendor_id = pci_config_read_word(tuple, 0x2C);
        let subsystem_id = pci_config_read_word(tuple, 0x2E);
        let expansion_rom_base_address = pci_config_read_long(tuple, 0x30);
        let capabilities_pointer = pci_config_read_byte(tuple, 0x34);
        let interrupt_line = pci_config_read_byte(tuple, 0x3C);
        let interrupt_pin = pci_config_read_byte(tuple, 0x3D);
        let min_gnt = pci_config_read_byte(tuple, 0x3E);
        let max_latency = pci_config_read_byte(tuple, 0x3F);
        Some(PCICommonHeader {
            vendor_id,
            device_id,
            command,
            status,
            revision_id,
            prog_if,
            base_class,
            subclass,
            cacheline_size,
            latency_timer,
            header_type,
            bist,
            base_address_registers,
            cardbus_cis_pointer,
            subsystem_vendor_id,
            subsystem_id,
            expansion_rom_base_address,
            capabilities_pointer,
            interrupt_line,
            interrupt_pin,
            min_gnt,
            max_latency,
            bus,
            device,
            function,
        })
    }
    fn as_pci_conf(&self, bus: u8, device: u8, function: u8) -> PciConf {
        PciConf {
            vendor_id: self.vendor_id,
            device_id: self.device_id,
            subsystem_vendor_id: self.subsystem_vendor_id,
            subsystem_id: self.subsystem_id,
            revision_id: self.revision_id,
            prog_if: self.prog_if,
            subclass: self.subclass,
            base_class: self.base_class,
            cacheline_size: self.cacheline_size,
            header_type: self.header_type,
            bus,
            device,
            function,
        }
    }
}

// Read a "word" from the PCI config. A "word" is 16 bits.
// An "offset" is 8 bits.
// That means that "offset 2" will give back bits [16, 31].
fn pci_config_read_word((bus, slot, func): (u8, u8, u8), offset: u8) -> u16 {
    // Address to send through the port.
    // 7-0 = register offset, but [1, 0] is always 0
    // 10-8 = function number
    // 15-11 = device number
    // 23-16 = bus number
    // 30-24 reserved
    // 31 - enable
    let address = ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((func as u32) << 8)
        | ((offset & 0xFC) as u32)
        | (1u32 << 31);

    /*
     * SAFETY: we trust the system integrator here that the ports return what
     * the spec says that they should. This might not always be the case.
     */
    unsafe {
        outl(CONFIG_ADDRESS, address);
    }
    unsafe { ((inl(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF) as u16 }
}

/*
 * Read a long (2 "word"s).
 */
fn pci_config_read_long(tuple: (u8, u8, u8), offset: u8) -> u32 {
    let a = pci_config_read_word(tuple, offset) as u32;
    let b = pci_config_read_word(tuple, offset + 2) as u32;
    a | (b << 16)
}
fn pci_config_read_byte(tuple: (u8, u8, u8), offset: u8) -> u8 {
    return if offset % 2 == 1 {
        pci_config_read_word(tuple, offset - 1) >> 8
    } else {
        pci_config_read_word(tuple, offset)
    } as u8;
}

fn pci_get_vendor_id(tuple: (u8, u8, u8)) -> Option<u16> {
    let vendor = pci_config_read_word(tuple, 0);

    if vendor != 0xFFFF { Some(vendor) } else { None }
}
fn get_header_type(tuple: (u8, u8, u8)) -> u8 {
    pci_config_read_byte(tuple, 0xe)
}
fn pci_get_device_id(bus: u8, slot: u8) -> u16 {
    pci_config_read_word((bus, slot, 0), 2)
}
fn get_revision_id(tuple: (u8, u8, u8)) -> u8 {
    pci_config_read_byte(tuple, 8)
}
pub fn enable_command_bus_master(hdr: &mut PCICommonHeader) {
    hdr.command |= COMMAND_BUS_MASTER;
}
pub fn disable_intr(hdr: &mut PCICommonHeader) {
    hdr.command |= COMMAND_INTERRUPT_DISABLE;
}

pub fn enable_intr(hdr: &mut PCICommonHeader) {
    hdr.command &= !COMMAND_INTERRUPT_DISABLE;
}
/* Get the "num"th bit of "bits", starting from 0.  */
macro_rules! bit {
    ($bits:expr, $num:expr) => {
        $bits & (1 << $num) != 0
    };
}

fn pci_status(status: u16) {
    debugln!(
        "Cap: {} 66MHz: {} FastB2B: {} MParityErr: {} DEVSEL={} SifTAbort: {} RecTAbort: {} RecMAbort: {} SigSysError: {} ParityErr {}",
        bit!(status, 4),
        bit!(status, 5),
        bit!(status, 7),
        bit!(status, 8),
        match (status >> 9) & 0xb11 {
            0b00 => {
                "Fast"
            }
            0b01 => {
                "Medium"
            }
            0b10 => {
                "Slow"
            }
            _ => {
                "Unknown"
            }
        },
        bit!(status, 11),
        bit!(status, 12),
        bit!(status, 13),
        bit!(status, 14),
        bit!(status, 15),
    );
}

// Also known as the command register.
fn pci_control(control: u16) {
    debugln!("I/O space: {}", bit!(control, 0));
    debugln!("MemSpace: {}", bit!(control, 1));
    debugln!("BusMaster: {}", bit!(control, 2));
    debugln!("SpecCycle: {}", bit!(control, 3));
    debugln!("memory WInv: {}", bit!(control, 4));
    debugln!("VGASnoop: {}", bit!(control, 5));
    debugln!("ParErr: {}", bit!(control, 6));
    debugln!("SERR#: {}", bit!(control, 8));
    debugln!("FastB2B: {}", bit!(control, 9));
    debugln!("IntOff: {}", bit!(control, 10));
}

fn command_and_status_registers(hdr: &mut PCICommonHeader) {
    debugln!("Cap ptr: {:x}", hdr.capabilities_pointer);
    debugln!("== Command Register ==");
    pci_control(hdr.command);
    debugln!("== Status Register ==");
    pci_status(hdr.status);
    debugln!(
        "intr: line {} pin {}",
        hdr.interrupt_line,
        hdr.interrupt_pin
    );
    debugln!("HDR {:#x?}", hdr);
}

fn check_device(bus: u8, device: u8) {
    let vendor_id = pci_get_vendor_id((bus, device, 0));
    if vendor_id.is_none() {
        return;
    }
    let header_type = get_header_type((bus, device, 0));
    if (header_type & 0x80) != 0 {
        for func in 1..8 {
            let dev = &mut PCICommonHeader::from(bus, device, func);
            if let Some(dev) = dev {
                info_headers((bus, device, func), dev.capabilities_pointer, dev);
                let mut pci_confs = PCI_CONFS.lock();
                pci_confs.push(dev.as_pci_conf(bus, device, func));
            }
        }
    } else {
        let dev = &mut PCICommonHeader::from(bus, device, 0);
        if let Some(dev) = dev {
            info_headers((bus, device, 0), dev.capabilities_pointer, dev);
            let mut pci_confs = PCI_CONFS.lock();
            pci_confs.push(dev.as_pci_conf(bus, device, 0));
        }
    }
}
fn get_prog_if(tuple: (u8, u8, u8)) -> u8 {
    pci_config_read_byte(tuple, 0x9)
}
fn get_subclass(tuple: (u8, u8, u8)) -> u8 {
    (pci_config_read_word(tuple, 0xa)) as u8
}
fn get_base_class(tuple: (u8, u8, u8)) -> u8 {
    pci_config_read_byte(tuple, 0xb)
}

fn check_bus(bus: u8) {
    for device in 0..32 {
        check_device(bus, device);
    }
}

fn check_function(tuple: (u8, u8, u8)) -> (u8, u8) {
    let base_class = get_base_class(tuple);
    let subclass = get_subclass(tuple);
    let header = get_header_type(tuple);
    // 0x6 == bridge, 0x4 == PCI-to-PCI bridge
    // PCI-to-PCI bridge header == 0x1
    if base_class == 0x6 && subclass == 0x4 && header == 0x1 {
        let secondary_bus = pci_config_read_byte(tuple, 0x15);
        check_bus(secondary_bus);
    }
    (base_class, subclass)
}

fn check_all_buses() {
    let header_type = get_header_type((0, 0, 0));
    if (header_type & 0x80) == 0 {
        check_bus(0);
    } else {
        for function in 0..8 {
            if let Some(_) = pci_get_vendor_id((0, 0, function)) {
                break;
            }
            check_bus(function);
        }
    }
}

#[allow(unused)]
fn pci_brute_force_scan() {
    for bus in 0..=255 {
        for device in 0..32 {
            let dev = PCICommonHeader::from(bus, device, 0);
            if let Some(dev) = dev {
                println!("{}", dev);
            }
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct FatPointerArray_pci_conf {
    ptr: *mut PciConf,
    len: usize,
}

#[unsafe(no_mangle)]
pub extern "C" fn pci_get_conf() -> FatPointerArray_pci_conf {
    let pci_confs = PCI_CONFS.lock();
    let (ptr, len, _) = pci_confs.clone().into_raw_parts();
    FatPointerArray_pci_conf { ptr, len }
}

/*
 * Currently, these do not get added to any structure. They just get printed.
 */
#[unsafe(no_mangle)]
pub extern "C" fn pci_init() {
    check_all_buses();
    let pci_confs = PCI_CONFS.lock();
    if pci_confs.len() == 0 {
        // no devices found
    }
}
