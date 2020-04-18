// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#ifndef LIBSPIRV_OPERAND_H_
#define LIBSPIRV_OPERAND_H_

#include <deque>

#include "spirv-tools/libspirv.h"
#include "table.h"

// A sequence of operand types.
//
// A SPIR-V parser uses an operand pattern to describe what is expected
// next on the input.
//
// As we parse an instruction in text or binary form from left to right,
// we pull and push from the front of the pattern.
using spv_operand_pattern_t = std::deque<spv_operand_type_t>;

// Finds the named operand in the table. The type parameter specifies the
// operand's group. A handle of the operand table entry for this operand will
// be written into *entry.
spv_result_t spvOperandTableNameLookup(const spv_operand_table table,
                                       const spv_operand_type_t type,
                                       const char* name,
                                       const size_t name_length,
                                       spv_operand_desc* entry);

// Finds the operand with value in the table. The type parameter specifies the
// operand's group. A handle of the operand table entry for this operand will
// be written into *entry.
spv_result_t spvOperandTableValueLookup(const spv_operand_table table,
                                        const spv_operand_type_t type,
                                        const uint32_t value,
                                        spv_operand_desc* entry);

// Gets the name string of the non-variable operand type.
const char* spvOperandTypeStr(spv_operand_type_t type);

// Returns true if the given type is a concrete and also a mask.
bool spvOperandIsConcreteMask(spv_operand_type_t type);

// Returns true if an operand of the given type is optional.
bool spvOperandIsOptional(spv_operand_type_t type);

// Returns true if an operand type represents zero or more logical operands.
//
// Note that a single logical operand may still be a variable number of words.
// For example, a literal string may be many words, but is just one logical
// operand.
bool spvOperandIsVariable(spv_operand_type_t type);

// Inserts a list of operand types into the front of the given pattern.
// The types parameter specifies the source array of types, ending with
// SPV_OPERAND_TYPE_NONE.
void spvPrependOperandTypes(const spv_operand_type_t* types,
                            spv_operand_pattern_t* pattern);

// Inserts the operands expected after the given typed mask onto the
// front of the given pattern.
//
// Each set bit in the mask represents zero or more operand types that should
// be prepended onto the pattern.  Operands for a less significant bit always
// appear before operands for a more significant bit.
//
// If a set bit is unknown, then we assume it has no operands.
void spvPrependOperandTypesForMask(const spv_operand_table operand_table,
                                   const spv_operand_type_t mask_type,
                                   const uint32_t mask,
                                   spv_operand_pattern_t* pattern);

// Expands an operand type representing zero or more logical operands,
// exactly once.
//
// If the given type represents potentially several logical operands,
// then prepend the given pattern with the first expansion of the logical
// operands, followed by original type.  Otherwise, don't modify the pattern.
//
// For example, the SPV_OPERAND_TYPE_VARIABLE_ID represents zero or more
// IDs.  In that case we would prepend the pattern with SPV_OPERAND_TYPE_ID
// followed by SPV_OPERAND_TYPE_VARIABLE_ID again.
//
// This also applies to zero or more tuples of logical operands.  In that case
// we prepend pattern with for the members of the tuple, followed by the
// original type argument.  The pattern must encode the fact that if any part
// of the tuple is present, then all tuple members should be.  So the first
// member of the tuple must be optional, and the remaining members
// non-optional.
//
// Returns true if we modified the pattern.
bool spvExpandOperandSequenceOnce(spv_operand_type_t type,
                                  spv_operand_pattern_t* pattern);

// Expands the first element in the pattern until it is a matchable operand
// type, then pops it off the front and returns it.  The pattern must not be
// empty.
//
// A matchable operand type is anything other than a zero-or-more-items
// operand type.
spv_operand_type_t spvTakeFirstMatchableOperand(spv_operand_pattern_t* pattern);

// Calculates the corresponding post-immediate alternate pattern, which allows
// a limited set of operand types.
spv_operand_pattern_t spvAlternatePatternFollowingImmediate(
    const spv_operand_pattern_t& pattern);

// Is the operand an ID?
bool spvIsIdType(spv_operand_type_t type);

#endif  // LIBSPIRV_OPERAND_H_
