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

#include "fxbarcode/datamatrix/BC_C40Encoder.h"

#include "fxbarcode/common/BC_CommonBitMatrix.h"
#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_EncoderContext.h"
#include "fxbarcode/datamatrix/BC_HighLevelEncoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"

namespace {

WideString EncodeToC40Codewords(const WideString& sb, int32_t startPos) {
  wchar_t c1 = sb[startPos];
  wchar_t c2 = sb[startPos + 1];
  wchar_t c3 = sb[startPos + 2];
  int32_t v = (1600 * c1) + (40 * c2) + c3 + 1;
  wchar_t cw[2];
  cw[0] = static_cast<wchar_t>(v / 256);
  cw[1] = static_cast<wchar_t>(v % 256);
  return WideString(cw, FX_ArraySize(cw));
}

}  // namespace

CBC_C40Encoder::CBC_C40Encoder() {}
CBC_C40Encoder::~CBC_C40Encoder() {}
int32_t CBC_C40Encoder::getEncodingMode() {
  return C40_ENCODATION;
}
void CBC_C40Encoder::Encode(CBC_EncoderContext& context, int32_t& e) {
  WideString buffer;
  while (context.hasMoreCharacters()) {
    wchar_t c = context.getCurrentChar();
    context.m_pos++;
    int32_t lastCharSize = encodeChar(c, buffer, e);
    if (e != BCExceptionNO) {
      return;
    }
    int32_t unwritten = (buffer.GetLength() / 3) * 2;
    int32_t curCodewordCount = context.getCodewordCount() + unwritten;
    context.updateSymbolInfo(curCodewordCount, e);
    if (e != BCExceptionNO) {
      return;
    }
    int32_t available = context.m_symbolInfo->dataCapacity() - curCodewordCount;
    if (!context.hasMoreCharacters()) {
      if ((buffer.GetLength() % 3) == 2) {
        if (available < 2 || available > 2) {
          lastCharSize = BacktrackOneCharacter(&context, &buffer, lastCharSize);
          if (lastCharSize < 0) {
            e = BCExceptionGeneric;
            return;
          }
        }
      }
      while ((buffer.GetLength() % 3) == 1 &&
             ((lastCharSize <= 3 && available != 1) || lastCharSize > 3)) {
        lastCharSize = BacktrackOneCharacter(&context, &buffer, lastCharSize);
        if (lastCharSize < 0) {
          e = BCExceptionGeneric;
          return;
        }
      }
      break;
    }
    int32_t count = buffer.GetLength();
    if ((count % 3) == 0) {
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
void CBC_C40Encoder::writeNextTriplet(CBC_EncoderContext& context,
                                      WideString& buffer) {
  context.writeCodewords(EncodeToC40Codewords(buffer, 0));
  buffer.Delete(0, 3);
}
void CBC_C40Encoder::handleEOD(CBC_EncoderContext& context,
                               WideString& buffer,
                               int32_t& e) {
  int32_t unwritten = (buffer.GetLength() / 3) * 2;
  int32_t rest = buffer.GetLength() % 3;
  int32_t curCodewordCount = context.getCodewordCount() + unwritten;
  context.updateSymbolInfo(curCodewordCount, e);
  if (e != BCExceptionNO) {
    return;
  }
  int32_t available = context.m_symbolInfo->dataCapacity() - curCodewordCount;
  if (rest == 2) {
    buffer += (wchar_t)'\0';
    while (buffer.GetLength() >= 3) {
      writeNextTriplet(context, buffer);
    }
    if (context.hasMoreCharacters()) {
      context.writeCodeword(CBC_HighLevelEncoder::C40_UNLATCH);
    }
  } else if (available == 1 && rest == 1) {
    while (buffer.GetLength() >= 3) {
      writeNextTriplet(context, buffer);
    }
    if (context.hasMoreCharacters()) {
      context.writeCodeword(CBC_HighLevelEncoder::C40_UNLATCH);
    }
    context.m_pos--;
  } else if (rest == 0) {
    while (buffer.GetLength() >= 3) {
      writeNextTriplet(context, buffer);
    }
    if (available > 0 || context.hasMoreCharacters()) {
      context.writeCodeword(CBC_HighLevelEncoder::C40_UNLATCH);
    }
  } else {
    e = BCExceptionIllegalStateUnexpectedCase;
    return;
  }
  context.signalEncoderChange(ASCII_ENCODATION);
}
int32_t CBC_C40Encoder::encodeChar(wchar_t c, WideString& sb, int32_t& e) {
  if (c == ' ') {
    sb += (wchar_t)'\3';
    return 1;
  } else if ((c >= '0') && (c <= '9')) {
    sb += (wchar_t)(c - 48 + 4);
    return 1;
  } else if ((c >= 'A') && (c <= 'Z')) {
    sb += (wchar_t)(c - 65 + 14);
    return 1;
  } else if (c <= 0x1f) {
    sb += (wchar_t)'\0';
    sb += c;
    return 2;
  } else if ((c >= '!') && (c <= '/')) {
    sb += (wchar_t)'\1';
    sb += (wchar_t)(c - 33);
    return 2;
  } else if ((c >= ':') && (c <= '@')) {
    sb += (wchar_t)'\1';
    sb += (wchar_t)(c - 58 + 15);
    return 2;
  } else if ((c >= '[') && (c <= '_')) {
    sb += (wchar_t)'\1';
    sb += (wchar_t)(c - 91 + 22);
    return 2;
  } else if ((c >= 60) && (c <= 0x7f)) {
    sb += (wchar_t)'\2';
    sb += (wchar_t)(c - 96);
    return 2;
  } else if (c >= 80) {
    sb += (wchar_t)'\1';
    sb += (wchar_t)0x001e;
    int32_t len = 2;
    len += encodeChar((c - 128), sb, e);
    if (e != BCExceptionNO)
      return 0;
    return len;
  } else {
    e = BCExceptionIllegalArgument;
    return 0;
  }
}

int32_t CBC_C40Encoder::BacktrackOneCharacter(CBC_EncoderContext* context,
                                              WideString* buffer,
                                              int32_t lastCharSize) {
  if (context->m_pos < 1)
    return -1;

  int32_t count = buffer->GetLength();
  if (count < lastCharSize)
    return -1;

  buffer->Delete(count - lastCharSize, lastCharSize);
  context->m_pos--;
  wchar_t c = context->getCurrentChar();
  int32_t e = BCExceptionNO;
  WideString removed;
  int32_t len = encodeChar(c, removed, e);
  if (e != BCExceptionNO)
    return -1;

  ASSERT(len > 0);
  context->resetSymbolInfo();
  return len;
}
