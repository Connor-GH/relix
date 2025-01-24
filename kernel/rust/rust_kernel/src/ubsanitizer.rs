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
 * SUBSTITUTE GOODS OR SERVICES { todo!(); } LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

use crate::printing::*;
use core::ffi::CStr;
use core::ffi::{c_char, c_void};

#[repr(C)]
struct CSourceLocation {
    filename: *const c_char,
    line: u32,
    column: u32,
}

#[repr(C)]
struct CTypeDescriptor {
    type_kind: u16,
    type_info: u16,
    type_name: [u8; 1],
}

#[repr(C)]
struct COverflowData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
}

#[repr(C)]
struct CUnreachableData {
    location: CSourceLocation,
}

#[repr(C)]
struct CCFICheckFailData {
    check_kind: u8,
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
}

#[repr(C)]
struct CDynamicTypeCacheMissData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
    type_info: *const c_void,
    typecheck_kind: u8,
}

#[repr(C)]
struct CFunctionTypeMismatchData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
}

#[repr(C)]
struct CInvalidBuiltinData {
    location: CSourceLocation,
    kind: u8,
}

#[repr(C)]
struct CInvalidValueData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
}

#[repr(C)]
struct CNonNullArgData {
    location: CSourceLocation,
    attribute_location: CSourceLocation,
    arg_index: i32,
}

#[repr(C)]
struct CNonNullReturnData {
    attribute_location: CSourceLocation,
}

#[repr(C)]
struct COutOfBoundsData {
    location: CSourceLocation,
    array_type: *const CTypeDescriptor,
    index_type: *const CTypeDescriptor,
}

#[repr(C)]
struct CPointerOverflowData {
    location: CSourceLocation,
}

#[repr(C)]
struct CShiftOutOfBoundsData {
    location: CSourceLocation,
    lhs_type: *const CTypeDescriptor,
    rhs_type: *const CTypeDescriptor,
}

#[repr(C)]
struct CTypeMismatchData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
    log_alignment: u64,
    typecheck_kind: u8,
}

#[repr(C)]
struct CTypeMismatchDataV1 {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
    log_alignment: u8,
    typecheck_kind: u8,
}

#[repr(C)]
struct CVLABoundData {
    location: CSourceLocation,
    type_: *const CTypeDescriptor,
}

#[repr(C)]
struct CFloatCastOverflowData {
    location: CSourceLocation, /* This field exists in this struct since 2015 August 11th */
    from_type: *const CTypeDescriptor,
    to_type: *const CTypeDescriptor,
}

#[repr(C)]
struct CImplicitConversionData {
    location: CSourceLocation,
    from_type: *const CTypeDescriptor,
    to_type: *const CTypeDescriptor,
    kind: u8,
}

