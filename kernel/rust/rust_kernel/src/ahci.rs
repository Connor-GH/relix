use crate::pci::{disable_intr, enable_command_bus_master, PCICommonHeader};

pub fn ahci_init(hdr: &mut PCICommonHeader) {
    // Enable "bus master"
    enable_command_bus_master(hdr);
    disable_intr(hdr);
}
