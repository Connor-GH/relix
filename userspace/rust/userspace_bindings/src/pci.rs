
#[derive(Clone)]
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
