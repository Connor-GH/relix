
#[derive(Clone)]
#[repr(C)]
pub struct PciConf {
  pub vendor_id: u16,
  pub device_id: u16,
  pub subsystem_vendor_id: u16,
  pub subsystem_id: u16,
  revision_id: u8,
  pub prog_if: u8,
  pub subclass: u8,
  pub base_class: u8,
  cacheline_size: u8,
  header_type: u8,
  function: u8,
  device: u8,
  bus: u8,
}
