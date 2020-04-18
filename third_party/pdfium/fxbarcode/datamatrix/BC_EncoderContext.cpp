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

#include "fxbarcode/datamatrix/BC_EncoderContext.h"

#include "fxbarcode/BC_UtilCodingConvert.h"
#include "fxbarcode/common/BC_CommonBitMatrix.h"
#include "fxbarcode/datamatrix/BC_Encoder.h"
#include "fxbarcode/datamatrix/BC_SymbolInfo.h"
#include "fxbarcode/utils.h"

CBC_EncoderContext::CBC_EncoderContext(const WideString& msg,
                                       const WideString& ecLevel,
                                       int32_t& e) {
  ByteString dststr;
  CBC_UtilCodingConvert::UnicodeToUTF8(msg, dststr);
  size_t c = dststr.GetLength();
  WideString sb;
  sb.Reserve(c);
  for (size_t i = 0; i < c; i++) {
    wchar_t ch = static_cast<wchar_t>(dststr[i] & 0xff);
    if (ch == '?' && dststr[i] != '?') {
      e = BCExceptionCharactersOutsideISO88591Encoding;
    }
    sb += ch;
  }
  m_msg = sb;
  m_allowRectangular = true;
  m_newEncoding = -1;
  m_pos = 0;
  m_symbolInfo = nullptr;
  m_skipAtEnd = 0;
}

CBC_EncoderContext::~CBC_EncoderContext() {}

void CBC_EncoderContext::setAllowRectangular(bool allow) {
  m_allowRectangular = allow;
}

void CBC_EncoderContext::setSkipAtEnd(int32_t count) {
  m_skipAtEnd = count;
}
wchar_t CBC_EncoderContext::getCurrentChar() {
  return m_msg[m_pos];
}
wchar_t CBC_EncoderContext::getCurrent() {
  return m_msg[m_pos];
}

void CBC_EncoderContext::writeCodewords(const WideString& codewords) {
  m_codewords += codewords;
}

void CBC_EncoderContext::writeCodeword(wchar_t codeword) {
  m_codewords += codeword;
}
size_t CBC_EncoderContext::getCodewordCount() {
  return m_codewords.GetLength();
}
void CBC_EncoderContext::signalEncoderChange(int32_t encoding) {
  m_newEncoding = encoding;
}
void CBC_EncoderContext::resetEncoderSignal() {
  m_newEncoding = -1;
}
bool CBC_EncoderContext::hasMoreCharacters() {
  return m_pos < getTotalMessageCharCount();
}
size_t CBC_EncoderContext::getRemainingCharacters() {
  return getTotalMessageCharCount() - m_pos;
}
void CBC_EncoderContext::updateSymbolInfo(int32_t& e) {
  updateSymbolInfo(getCodewordCount(), e);
}
void CBC_EncoderContext::updateSymbolInfo(int32_t len, int32_t& e) {
  if (!m_symbolInfo || len > m_symbolInfo->dataCapacity()) {
    m_symbolInfo = CBC_SymbolInfo::lookup(len, m_allowRectangular, e);
    if (e != BCExceptionNO)
      return;
  }
}

void CBC_EncoderContext::resetSymbolInfo() {
  m_allowRectangular = true;
}

size_t CBC_EncoderContext::getTotalMessageCharCount() {
  return m_msg.GetLength() - m_skipAtEnd;
}
