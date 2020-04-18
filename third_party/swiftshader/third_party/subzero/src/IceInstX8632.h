//===- subzero/src/IceInstX8632.h - x86-32 machine instructions -*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Used to house all the X8632 instructions.
///
/// Subzero has been modified to use templates for X86 instructions, so all
/// those definitions are are in IceInstX86Base.h
///
/// When interacting with the X8632 target (which should only happen in the
/// X8632 TargetLowering) clients have should use the Ice::X8632::Traits::Insts
/// traits, which hides all the template verboseness behind a type alias.
///
/// For example, to create an X8632 MOV Instruction, clients should do
///
/// ::Ice::X8632::Traits::Insts::Mov::create
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEINSTX8632_H
#define SUBZERO_SRC_ICEINSTX8632_H

#include "IceDefs.h"
#include "IceInst.h"
#define X86NAMESPACE X8632
#include "IceInstX86Base.h"
#undef X86NAMESPACE
#include "IceOperand.h"
#include "IceTargetLoweringX8632Traits.h"

#endif // SUBZERO_SRC_ICEINSTX8632_H
