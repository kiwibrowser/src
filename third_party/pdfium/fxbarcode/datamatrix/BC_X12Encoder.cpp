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

#include "fxbarcode/datamatrix/BC_X12Encoder.h"

#include "fxbarcode/common/BC_CommonBitMatrix.h"
#include "fxbarcode/datamatrix/BC_C40Encoder.h"
#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_EncoderContext.h"
#include "fxbarcode/datamatrix/BC_HighLevelEncoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"

CBC_X12Encoder::CBC_X12Encoder() {}
CBC_X12Encoder::~CBC_X12Encoder() {}
int32_t CBC_X12Encoder::getEncodingMode() {
  return X12_ENCODATION;
}
void CBC_X12Encoder::Encode(CBC_EncoderContext& context, int32_t& e) {
  WideString buffer;
  while (context.hasMoreCharacters()) {
    wchar_t c = context.getCurrentChar();
    context.m_pos++;
    encodeChar(c, buffer, e);
    if (e != BCExceptionNO) {
      return;
    }
    int32_t count = buffer.GetLength();
    if ((count % 3) == 0) {
      writeNextTriplet(context, buffer);
      int32_t newMode = CBC_HighLevelEncoder::lookAheadTest(
          context.m_msg, context.m_pos, getEncodingMode());
      if (newMode != getEncodingMode()) {
        context.signalEncoderChange(newMode);
        break;
      }
    }
  }
  handleEOD(context, buffer, e);
}
void CBC_X12Encoder::handleEOD(CBC_EncoderContext& context,
                               WideString& buffer,
                               int32_t& e) {
  context.updateSymbolInfo(e);
  if (e != BCExceptionNO) {
    return;
  }
  int32_t available =
      context.m_symbolInfo->dataCapacity() - context.getCodewordCount();
  int32_t count = buffer.GetLength();
  if (count == 2) {
    context.writeCodeword(CBC_HighLevelEncoder::X12_UNLATCH);
    context.m_pos -= 2;
    context.signalEncoderChange(ASCII_ENCODATION);
  } else if (count == 1) {
    context.m_pos--;
    if (available > 1) {
      context.writeCodeword(CBC_HighLevelEncoder::X12_UNLATCH);
    }
    context.signalEncoderChange(ASCII_ENCODATION);
  }
}
int32_t CBC_X12Encoder::encodeChar(wchar_t c, WideString& sb, int32_t& e) {
  if (c == '\r') {
    sb += (wchar_t)'\0';
  } else if (c == '*') {
    sb += (wchar_t)'\1';
  } else if (c == '>') {
    sb += (wchar_t)'\2';
  } else if (c == ' ') {
    sb += (wchar_t)'\3';
  } else if (c >= '0' && c <= '9') {
    sb += (wchar_t)(c - 48 + 4);
  } else if (c >= 'A' && c <= 'Z') {
    sb += (wchar_t)(c - 65 + 14);
  } else {
    e = BCExceptionIllegalArgument;
    return -1;
  }
  return 1;
}
