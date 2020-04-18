//===- subzero/src/IceInstX8664.h - x86-64 machine instructions -*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief (Note: x86 instructions are templates, and they are defined in
/// src/IceInstX86Base.)
///
/// When interacting with the X8664 target (which should only happen in the
/// X8664 TargetLowering) clients have should use the Ice::X8664::Traits::Insts
/// traits, which hides all the template verboseness behind a type alias.
///
/// For example, to create an X8664 MOV Instruction, clients should do
///
/// ::Ice::X8664::Traits::Insts::Mov::create
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEINSTX8664_H
#define SUBZERO_SRC_ICEINSTX8664_H

#include "IceDefs.h"
#include "IceInst.h"
#define X86NAMESPACE X8664
#include "IceInstX86Base.h"
#undef X86NAMESPACE
#include "IceOperand.h"
#include "IceTargetLoweringX8664Traits.h"

#endif // SUBZERO_SRC_ICEINSTX8664_H
