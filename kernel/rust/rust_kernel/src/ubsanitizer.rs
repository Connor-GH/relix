/*
 * This code uses the struct definitions based on
 * NetBSD. The language is obviously different, but
 * I will nevertheless provide the copyright notice.
 */

use crate::printing::*;
use core::ffi::CStr;
use core::ffi::c_char;

use kernel_bindings::bindings::CAlignmentAssumptionData;
use kernel_bindings::bindings::CCFICheckFailData;
use kernel_bindings::bindings::CDynamicTypeCacheMissData;
use kernel_bindings::bindings::CFloatCastOverflowData;
use kernel_bindings::bindings::CFunctionTypeMismatchData;
use kernel_bindings::bindings::CImplicitConversionData;
use kernel_bindings::bindings::CInvalidBuiltinData;
use kernel_bindings::bindings::CInvalidValueData;
use kernel_bindings::bindings::CNonNullArgData;
use kernel_bindings::bindings::CNonNullReturnData;
use kernel_bindings::bindings::COutOfBoundsData;
use kernel_bindings::bindings::COverflowData;
use kernel_bindings::bindings::CPointerOverflowData;
use kernel_bindings::bindings::CShiftOutOfBoundsData;
use kernel_bindings::bindings::CSourceLocation;
use kernel_bindings::bindings::CTypeMismatchData;
use kernel_bindings::bindings::CTypeMismatchDataV1;
use kernel_bindings::bindings::CUnreachableData;
use kernel_bindings::bindings::CVLABoundData;

