#pragma once
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
#include <stdint.h>

struct CSourceLocation {
	char *filename;
	uint32_t line;
	uint32_t column;
};

struct CTypeDescriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[];
};

struct COverflowData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
};

struct CUnreachableData {
	struct CSourceLocation location;
};

struct CCFICheckFailData {
	uint8_t check_kind;
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
};

struct CDynamicTypeCacheMissData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
	void *type_info;
	uint8_t typecheck_kind;
};

struct CFunctionTypeMismatchData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
};

struct CInvalidBuiltinData {
	struct CSourceLocation location;
	uint8_t kind;
};

struct CInvalidValueData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
};

struct CNonNullArgData {
	struct CSourceLocation location;
	struct CSourceLocation attribute_location;
	int arg_index;
};

struct CNonNullReturnData {
	struct CSourceLocation attribute_location;
};

struct COutOfBoundsData {
	struct CSourceLocation location;
	struct CTypeDescriptor *array_type;
	struct CTypeDescriptor *index_type;
};

struct CPointerOverflowData {
	struct CSourceLocation location;
};

struct CShiftOutOfBoundsData {
	struct CSourceLocation location;
	struct CTypeDescriptor *lhs_type;
	struct CTypeDescriptor *rhs_type;
};

struct CTypeMismatchData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
	unsigned long log_alignment;
	uint8_t typecheck_kind;
};

struct CTypeMismatchDataV1 {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
	uint8_t log_alignment;
	uint8_t typecheck_kind;
};

struct CVLABoundData {
	struct CSourceLocation location;
	struct CTypeDescriptor *type;
};

struct CFloatCastOverflowData {
	struct CSourceLocation location; /* This field exists in this struct since
	                                    2015 August 11th */
	struct CTypeDescriptor *from_type;
	struct CTypeDescriptor *to_type;
};

struct CImplicitConversionData {
	struct CSourceLocation location;
	struct CTypeDescriptor *from_type;
	struct CTypeDescriptor *to_type;
	uint8_t kind;
};

struct CAlignmentAssumptionData {
	struct CSourceLocation location;
	struct CSourceLocation assumption_location;
	struct CTypeDescriptor *type;
};