#[repr(C)]
struct CAlignmentAssumptionData {
    location: CSourceLocation,
    assumption_location: CSourceLocation,
    type_: *const CTypeDescriptor,
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
fn __ubsan_handle_add_overflow(data: *const COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: add overflow: {} + {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_add_overflow_abort(_data: *const COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_alignment_assumption(
    _data: *const CAlignmentAssumptionData,
    _ptr: u64,
    _alignment: u64,
    _offset: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_alignment_assumption_abort(
    _data: *const CAlignmentAssumptionData,
    _ptr: u64,
    _alignment: u64,
    _offset: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_builtin_unreachable(_data: *const CUnreachableData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_cfi_bad_type(
    _data: *const CCFICheckFailData,
    _vtable: u64,
    _is_valid_vtable: bool,
    _from_unrecoverable_handler: bool,
    _program_counter: u64,
    _frame_pointer: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_cfi_check_fail(_data: *const CCFICheckFailData, _value: u64, _valid_vtable: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_cfi_check_fail_abort(
    _data: *const CCFICheckFailData,
    _value: u64,
    _valid_vtable: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_divrem_overflow(data: *const COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: divrem overflow: {} / {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_divrem_overflow_abort(_data: *const COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_dynamic_type_cache_miss(
    _data: *const CDynamicTypeCacheMissData,
    _ptr: u64,
    _hash: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_dynamic_type_cache_miss_abort(
    _data: *const CDynamicTypeCacheMissData,
    _ptr: u64,
    _hash: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_float_cast_overflow(_data: *const CFloatCastOverflowData, _from: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_float_cast_overflow_abort(_data: *const CFloatCastOverflowData, _from: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_function_type_mismatch(_data: *const CFunctionTypeMismatchData, _function: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_function_type_mismatch_abort(
    _data: *const CFunctionTypeMismatchData,
    _function: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_function_type_mismatch_v1(
    _data: *const CFunctionTypeMismatchData,
    _function: u64,
    _callee_rtti: u64,
    _fn_rtti: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_function_type_mismatch_v1_abort(
    _data: *const CFunctionTypeMismatchData,
    _function: u64,
    _callee_rtti: u64,
    _fn_rtti: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_invalid_builtin(_data: *const CInvalidBuiltinData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_invalid_builtin_abort(_data: *const CInvalidBuiltinData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_load_invalid_value(data: *const CInvalidValueData, val: u64) {
    println!("KUBSAN: load-invalid-value: {}", val);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_load_invalid_value_abort(_data: *const CInvalidValueData, _val: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_missing_return(_data: *const CUnreachableData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_moverflow(_data: *const COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_moverflow_abort(_data: *const COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_negate_overflow(_data: *const COverflowData, _old_val: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_negate_overflow_abort(_data: *const COverflowData, _old_val: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nonnull_arg(data: *const CNonNullArgData) {
    println!(
        "KUBSAN: null pointer passed as argument {}, which is declared to never be null",
        (unsafe { &*data }).arg_index
    );
    print_location(&(unsafe { &*data }).location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nonnull_arg_abort(_data: *const CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nonnull_return_v1(
    _data: *const CNonNullReturnData,
    _location_ptr: *const CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nonnull_return_v1_abort(
    _data: *const CNonNullReturnData,
    _location_ptr: *const CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nullability_arg(_data: *const CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nullability_arg_abort(_data: *const CNonNullArgData) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nullability_return_v1(
    _data: *const CNonNullReturnData,
    _location_ptr: *const CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_nullability_return_v1_abort(
    _data: *const CNonNullReturnData,
    _location_ptr: *const CSourceLocation,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_out_of_bounds(_data: *const COutOfBoundsData, _index: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_out_of_bounds_abort(_data: *const COutOfBoundsData, _index: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_pointer_overflow(data: *const CPointerOverflowData, base: u64, result: u64) {
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
fn __ubsan_handle_pointer_overflow_abort(
    _data: *const CPointerOverflowData,
    _base: u64,
    _result: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_shift_out_of_bounds(_data: *const CShiftOutOfBoundsData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_shift_out_of_bounds_abort(
    _data: *const CShiftOutOfBoundsData,
    _lhs: u64,
    _rhs: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_sub_overflow(data: *const COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: sub overflow: {} - {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_sub_overflow_abort(_data: *const COverflowData, _lhs: u64, _rhs: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_mul_overflow(data: *const COverflowData, lhs: u64, rhs: u64) {
    println!("KUBSAN: mul overflow: {} * {}", lhs, rhs);
    print_location(&unsafe { &*data }.location);
}
#[unsafe(no_mangle)]
fn __ubsan_handle_type_mismatch(_data: *const CTypeMismatchData, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_type_mismatch_abort(_data: *const CTypeMismatchData, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_type_mismatch_v1(data: *const CTypeMismatchDataV1, ptr: u64) {
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
fn __ubsan_handle_type_mismatch_v1_abort(_data: *const CTypeMismatchDataV1, _ptr: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_vla_bound_not_positive(_data: *const CVLABoundData, _bound: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_vla_bound_not_positive_abort(_data: *const CVLABoundData, _bound: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_implicit_conversion(_data: *const CImplicitConversionData, _from: u64, _to: u64) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_handle_implicit_conversion_abort(
    _data: *const CImplicitConversionData,
    _from: u64,
    _to: u64,
) {
    todo!();
}
#[unsafe(no_mangle)]
fn __ubsan_get_current_report_data(
    _out_issue_kind: *const *const c_char,
    _out_message: *const *const c_char,
    _out_filename: *const *const c_char,
    _out_line: *const u32,
    _out_col: *const u32,
    _out_memory_addr: *mut *mut c_char,
) {
    todo!();
}
