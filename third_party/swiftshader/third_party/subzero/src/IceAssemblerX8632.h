//===- subzero/src/IceAssemblerX8632.h - Assembler for x86-32 ---*- C++ -*-===//
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
/// \brief Instantiates the Assembler for X86-32.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEASSEMBLERX8632_H
#define SUBZERO_SRC_ICEASSEMBLERX8632_H

#define X86NAMESPACE X8632
#include "IceAssemblerX86Base.h"
#undef X86NAMESPACE
#include "IceTargetLoweringX8632Traits.h"

namespace Ice {
namespace X8632 {

using AssemblerX8632 = AssemblerX86Base<X8632::Traits>;
using Label = AssemblerX8632::Label;
using Immediate = AssemblerX8632::Immediate;

} // end of namespace X8632
} // end of namespace Ice

#endif // SUBZERO_SRC_ICEASSEMBLERX8632_H
