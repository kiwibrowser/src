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

#include "fxbarcode/datamatrix/BC_EdifactEncoder.h"

#include <algorithm>

#include "fxbarcode/common/BC_CommonBitMatrix.h"
#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_EncoderContext.h"
#include "fxbarcode/datamatrix/BC_HighLevelEncoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"

namespace {

WideString EncodeToEdifactCodewords(const WideString& sb, int32_t startPos) {
  int32_t len = sb.GetLength() - startPos;
  if (len == 0)
    return WideString();

  wchar_t c1 = sb[startPos];
  wchar_t c2 = len >= 2 ? sb[startPos + 1] : 0;
  wchar_t c3 = len >= 3 ? sb[startPos + 2] : 0;
  wchar_t c4 = len >= 4 ? sb[startPos + 3] : 0;
  int32_t v = (c1 << 18) + (c2 << 12) + (c3 << 6) + c4;
  constexpr int32_t kBuflen = 3;
  wchar_t cw[kBuflen];
  cw[0] = static_cast<wchar_t>((v >> 16) & 255);
  cw[1] = static_cast<wchar_t>((v >> 8) & 255);
  cw[2] = static_cast<wchar_t>(v & 255);
  return WideString(cw, std::min(len, kBuflen));
}

bool HandleEOD(CBC_EncoderContext* context, const WideString& buffer) {
  int32_t count = buffer.GetLength();
  if (count == 0)
    return true;
  if (count > 4)
    return false;

  if (count == 1) {
    int32_t e = BCExceptionNO;
    context->updateSymbolInfo(e);
    if (e != BCExceptionNO)
      return false;

    int32_t available =
        context->m_symbolInfo->dataCapacity() - context->getCodewordCount();
    int32_t remaining = context->getRemainingCharacters();
    if (remaining == 0 && available <= 2)
      return true;
  }

  int32_t restChars = count - 1;
  WideString encoded = EncodeToEdifactCodewords(buffer, 0);
  if (encoded.IsEmpty())
    return false;

  bool endOfSymbolReached = !context->hasMoreCharacters();
  bool restInAscii = endOfSymbolReached && restChars <= 2;
  if (restChars <= 2) {
    int32_t e = BCExceptionNO;
    context->updateSymbolInfo(context->getCodewordCount() + restChars, e);
    if (e != BCExceptionNO)
      return false;

    int32_t available =
        context->m_symbolInfo->dataCapacity() - context->getCodewordCount();
    if (available >= 3) {
      restInAscii = false;
      context->updateSymbolInfo(
          context->getCodewordCount() + encoded.GetLength(), e);
      if (e != BCExceptionNO)
        return false;
    }
  }

  if (restInAscii) {
    context->resetSymbolInfo();
    context->m_pos -= restChars;
  } else {
    context->writeCodewords(encoded);
  }
  context->signalEncoderChange(ASCII_ENCODATION);
  return true;
}

void encodeChar(wchar_t c, WideString* sb, int32_t& e) {
  if (c >= ' ' && c <= '?') {
    *sb += c;
  } else if (c >= '@' && c <= '^') {
    *sb += (wchar_t)(c - 64);
  } else {
    e = BCExceptionIllegalArgument;
  }
}

}  // namespace

CBC_EdifactEncoder::CBC_EdifactEncoder() {}

CBC_EdifactEncoder::~CBC_EdifactEncoder() {}

int32_t CBC_EdifactEncoder::getEncodingMode() {
  return EDIFACT_ENCODATION;
}

void CBC_EdifactEncoder::Encode(CBC_EncoderContext& context, int32_t& e) {
  WideString buffer;
  while (context.hasMoreCharacters()) {
    wchar_t c = context.getCurrentChar();
    encodeChar(c, &buffer, e);
    if (e != BCExceptionNO) {
      return;
    }
    context.m_pos++;
    int32_t count = buffer.GetLength();
    if (count >= 4) {
      WideString encoded = EncodeToEdifactCodewords(buffer, 0);
      if (encoded.IsEmpty()) {
        e = BCExceptionGeneric;
        return;
      }
      context.writeCodewords(encoded);
      buffer.Delete(0, 4);
      int32_t newMode = CBC_HighLevelEncoder::lookAheadTest(
          context.m_msg, context.m_pos, getEncodingMode());
      if (newMode != getEncodingMode()) {
        context.signalEncoderChange(ASCII_ENCODATION);
        break;
      }
    }
  }
  buffer += static_cast<wchar_t>(31);
  if (!HandleEOD(&context, buffer))
    e = BCExceptionGeneric;
}
