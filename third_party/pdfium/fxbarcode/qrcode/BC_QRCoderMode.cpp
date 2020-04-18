// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2007 ZXing authors
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

#include "fxbarcode/qrcode/BC_QRCoderMode.h"

#include <utility>

#include "fxbarcode/utils.h"

CBC_QRCoderMode* CBC_QRCoderMode::sBYTE = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sNUMERIC = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sALPHANUMERIC = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sKANJI = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sECI = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sGBK = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sTERMINATOR = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sFNC1_FIRST_POSITION = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sFNC1_SECOND_POSITION = nullptr;
CBC_QRCoderMode* CBC_QRCoderMode::sSTRUCTURED_APPEND = nullptr;

CBC_QRCoderMode::CBC_QRCoderMode(std::vector<int32_t> charCountBits,
                                 int32_t bits,
                                 ByteString name)
    : m_characterCountBitsForVersions(std::move(charCountBits)),
      m_bits(bits),
      m_name(name) {}

CBC_QRCoderMode::~CBC_QRCoderMode() {}

void CBC_QRCoderMode::Initialize() {
  sBYTE = new CBC_QRCoderMode({8, 16, 16}, 0x4, "BYTE");
  sALPHANUMERIC = new CBC_QRCoderMode({9, 11, 13}, 0x2, "ALPHANUMERIC");
  sECI = new CBC_QRCoderMode(std::vector<int32_t>(), 0x7, "ECI");
  sKANJI = new CBC_QRCoderMode({8, 10, 12}, 0x8, "KANJI");
  sNUMERIC = new CBC_QRCoderMode({10, 12, 14}, 0x1, "NUMERIC");
  sGBK = new CBC_QRCoderMode({8, 10, 12}, 0x0D, "GBK");
  sTERMINATOR = new CBC_QRCoderMode(std::vector<int32_t>(), 0x00, "TERMINATOR");
  sFNC1_FIRST_POSITION =
      new CBC_QRCoderMode(std::vector<int32_t>(), 0x05, "FNC1_FIRST_POSITION");
  sFNC1_SECOND_POSITION =
      new CBC_QRCoderMode(std::vector<int32_t>(), 0x09, "FNC1_SECOND_POSITION");
  sSTRUCTURED_APPEND =
      new CBC_QRCoderMode(std::vector<int32_t>(), 0x03, "STRUCTURED_APPEND");
}

void CBC_QRCoderMode::Finalize() {
  delete sBYTE;
  delete sALPHANUMERIC;
  delete sECI;
  delete sKANJI;
  delete sNUMERIC;
  delete sGBK;
  delete sTERMINATOR;
  delete sFNC1_FIRST_POSITION;
  delete sFNC1_SECOND_POSITION;
  delete sSTRUCTURED_APPEND;
}

CBC_QRCoderMode* CBC_QRCoderMode::ForBits(int32_t bits, int32_t& e) {
  switch (bits) {
    case 0x0:
      return sTERMINATOR;
    case 0x1:
      return sNUMERIC;
    case 0x2:
      return sALPHANUMERIC;
    case 0x3:
      return sSTRUCTURED_APPEND;
    case 0x4:
      return sBYTE;
    case 0x5:
      return sFNC1_FIRST_POSITION;
    case 0x7:
      return sECI;
    case 0x8:
      return sKANJI;
    case 0x9:
      return sFNC1_SECOND_POSITION;
    case 0x0D:
      return sGBK;
    default:
      e = BCExceptionUnsupportedMode;
      return nullptr;
  }
}

int32_t CBC_QRCoderMode::GetBits() const {
  return m_bits;
}

ByteString CBC_QRCoderMode::GetName() const {
  return m_name;
}

int32_t CBC_QRCoderMode::GetCharacterCountBits(int32_t number,
                                               int32_t& e) const {
  if (m_characterCountBitsForVersions.empty()) {
    e = BCExceptionCharacterNotThisMode;
    return 0;
  }
  int32_t offset;
  if (number <= 9) {
    offset = 0;
  } else if (number <= 26) {
    offset = 1;
  } else {
    offset = 2;
  }
  return m_characterCountBitsForVersions[offset];
}

void CBC_QRCoderMode::Destroy() {
  if (sBYTE) {
    delete CBC_QRCoderMode::sBYTE;
    sBYTE = nullptr;
  }
  if (sNUMERIC) {
    delete CBC_QRCoderMode::sNUMERIC;
    sNUMERIC = nullptr;
  }
  if (sALPHANUMERIC) {
    delete CBC_QRCoderMode::sALPHANUMERIC;
    sALPHANUMERIC = nullptr;
  }
  if (sKANJI) {
    delete CBC_QRCoderMode::sKANJI;
    sKANJI = nullptr;
  }
  if (sECI) {
    delete CBC_QRCoderMode::sECI;
    sECI = nullptr;
  }
  if (sGBK) {
    delete CBC_QRCoderMode::sGBK;
    sGBK = nullptr;
  }
  if (sTERMINATOR) {
    delete CBC_QRCoderMode::sTERMINATOR;
    sTERMINATOR = nullptr;
  }
  if (sFNC1_FIRST_POSITION) {
    delete CBC_QRCoderMode::sFNC1_FIRST_POSITION;
    sFNC1_FIRST_POSITION = nullptr;
  }
  if (sFNC1_SECOND_POSITION) {
    delete CBC_QRCoderMode::sFNC1_SECOND_POSITION;
    sFNC1_SECOND_POSITION = nullptr;
  }
  if (sSTRUCTURED_APPEND) {
    delete CBC_QRCoderMode::sSTRUCTURED_APPEND;
    sSTRUCTURED_APPEND = nullptr;
  }
}
