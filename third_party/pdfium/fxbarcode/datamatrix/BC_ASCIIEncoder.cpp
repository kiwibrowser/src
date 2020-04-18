// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2006-2007 Jeremias Maerki.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fxbarcode/datamatrix/BC_ASCIIEncoder.h"

#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_EncoderContext.h"
#include "fxbarcode/datamatrix/BC_HighLevelEncoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"
#include "third_party/base/optional.h"

namespace {

Optional<wchar_t> EncodeASCIIDigits(wchar_t digit1, wchar_t digit2) {
  if (!CBC_HighLevelEncoder::isDigit(digit1) ||
      !CBC_HighLevelEncoder::isDigit(digit2)) {
    // This could potentially return 0 as a sentinel value. Then this function
    // can just return wchar_t instead of Optional<wchar_t>.
    return {};
  }
  return static_cast<wchar_t>((digit1 - 48) * 10 + (digit2 - 48) + 130);
}

}  // namespace

CBC_ASCIIEncoder::CBC_ASCIIEncoder() {}

CBC_ASCIIEncoder::~CBC_ASCIIEncoder() {}

int32_t CBC_ASCIIEncoder::getEncodingMode() {
  return ASCII_ENCODATION;
}

void CBC_ASCIIEncoder::Encode(CBC_EncoderContext& context, int32_t& e) {
  int32_t n = CBC_HighLevelEncoder::determineConsecutiveDigitCount(
      context.m_msg, context.m_pos);
  if (n >= 2) {
    Optional<wchar_t> code = EncodeASCIIDigits(
        context.m_msg[context.m_pos], context.m_msg[context.m_pos + 1]);
    if (!code) {
      e = BCExceptionGeneric;
      return;
    }
    context.writeCodeword(*code);
    context.m_pos += 2;
    return;
  }

  wchar_t c = context.getCurrentChar();
  int32_t newMode = CBC_HighLevelEncoder::lookAheadTest(
      context.m_msg, context.m_pos, getEncodingMode());
  if (newMode != getEncodingMode()) {
    switch (newMode) {
      case BASE256_ENCODATION:
        context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_BASE256);
        context.signalEncoderChange(BASE256_ENCODATION);
        return;
      case C40_ENCODATION:
        context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_C40);
        context.signalEncoderChange(C40_ENCODATION);
        return;
      case X12_ENCODATION:
        context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_ANSIX12);
        context.signalEncoderChange(X12_ENCODATION);
        return;
      case TEXT_ENCODATION:
        context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_TEXT);
        context.signalEncoderChange(TEXT_ENCODATION);
        return;
      case EDIFACT_ENCODATION:
        context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_EDIFACT);
        context.signalEncoderChange(EDIFACT_ENCODATION);
        return;
      default:
        e = BCExceptionGeneric;
        return;
    }
  }

  if (CBC_HighLevelEncoder::isExtendedASCII(c)) {
    context.writeCodeword(CBC_HighLevelEncoder::UPPER_SHIFT);
    context.writeCodeword(static_cast<wchar_t>(c - 128 + 1));
  } else {
    context.writeCodeword(static_cast<wchar_t>(c + 1));
  }
  context.m_pos++;
}
