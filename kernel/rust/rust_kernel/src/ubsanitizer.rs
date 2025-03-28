/*
 * This code uses the struct definitions based on
 * NetBSD. The language is obviously different, but
 * I will nevertheless provide the copyright notice.
 */
/*	$NetBSD: ubsan.c,v 1.12 2023/12/07 07:10:44 andvar Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES); LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

use crate::printing::*;
use bindings::__IncompleteArrayField;
use core::ffi::CStr;
use core::ffi::{c_char, c_void};

#[repr(C)]
pub struct CSourceLocation {
    filename: *mut c_char,
    line: u32,
    column: u32,
}

#[repr(C)]
pub struct CTypeDescriptor {
    type_kind: u16,
    type_info: u16,
    type_name: __IncompleteArrayField<c_char>,
}

#[repr(C)]
pub struct COverflowData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CUnreachableData {
    location: CSourceLocation,
}

#[repr(C)]
pub struct CCFICheckFailData {
    check_kind: u8,
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CDynamicTypeCacheMissData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
    type_info: *mut c_void,
    typecheck_kind: u8,
}

#[repr(C)]
pub struct CFunctionTypeMismatchData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CInvalidBuiltinData {
    location: CSourceLocation,
    kind: u8,
}

#[repr(C)]
pub struct CInvalidValueData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CNonNullArgData {
    location: CSourceLocation,
    attribute_location: CSourceLocation,
    arg_index: i32,
}

#[repr(C)]
pub struct CNonNullReturnData {
    attribute_location: CSourceLocation,
}

#[repr(C)]
pub struct COutOfBoundsData {
    location: CSourceLocation,
    array_type: *mut CTypeDescriptor,
    index_type: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CPointerOverflowData {
    location: CSourceLocation,
}

#[repr(C)]
pub struct CShiftOutOfBoundsData {
    location: CSourceLocation,
    lhs_type: *mut CTypeDescriptor,
    rhs_type: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CTypeMismatchData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
    log_alignment: u64,
    typecheck_kind: u8,
}

#[repr(C)]
pub struct CTypeMismatchDataV1 {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
    log_alignment: u8,
    typecheck_kind: u8,
}

#[repr(C)]
pub struct CVLABoundData {
    location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CFloatCastOverflowData {
    location: CSourceLocation, /* This field exists in this struct since 2015 August 11th */
    from_type: *mut CTypeDescriptor,
    to_type: *mut CTypeDescriptor,
}

#[repr(C)]
pub struct CImplicitConversionData {
    location: CSourceLocation,
    from_type: *mut CTypeDescriptor,
    to_type: *mut CTypeDescriptor,
    kind: u8,
}

#[repr(C)]
pub struct CAlignmentAssumptionData {
    location: CSourceLocation,
    assumption_location: CSourceLocation,
    type_: *mut CTypeDescriptor,
}

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
pub extern "C" fn __ubsan_handle_add_overflow_abort(_data: *mut COverflowData, _lhs: u64, _rhs: u64) {
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
pub extern "C" fn __ubsan_handle_cfi_check_fail(_data: *mut CCFICheckFailData, _value: u64, _valid_vtable: u64) {
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
pub extern "C" fn __ubsan_handle_divrem_overflow_abort(_data: *mut COverflowData, _lhs: u64, _rhs: u64) {
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
pub extern "C" fn __ubsan_handle_float_cast_overflow(_data: *mut CFloatCastOverflowData, _from: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_float_cast_overflow_abort(_data: *mut CFloatCastOverflowData, _from: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_function_type_mismatch(_data: *mut CFunctionTypeMismatchData, _function: u64) {
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
pub extern "C" fn __ubsan_handle_load_invalid_value_abort(_data: *mut CInvalidValueData, _val: u64) {
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
    println!("index_type {} {} {}", index_type.type_kind, index_type.type_info,
                unsafe { core::ffi::CStr::from_ptr(index_type.type_name.as_ptr()) }.to_str().unwrap() );
    println!("array_type {} {} {}", array_type.type_kind, array_type.type_info,
                unsafe { core::ffi::CStr::from_ptr(array_type.type_name.as_ptr()) }.to_str().unwrap() );
    println!("tried index {}", index);
    print_location(&data.location);
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_out_of_bounds_abort(_data: *mut COutOfBoundsData, _index: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_pointer_overflow(data: *mut CPointerOverflowData, base: u64, result: u64) {
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
pub extern "C" fn __ubsan_handle_shift_out_of_bounds(data: *mut CShiftOutOfBoundsData, lhs: u64, rhs: u64) {
    let deref = unsafe { &*data};
    let lhs_type = unsafe { &*deref.lhs_type };
    let rhs_type = unsafe { &*deref.rhs_type };
    println!("lhs_type {} {} {}", lhs_type.type_kind, lhs_type.type_info,
                unsafe { core::ffi::CStr::from_ptr(lhs_type.type_name.as_ptr()) }.to_str().unwrap() );
    println!("rhs_type {} {} {}", rhs_type.type_kind, rhs_type.type_info,
                unsafe { core::ffi::CStr::from_ptr(rhs_type.type_name.as_ptr()) }.to_str().unwrap() );
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
pub extern "C" fn __ubsan_handle_sub_overflow_abort(_data: *mut COverflowData, _lhs: u64, _rhs: u64) {
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
pub extern "C" fn __ubsan_handle_type_mismatch_v1_abort(_data: *mut CTypeMismatchDataV1, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_vla_bound_not_positive(_data: *mut CVLABoundData, _bound: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_vla_bound_not_positive_abort(_data: *mut CVLABoundData, _bound: u64) {
    todo!();
}
#[unsafe(no_mangle)]
pub extern "C" fn __ubsan_handle_implicit_conversion(_data: *mut CImplicitConversionData, _from: u64, _to: u64) {
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
