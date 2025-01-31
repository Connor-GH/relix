// https://wiki.osdev.org/PCI
use crate::printing::*;
use bindings::x86::{inl, outl};
const CONFIG_ADDRESS: u16 = 0xCF8;
const CONFIG_DATA: u16 = 0xCFC;

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
}
impl core::fmt::Display for PCICommonHeader {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_fmt(format_args!(
            "{:x}:{:x} {:x}:{:x} {:x} base_class {:x} subclass {:x} header type {:x}",
            self.vendor_id,
            self.device_id,
            self.subsystem_vendor_id,
            self.subsystem_id,
            self.revision_id,
            self.base_class,
            self.subclass,
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
            /* status */
            .field("revision_id", &self.revision_id)
            .field("prog_if", &self.prog_if)
            .field("subclass", &self.subclass)
            .field("base_class", &self.base_class)
            /* cacheline size */
            .field("latency_timer", &self.latency_timer)
            .field("header_type", &self.header_type)
            /* BIST */
            /* base address registers (BAR) */
            /* CardBus cis pointer */
            .field("subsystem_vendor_id", &self.subsystem_vendor_id)
            .field("subsystem_id", &self.subsystem_id)
            /* expansion ROM base address */
            /* capabilities pointer */
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
        let cacheline_size = pci_config_read_word(tuple, 0xC) as u8;
        let latency_timer = pci_config_read_word(tuple, 0xD) as u8;
        let header_type = get_header_type(tuple);
        let bist = pci_config_read_word(tuple, 0xF) as u8;
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
        let capabilities_pointer = pci_config_read_word(tuple, 0x34) as u8;
        let interrupt_line = pci_config_read_word(tuple, 0x3C) as u8;
        let interrupt_pin = pci_config_read_word(tuple, 0x3D) as u8;
        let min_gnt = pci_config_read_word(tuple, 0x3E) as u8;
        let max_latency = pci_config_read_word(tuple, 0x3F) as u8;
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
        })
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
        outl(address, CONFIG_ADDRESS);
    }
    unsafe { ((inl(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF) as u16 }
}

/*
 * Read a long (2 "word"s).
 */
fn pci_config_read_long(tuple: (u8, u8, u8), offset: u8) -> u32 {
    pci_config_read_word(tuple, offset) as u32
        & (pci_config_read_word(tuple, offset+2) as u32) << 16
}

fn pci_get_vendor_id(tuple: (u8, u8, u8)) -> Option<u16> {
    let vendor = pci_config_read_word(tuple, 0);

    if vendor != 0xFFFF { Some(vendor) } else { None }
}
fn get_header_type(tuple: (u8, u8, u8)) -> u8 {
    let header = pci_config_read_word(tuple, 0xe) as u8;
    header
}
fn pci_get_device_id(bus: u8, slot: u8) -> u16 {
    let device = pci_config_read_word((bus, slot, 0), 2);
    device
}
fn get_revision_id(tuple: (u8, u8, u8)) -> u8 {
    let revision = pci_config_read_word(tuple, 8);
    revision as u8
}
fn check_device(bus: u8, device: u8) {
    let vendor_id = pci_get_vendor_id((bus, device, 0));
    if vendor_id.is_none() {
        return;
    }
    let header_type = get_header_type((bus, device, 0));
    if (header_type & 0x80) != 0 {
        for func in 1..8 {
            let dev = PCICommonHeader::from(bus, device, func);
            if let Some(dev) = dev {
                println!("{}", dev);
            }
        }
    } else {
        let dev = PCICommonHeader::from(bus, device, 0);
        if let Some(dev) = dev {
            println!("{}", dev);
        }
    }
}
fn get_prog_if(tuple: (u8, u8, u8)) -> u8 {
    (pci_config_read_word(tuple, 5)) as u8
}
fn get_subclass(tuple: (u8, u8, u8)) -> u8 {
    (pci_config_read_word(tuple, 6)) as u8
}
fn get_base_class(tuple: (u8, u8, u8)) -> u8 {
    (pci_config_read_word(tuple, 7)) as u8
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
        let secondary_bus = (pci_config_read_word(tuple, 0x15) >> 8) as u8;
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

/*
 * Currently, these do not get added to any structure. They just get printed.
 */
#[unsafe(no_mangle)]
pub extern "C" fn pci_init() {
    check_all_buses();
}
