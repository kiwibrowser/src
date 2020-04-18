// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_HIGHLEVELENCODER_H_
#define FXBARCODE_DATAMATRIX_BC_HIGHLEVELENCODER_H_

#include <vector>

#include "core/fxcrt/widestring.h"

#define ASCII_ENCODATION 0
#define C40_ENCODATION 1
#define TEXT_ENCODATION 2
#define X12_ENCODATION 3
#define EDIFACT_ENCODATION 4
#define BASE256_ENCODATION 5

class CBC_HighLevelEncoder {
 public:
  CBC_HighLevelEncoder();
  ~CBC_HighLevelEncoder();

  std::vector<uint8_t>& getBytesForMessage(WideString msg);

  static WideString encodeHighLevel(WideString msg,
                                    WideString ecLevel,
                                    bool allowRectangular,
                                    int32_t& e);
  static int32_t lookAheadTest(WideString msg,
                               int32_t startpos,
                               int32_t currentMode);
  static bool isDigit(wchar_t ch);
  static bool isExtendedASCII(wchar_t ch);
  static int32_t determineConsecutiveDigitCount(WideString msg,
                                                int32_t startpos);

  static const wchar_t LATCH_TO_C40;
  static const wchar_t LATCH_TO_BASE256;
  static const wchar_t UPPER_SHIFT;
  static const wchar_t LATCH_TO_ANSIX12;
  static const wchar_t LATCH_TO_TEXT;
  static const wchar_t LATCH_TO_EDIFACT;
  static const wchar_t C40_UNLATCH;
  static const wchar_t X12_UNLATCH;

 private:
  static wchar_t randomize253State(wchar_t ch, int32_t codewordPosition);
  static int32_t findMinimums(std::vector<float>& charCounts,
                              std::vector<int32_t>& intCharCounts,
                              int32_t min,
                              std::vector<uint8_t>& mins);
  static int32_t getMinimumCount(std::vector<uint8_t>& mins);
  static bool isNativeC40(wchar_t ch);
  static bool isNativeText(wchar_t ch);
  static bool isNativeX12(wchar_t ch);
  static bool isX12TermSep(wchar_t ch);
  static bool isNativeEDIFACT(wchar_t ch);

  static const wchar_t PAD;
  static const wchar_t MACRO_05;
  static const wchar_t MACRO_06;
  static const wchar_t MACRO_05_HEADER[];
  static const wchar_t MACRO_06_HEADER[];
  static const wchar_t MACRO_TRAILER;

  std::vector<uint8_t> m_bytearray;
};

#endif  // FXBARCODE_DATAMATRIX_BC_HIGHLEVELENCODER_H_
