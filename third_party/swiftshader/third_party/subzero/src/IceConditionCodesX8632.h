//===- subzero/src/IceConditionCodesX8632.h - Condition Codes ---*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the condition codes for x86-32.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICECONDITIONCODESX8632_H
#define SUBZERO_SRC_ICECONDITIONCODESX8632_H

#include "IceDefs.h"
#include "IceInstX8632.def"

namespace Ice {

class CondX86 {
  CondX86() = delete;
  CondX86(const CondX86 &) = delete;
  CondX86 &operator=(const CondX86 &) = delete;

public:
  /// An enum of condition codes used for branches and cmov. The enum value
  /// should match the value used to encode operands in binary instructions.
  enum BrCond {
#define X(val, encode, opp, dump, emit) val = encode,
    ICEINSTX8632BR_TABLE
#undef X
        Br_None
  };

  /// An enum of condition codes relevant to the CMPPS instruction. The enum
  /// value should match the value used to encode operands in binary
  /// instructions.
  enum CmppsCond {
#define X(val, emit) val,
    ICEINSTX8632CMPPS_TABLE
#undef X
        Cmpps_Invalid
  };
};

} // end of namespace Ice

#endif // SUBZERO_SRC_ICECONDITIONCODESX8632_H
