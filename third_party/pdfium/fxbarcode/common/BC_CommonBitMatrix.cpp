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

#include "fxbarcode/common/BC_CommonBitMatrix.h"

#include "fxbarcode/common/BC_CommonBitArray.h"
#include "fxbarcode/utils.h"
#include "third_party/base/ptr_util.h"

CBC_CommonBitMatrix::CBC_CommonBitMatrix() {}

void CBC_CommonBitMatrix::Init(int32_t dimension) {
  m_width = dimension;
  m_height = dimension;
  int32_t rowSize = (m_height + 31) >> 5;
  m_rowSize = rowSize;
  m_bits = FX_Alloc2D(int32_t, m_rowSize, m_height);
  memset(m_bits, 0, m_rowSize * m_height * sizeof(int32_t));
}

void CBC_CommonBitMatrix::Init(int32_t width, int32_t height) {
  m_width = width;
  m_height = height;
  int32_t rowSize = (width + 31) >> 5;
  m_rowSize = rowSize;
  m_bits = FX_Alloc2D(int32_t, m_rowSize, m_height);
  memset(m_bits, 0, m_rowSize * m_height * sizeof(int32_t));
}

CBC_CommonBitMatrix::~CBC_CommonBitMatrix() {
  FX_Free(m_bits);
}

bool CBC_CommonBitMatrix::Get(int32_t x, int32_t y) const {
  int32_t offset = y * m_rowSize + (x >> 5);
  if (offset >= m_rowSize * m_height || offset < 0)
    return false;
  return ((((uint32_t)m_bits[offset]) >> (x & 0x1f)) & 1) != 0;
}

void CBC_CommonBitMatrix::Set(int32_t x, int32_t y) {
  int32_t offset = y * m_rowSize + (x >> 5);
  if (offset >= m_rowSize * m_height || offset < 0)
    return;
  m_bits[offset] |= 1 << (x & 0x1f);
}

void CBC_CommonBitMatrix::Flip(int32_t x, int32_t y) {
  int32_t offset = y * m_rowSize + (x >> 5);
  m_bits[offset] ^= 1 << (x & 0x1f);
}

void CBC_CommonBitMatrix::Clear() {
  memset(m_bits, 0, m_rowSize * m_height * sizeof(int32_t));
}

bool CBC_CommonBitMatrix::SetRegion(int32_t left,
                                    int32_t top,
                                    int32_t width,
                                    int32_t height) {
  if (top < 0 || left < 0 || height < 1 || width < 1)
    return false;

  int32_t right = left + width;
  int32_t bottom = top + height;
  if (m_height < bottom || m_width < right)
    return false;

  for (int32_t y = top; y < bottom; y++) {
    int32_t offset = y * m_rowSize;
    for (int32_t x = left; x < right; x++)
      m_bits[offset + (x >> 5)] |= 1 << (x & 0x1f);
  }
  return true;
}

void CBC_CommonBitMatrix::SetRow(int32_t y, CBC_CommonBitArray* row) {
  int32_t l = y * m_rowSize;
  for (int32_t i = 0; i < m_rowSize; i++) {
    m_bits[l] = row->GetBitArray()[i];
    l++;
  }
}

void CBC_CommonBitMatrix::SetCol(int32_t y, CBC_CommonBitArray* col) {
  for (size_t i = 0; i < col->GetBits().size(); ++i)
    m_bits[i * m_rowSize + y] = col->GetBitArray()[i];
}
