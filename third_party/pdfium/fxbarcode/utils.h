// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_UTILS_H_
#define FXBARCODE_UTILS_H_

#include <ctype.h>

#include <vector>

#include "core/fxcrt/fx_string.h"

bool BC_FX_ByteString_Replace(ByteString& dst,
                              uint32_t first,
                              uint32_t last,
                              int32_t count,
                              char c);
void BC_FX_ByteString_Append(ByteString& dst, int32_t count, char c);
void BC_FX_ByteString_Append(ByteString& dst, const std::vector<uint8_t>& ba);

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
#include <limits>
#elif _FX_OS_ == _FX_OS_MACOSX_ || _FX_OS_ == _FX_OS_LINUX_
#include <limits.h>
#endif

enum BCFORMAT {
  BCFORMAT_UNSPECIFY = -1,
  BCFORMAT_CODABAR,
  BCFORMAT_CODE_39,
  BCFORMAT_CODE_128,
  BCFORMAT_CODE_128B,
  BCFORMAT_CODE_128C,
  BCFORMAT_EAN_8,
  BCFORMAT_UPC_A,
  BCFORMAT_EAN_13,
  BCFORMAT_PDF_417,
  BCFORMAT_DATAMATRIX,
  BCFORMAT_QR_CODE
};

#define BCExceptionNO 0
#define BCExceptionIllegalArgument 16
#define BCExceptionDegreeIsNegative 31
#define BCExceptionAIsZero 37
#define BCExceptionValueMustBeEither0or1 50
#define BCExceptionBadIndexException 52
#define BCExceptionNoSuchVersion 58
#define BCExceptionUnsupportedMode 64
#define BCExceptionInvalidateCharacter 65
#define BCExceptionInvalidateMaskPattern 68
#define BCExceptionNullPointer 69
#define BCExceptionBadMask 70
#define BCExceptionInvalidateImageData 73
#define BCExceptionHeight_8BeZero 74
#define BCExceptionCharacterNotThisMode 75
#define BCExceptionInvalidateData 77
#define BCExceptionCharactersOutsideISO88591Encoding 87
#define BCExceptionIllegalDataCodewords 88
#define BCExceptionIllegalStateUnexpectedCase 90
#define BCExceptionIllegalStateMessageLengthInvalid 92
#define BCExceptionNonEncodableCharacterDetected 96
#define BCExceptionGeneric 107

#endif  // FXBARCODE_UTILS_H_
