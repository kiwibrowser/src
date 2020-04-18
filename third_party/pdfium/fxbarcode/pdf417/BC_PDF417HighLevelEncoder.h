// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_PDF417_BC_PDF417HIGHLEVELENCODER_H_
#define FXBARCODE_PDF417_BC_PDF417HIGHLEVELENCODER_H_

#include <vector>

#include "core/fxcrt/fx_string.h"
#include "fxbarcode/pdf417/BC_PDF417Compaction.h"

class CBC_PDF417HighLevelEncoder {
 public:
  static WideString encodeHighLevel(WideString msg,
                                    Compaction compaction,
                                    int32_t& e);
  static void Inverse();
  static void Initialize();
  static void Finalize();

 private:
  static const int32_t TEXT_COMPACTION;
  static const int32_t BYTE_COMPACTION;
  static const int32_t NUMERIC_COMPACTION;
  static const int32_t SUBMODE_PUNCTUATION;
  static const int32_t LATCH_TO_TEXT;
  static const int32_t LATCH_TO_BYTE_PADDED;
  static const int32_t LATCH_TO_NUMERIC;
  static const int32_t SHIFT_TO_BYTE;
  static const int32_t LATCH_TO_BYTE;
  static const uint8_t TEXT_MIXED_RAW[];
  static const uint8_t TEXT_PUNCTUATION_RAW[];

  static int32_t MIXED[128];
  static int32_t PUNCTUATION[128];

  static int32_t encodeText(WideString msg,
                            size_t startpos,
                            size_t count,
                            WideString& sb,
                            int32_t initialSubmode);
  static void encodeBinary(std::vector<uint8_t>* bytes,
                           size_t startpos,
                           size_t count,
                           int32_t startmode,
                           WideString& sb);
  static void encodeNumeric(WideString msg,
                            size_t startpos,
                            size_t count,
                            WideString& sb);
  static bool isDigit(wchar_t ch);
  static bool isAlphaUpper(wchar_t ch);
  static bool isAlphaLower(wchar_t ch);
  static bool isMixed(wchar_t ch);
  static bool isPunctuation(wchar_t ch);
  static bool isText(wchar_t ch);
  static size_t determineConsecutiveDigitCount(WideString msg, size_t startpos);
  static size_t determineConsecutiveTextCount(WideString msg, size_t startpos);
  static Optional<size_t> determineConsecutiveBinaryCount(
      WideString msg,
      std::vector<uint8_t>* bytes,
      size_t startpos);

  friend class PDF417HighLevelEncoder_EncodeNumeric_Test;
  friend class PDF417HighLevelEncoder_EncodeBinary_Test;
  friend class PDF417HighLevelEncoder_EncodeText_Test;
  friend class PDF417HighLevelEncoder_ConsecutiveDigitCount_Test;
  friend class PDF417HighLevelEncoder_ConsecutiveTextCount_Test;
  friend class PDF417HighLevelEncoder_ConsecutiveBinaryCount_Test;
};

#endif  // FXBARCODE_PDF417_BC_PDF417HIGHLEVELENCODER_H_
