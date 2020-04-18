//===- subzero/src/IceAssemblerX8664.h - Assembler for x86-64 ---*- C++ -*-===//
//
// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// Modified by the Subzero authors.
//
//===----------------------------------------------------------------------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Instantiates the Assembler for X86-64.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEASSEMBLERX8664_H
#define SUBZERO_SRC_ICEASSEMBLERX8664_H

#define X86NAMESPACE X8664
#include "IceAssemblerX86Base.h"
#undef X86NAMESPACE
#include "IceTargetLoweringX8664Traits.h"

namespace Ice {
namespace X8664 {

using AssemblerX8664 = AssemblerX86Base<X8664::Traits>;
using Label = AssemblerX8664::Label;
using Immediate = AssemblerX8664::Immediate;

} // end of namespace X8664
} // end of namespace Ice

#endif // SUBZERO_SRC_ICEASSEMBLERX8664_H
