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

#include "fxbarcode/qrcode/BC_QRCoderErrorCorrectionLevel.h"

CBC_QRCoderErrorCorrectionLevel* CBC_QRCoderErrorCorrectionLevel::L = nullptr;
CBC_QRCoderErrorCorrectionLevel* CBC_QRCoderErrorCorrectionLevel::M = nullptr;
CBC_QRCoderErrorCorrectionLevel* CBC_QRCoderErrorCorrectionLevel::Q = nullptr;
CBC_QRCoderErrorCorrectionLevel* CBC_QRCoderErrorCorrectionLevel::H = nullptr;

CBC_QRCoderErrorCorrectionLevel::CBC_QRCoderErrorCorrectionLevel(
    int32_t ordinal,
    int32_t bits,
    const char* name)
    : m_ordinal(ordinal), m_bits(bits), m_name(name) {}

CBC_QRCoderErrorCorrectionLevel::~CBC_QRCoderErrorCorrectionLevel() {}

void CBC_QRCoderErrorCorrectionLevel::Initialize() {
  L = new CBC_QRCoderErrorCorrectionLevel(0, 0x01, "L");
  M = new CBC_QRCoderErrorCorrectionLevel(1, 0x00, "M");
  Q = new CBC_QRCoderErrorCorrectionLevel(2, 0x03, "Q");
  H = new CBC_QRCoderErrorCorrectionLevel(3, 0x02, "H");
}

void CBC_QRCoderErrorCorrectionLevel::Finalize() {
  delete L;
  delete M;
  delete Q;
  delete H;
}

CBC_QRCoderErrorCorrectionLevel* CBC_QRCoderErrorCorrectionLevel::ForBits(
    int32_t bits) {
  switch (bits) {
    case 0x00:
      return M;
    case 0x01:
      return L;
    case 0x02:
      return H;
    case 0x03:
      return Q;
    default:
      return nullptr;
  }
}
void CBC_QRCoderErrorCorrectionLevel::Destroy() {
  if (L) {
    delete CBC_QRCoderErrorCorrectionLevel::L;
    L = nullptr;
  }
  if (M) {
    delete CBC_QRCoderErrorCorrectionLevel::M;
    M = nullptr;
  }
  if (H) {
    delete CBC_QRCoderErrorCorrectionLevel::H;
    H = nullptr;
  }
  if (Q) {
    delete CBC_QRCoderErrorCorrectionLevel::Q;
    Q = nullptr;
  }
}