fn print_location(location: &CSourceLocation) {
    if location.filename.is_null() {
        println!("KUBSAN: in unknown file");
    } else {
        println!(
            "KUBSAN: at {}, line {}, _column: {}",
            unsafe { CStr::from_ptr(location.filename) }
                .to_str()
                .unwrap_or("unknown"),
            location.line,
            location.column
        );
        panic!("KUBSAN");
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_add_overflow(data: *mut COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: add overflow: {} + {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_add_overflow_abort(
    _data: *mut COverflowData,
    _lhs: u64,
    _rhs: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_alignment_assumption(
    _data: *mut CAlignmentAssumptionData,
    _ptr: u64,
    _alignment: u64,
    _offset: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_alignment_assumption_abort(
    _data: *mut CAlignmentAssumptionData,
    _ptr: u64,
    _alignment: u64,
    _offset: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_builtin_unreachable(_data: *mut CUnreachableData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_cfi_bad_type(
    _data: *mut CCFICheckFailData,
    _vtable: u64,
    _is_valid_vtable: bool,
    _from_unrecoverable_handler: bool,
    _program_counter: u64,
    _frame_pointer: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_cfi_check_fail(
    _data: *mut CCFICheckFailData,
    _value: u64,
    _valid_vtable: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_cfi_check_fail_abort(
    _data: *mut CCFICheckFailData,
    _value: u64,
    _valid_vtable: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_divrem_overflow(data: *mut COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: divrem overflow: {} / {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_divrem_overflow_abort(
    _data: *mut COverflowData,
    _lhs: u64,
    _rhs: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_dynamic_type_cache_miss(
    _data: *mut CDynamicTypeCacheMissData,
    _ptr: u64,
    _hash: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_dynamic_type_cache_miss_abort(
    _data: *mut CDynamicTypeCacheMissData,
    _ptr: u64,
    _hash: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_float_cast_overflow(
    _data: *mut CFloatCastOverflowData,
    _from: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_float_cast_overflow_abort(
    _data: *mut CFloatCastOverflowData,
    _from: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_function_type_mismatch(
    _data: *mut CFunctionTypeMismatchData,
    _function: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_function_type_mismatch_abort(
    _data: *mut CFunctionTypeMismatchData,
    _function: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_function_type_mismatch_v1(
    _data: *mut CFunctionTypeMismatchData,
    _function: u64,
    _callee_rtti: u64,
    _fn_rtti: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_function_type_mismatch_v1_abort(
    _data: *mut CFunctionTypeMismatchData,
    _function: u64,
    _callee_rtti: u64,
    _fn_rtti: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_invalid_builtin(_data: *mut CInvalidBuiltinData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_invalid_builtin_abort(_data: *mut CInvalidBuiltinData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_load_invalid_value(data: *mut CInvalidValueData, val: u64) {
    println!("KUBSAN: load-invalid-value: {}", val);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_load_invalid_value_abort(
    _data: *mut CInvalidValueData,
    _val: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_missing_return(_data: *mut CUnreachableData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_moverflow(_data: *mut COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_moverflow_abort(_data: *mut COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_negate_overflow(_data: *mut COverflowData, _old_val: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_negate_overflow_abort(_data: *mut COverflowData, _old_val: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nonnull_arg(data: *mut CNonNullArgData) {
    println!(
        "KUBSAN: null pointer passed as argument {}, which is declared to never be null",
        (unsafe { &*data }).arg_index
    );
    print_location(&(unsafe { &*data }).location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nonnull_arg_abort(_data: *mut CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nonnull_return_v1(
    _data: *mut CNonNullReturnData,
    _location_ptr: *mut CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nonnull_return_v1_abort(
    _data: *mut CNonNullReturnData,
    _location_ptr: *mut CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nullability_arg(_data: *mut CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nullability_arg_abort(_data: *mut CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nullability_return_v1(
    _data: *mut CNonNullReturnData,
    _location_ptr: *mut CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_nullability_return_v1_abort(
    _data: *mut CNonNullReturnData,
    _location_ptr: *mut CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_out_of_bounds(data: *mut COutOfBoundsData, index: u64) {
    let data = unsafe { &*data };
    let index_type = unsafe { &*data.index_type };
    let array_type = unsafe { &*data.array_type };
    println!(
        "index_type {} {} {}",
        index_type.type_kind,
        index_type.type_info,
        unsafe { core::ffi::CStr::from_ptr(index_type.type_name.as_ptr()) }
            .to_str()
            .unwrap()
    );
    println!(
        "array_type {} {} {}",
        array_type.type_kind,
        array_type.type_info,
        unsafe { core::ffi::CStr::from_ptr(array_type.type_name.as_ptr()) }
            .to_str()
            .unwrap()
    );
    println!("tried index {}", index);
    print_location(&data.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_out_of_bounds_abort(_data: *mut COutOfBoundsData, _index: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_pointer_overflow(
    data: *mut CPointerOverflowData,
    base: u64,
    result: u64,
) {
    if base == 0 && result == 0 {
        println!("KUBSAN: applied zero offset to nullptr");
    } else if base == 0 && result != 0 {
        println!("KUBSAN: applied nonzero offset {:x} to nullptr", result);
    } else if base != 0 && result == 0 {
        println!(
            "KUBSAN: applying nonzero offset to non-null pointer {:x} produced null pointer",
            base
        );
    } else {
        println!(
            "KUBSAN: addition of unsigned offset to {:x} overflowed to {:x}",
            base, result
        );
    }
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_pointer_overflow_abort(
    _data: *mut CPointerOverflowData,
    _base: u64,
    _result: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_shift_out_of_bounds(
    data: *mut CShiftOutOfBoundsData,
    lhs: u64,
    rhs: u64,
) {
    let deref = unsafe { &*data };
    let lhs_type = unsafe { &*deref.lhs_type };
    let rhs_type = unsafe { &*deref.rhs_type };
    println!(
        "lhs_type {} {} {}",
        lhs_type.type_kind,
        lhs_type.type_info,
        unsafe { core::ffi::CStr::from_ptr(lhs_type.type_name.as_ptr()) }
            .to_str()
            .unwrap()
    );
    println!(
        "rhs_type {} {} {}",
        rhs_type.type_kind,
        rhs_type.type_info,
        unsafe { core::ffi::CStr::from_ptr(rhs_type.type_name.as_ptr()) }
            .to_str()
            .unwrap()
    );
    println!("KUBSAN: shift out of bounds: {} shifted by {}", lhs, rhs);

    print_location(&deref.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_shift_out_of_bounds_abort(
    _data: *mut CShiftOutOfBoundsData,
    _lhs: u64,
    _rhs: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_sub_overflow(data: *mut COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: sub overflow: {} - {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_sub_overflow_abort(
    _data: *mut COverflowData,
    _lhs: u64,
    _rhs: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_mul_overflow(data: *mut COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: mul overflow: {} * {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_type_mismatch(_data: *mut CTypeMismatchData, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_type_mismatch_abort(_data: *mut CTypeMismatchData, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_type_mismatch_v1(data: *mut CTypeMismatchDataV1, ptr: u64) {
    const KINDS: &[&str] = &[
        "load of",
        "store to",
        "reference binding to",
        "member access within",
        "member call on",
        "constructor call on",
        "downcast of",
        "downcast of",
        "upcast of",
        "cast to virtual base of",
        "_Nonnull binding to",
        "dynamic operation on",
    ];
    let alignment = 1 << unsafe { &*data }.log_alignment;
    let kind = KINDS[unsafe { &*data }.typecheck_kind as usize];
    if ptr == 0 {
        println!("KUBSAN: {} null pointer", kind);
    } else if (ptr & (alignment - 1)) != 0 {
        println!("KUBSAN: {} misaligned address {:x}", kind, ptr);
    } else {
        println!("KUBSAN: {} address {:x} with insufficient space", kind, ptr);
    }
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_type_mismatch_v1_abort(
    _data: *mut CTypeMismatchDataV1,
    _ptr: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_vla_bound_not_positive(_data: *mut CVLABoundData, _bound: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_vla_bound_not_positive_abort(
    _data: *mut CVLABoundData,
    _bound: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_implicit_conversion(
    _data: *mut CImplicitConversionData,
    _from: u64,
    _to: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_implicit_conversion_abort(
    _data: *mut CImplicitConversionData,
    _from: u64,
    _to: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_get_current_report_data(
    _out_issue_kind: *mut *const c_char,
    _out_message: *mut *const c_char,
    _out_filename: *mut *const c_char,
    _out_line: *mut u32,
    _out_col: *mut u32,
    _out_memory_addr: *mut *mut c_char,
) {
    todo!();
}
