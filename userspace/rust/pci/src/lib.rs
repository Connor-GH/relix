#![no_std]
#![feature(vec_into_raw_parts)]
use core::ffi::c_char;
use userspace_bindings::pci::PciConf;
extern crate alloc;
use alloc::ffi::CString;
#[cfg(feature = "pci-ids")]
use pci_ids::{Device, FromId};

#[cfg(not(feature = "pci-ids"))]
fn internal_libpci_device_info_empty(
    _vendor_id: u16,
    _device_id: u16,
    _subsystem_vendor_id: u16,
    _subsystem_id: u16,
    _base_class: u8,
    _subclass: u8,
    _prog_if: u8,
) -> Option<*mut *mut c_char> {
    let mut slice = alloc::vec::Vec::with_capacity(6);
    slice.push(CString::new("").ok()?.into_raw());
    slice.push(CString::new("").ok()?.into_raw());
    slice.push(CString::new("").ok()?.into_raw());
    slice.push(CString::new("").ok()?.into_raw());
    slice.push(CString::new("").ok()?.into_raw());
    slice.push(CString::new("").ok()?.into_raw());

    // Important for freeing the memory later.
    assert!(slice.len() == 6 && slice.capacity() == 6);
    Some(slice.into_raw_parts().0)
}

fn internal_libpci_device_info(
    vendor_id: u16,
    device_id: u16,
    subsystem_vendor_id: u16,
    subsystem_id: u16,
    base_class: u8,
    subclass: u8,
    prog_if: u8,
) -> Option<*mut *mut c_char> {
#[cfg(feature = "pci-ids")] {
        internal_libpci_device_info_pci_ids(
            vendor_id,
            device_id,
            subsystem_vendor_id,
            subsystem_id,
            base_class,
            subclass,
            prog_if,
        )
    }
#[cfg(not(feature = "pci-ids"))] {
        internal_libpci_device_info_empty(
            vendor_id,
            device_id,
            subsystem_vendor_id,
            subsystem_id,
            base_class,
            subclass,
            prog_if,
        )
    }
}

#[cfg(feature = "pci-ids")]
fn internal_libpci_device_info_pci_ids(
    vendor_id: u16,
    device_id: u16,
    subsystem_vendor_id: u16,
    subsystem_id: u16,
    base_class: u8,
    subclass: u8,
    prog_if: u8,
) -> Option<*mut *mut c_char> {
    let device = Device::from_vid_pid(vendor_id, device_id);
    let device_name = device.map_or_else(|| "Device", |device| device.name());
    let device_vendor_name = if device.is_some() {
        device.unwrap().vendor().name()
    } else {
        "Vendor"
    };

    let subsys_device = if device.is_some() {
        device.unwrap().subsystems().find(|subsys| {
            subsys.subvendor() == subsystem_vendor_id && subsys.subdevice() == subsystem_id
        })
    } else {
        None
    };
    let subsys_device_name = if subsys_device.is_some() {
        subsys_device.unwrap().name()
    } else {
        "Subsystem Device"
    };

    let subsys_vendor = if subsys_device.is_some() {
        pci_ids::Vendor::from_id(subsys_device.unwrap().subvendor())
    } else {
        None
    };
    let subsys_vendor_name = if subsys_vendor.is_some() {
        subsys_vendor.unwrap().name()
    } else {
        "Subsystem Vendor"
    };

    let subclass = pci_ids::Subclass::from_cid_sid(base_class, subclass);
    let subclass_string = subclass.map_or_else(|| "Subclass", |subclass| subclass.name());

    let prog_if = subclass.map_or_else(
        || "",
        |subclass| {
            subclass
                .prog_ifs()
                .find(|prog| prog.id() == prog_if)
                .map_or_else(|| "", |s| s.name())
        },
    );
    let mut slice = alloc::vec::Vec::with_capacity(6);
    slice.push(CString::new(device_vendor_name).ok()?.into_raw());
    slice.push(CString::new(device_name).ok()?.into_raw());
    slice.push(CString::new(subsys_vendor_name).ok()?.into_raw());
    slice.push(CString::new(subsys_device_name).ok()?.into_raw());
    slice.push(CString::new(subclass_string).ok()?.into_raw());
    slice.push(CString::new(prog_if).ok()?.into_raw());

    // Important for freeing the memory later.
    assert!(slice.len() == 6 && slice.capacity() == 6);
    Some(slice.into_raw_parts().0)
}
#[unsafe(no_mangle)]
pub extern "C" fn libpci_device_info_alloc(pci: PciConf) -> *mut *mut c_char {
    let ret = internal_libpci_device_info(
        pci.vendor_id,
        pci.device_id,
        pci.subsystem_vendor_id,
        pci.subsystem_id,
        pci.base_class,
        pci.subclass,
        pci.prog_if,
    );
    match ret {
        Some(s) => s,
        _ => core::ptr::null_mut(),
    }
}
#[unsafe(no_mangle)]
// SAFETY: arr must not have been modified by C code.
// There needs to be a "from_raw" for every "into_raw".
pub extern "C" fn libpci_device_info_free(arr: *mut *mut c_char) {
    // Memory from the Vec gets dropped at the end of the scope.
    let v = unsafe { alloc::vec::Vec::<*mut i8>::from_raw_parts(arr, 6, 6) };

    // Reclaim the CString memory.
    for str in v {
        let _s = unsafe { CString::from_raw(str) };
    }
}
