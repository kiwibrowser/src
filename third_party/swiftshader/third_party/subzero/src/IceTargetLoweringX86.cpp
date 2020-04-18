//===---- subzero/src/IceTargetLoweringX86.cpp - x86 lowering -*- C++ -*---===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Implements portions of the TargetLoweringX86Base class, and related
/// classes.
///
//===----------------------------------------------------------------------===//

// Choose one namespace, since including this file should not cause the
// templates to be instantiated.  This avoids duplicating the PoolTypeConverter
// data items, but is ugly as code common to all of x86 is including code
// specific to one of 32 or 64.
// TODO(jpp): replace this ugliness with the beauty of extern template.

#define X86NAMESPACE X8632
#include "IceTargetLoweringX86Base.h"
#undef X86NAMESPACE

namespace Ice {
namespace X86 {

const char *PoolTypeConverter<float>::TypeName = "float";
const char *PoolTypeConverter<float>::AsmTag = ".long";
const char *PoolTypeConverter<float>::PrintfString = "0x%x";

const char *PoolTypeConverter<double>::TypeName = "double";
const char *PoolTypeConverter<double>::AsmTag = ".quad";
const char *PoolTypeConverter<double>::PrintfString = "0x%llx";

const char *PoolTypeConverter<uint32_t>::TypeName = "i32";
const char *PoolTypeConverter<uint32_t>::AsmTag = ".long";
const char *PoolTypeConverter<uint32_t>::PrintfString = "0x%x";

const char *PoolTypeConverter<uint16_t>::TypeName = "i16";
const char *PoolTypeConverter<uint16_t>::AsmTag = ".short";
const char *PoolTypeConverter<uint16_t>::PrintfString = "0x%x";

const char *PoolTypeConverter<uint8_t>::TypeName = "i8";
const char *PoolTypeConverter<uint8_t>::AsmTag = ".byte";
const char *PoolTypeConverter<uint8_t>::PrintfString = "0x%x";

} // end of namespace X86
} // end of namespace Ice
