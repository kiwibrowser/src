//===- subzero/src/IceRegistersX8632.h - Register information ---*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the registers and their encodings for x86-32.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEREGISTERSX8632_H
#define SUBZERO_SRC_ICEREGISTERSX8632_H

#include "IceDefs.h"
#include "IceInstX8632.def"
#include "IceTypes.h"

namespace Ice {

class RegX8632 {
public:
  /// An enum of every register. The enum value may not match the encoding used
  /// to binary encode register operands in instructions.
  enum AllRegisters {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8, is16To8,      \
          isTrunc8Rcvr, isAhRcvr, aliases)                                     \
  val,
    REGX8632_TABLE
#undef X
        Reg_NUM
  };

  /// An enum of GPR Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum GPRRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8, is16To8,      \
          isTrunc8Rcvr, isAhRcvr, aliases)                                     \
  Encoded_##val = encode,
    REGX8632_GPR_TABLE
#undef X
        Encoded_Not_GPR = -1
  };

  /// An enum of XMM Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum XmmRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8, is16To8,      \
          isTrunc8Rcvr, isAhRcvr, aliases)                                     \
  Encoded_##val = encode,
    REGX8632_XMM_TABLE
#undef X
        Encoded_Not_Xmm = -1
  };

  /// An enum of Byte Registers. The enum value does match the encoding used to
  /// binary encode register operands in instructions.
  enum ByteRegister {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8, is16To8,      \
          isTrunc8Rcvr, isAhRcvr, aliases)                                     \
  Encoded_8_##val = encode,
    REGX8632_BYTEREG_TABLE
#undef X
        Encoded_Not_ByteReg = -1
  };

  /// An enum of X87 Stack Registers. The enum value does match the encoding
  /// used to binary encode register operands in instructions.
  enum X87STRegister {
#define X(val, encode, name) Encoded_##val = encode,
    X87ST_REGX8632_TABLE
#undef X
        Encoded_Not_X87STReg = -1
  };

  static inline X87STRegister getEncodedSTReg(uint32_t X87RegNum) {
    assert(int(Encoded_X87ST_First) <= int(X87RegNum));
    assert(X87RegNum <= Encoded_X87ST_Last);
    return X87STRegister(X87RegNum);
  }
};

} // end of namespace Ice

#endif // SUBZERO_SRC_ICEREGISTERSX8632_H
