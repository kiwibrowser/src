//===- subzero/src/IceRegistersX8664.h - Register information ---*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the registers and their encodings for x86-64.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEREGISTERSX8664_H
#define SUBZERO_SRC_ICEREGISTERSX8664_H

#include "IceDefs.h"
#include "IceInstX8664.def"
#include "IceTypes.h"

namespace Ice {

class RegX8664 {
public:
  /// An enum of every register. The enum value may not match the encoding used
  /// to binary encode register operands in instructions.
  enum AllRegisters {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  val,
    REGX8664_TABLE
#undef X
        Reg_NUM
  };

  /// An enum of GPR Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum GPRRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  Encoded_##val = encode,
    REGX8664_GPR_TABLE
#undef X
        Encoded_Not_GPR = -1
  };

  /// An enum of XMM Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum XmmRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  Encoded_##val = encode,
    REGX8664_XMM_TABLE
#undef X
        Encoded_Not_Xmm = -1
  };

  /// An enum of Byte Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum ByteRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  Encoded_8_##val = encode,
    REGX8664_BYTEREG_TABLE
#undef X
        Encoded_Not_ByteReg = -1
  };
};

} // end of namespace Ice

#endif // SUBZERO_SRC_ICEREGISTERSX8664_H
