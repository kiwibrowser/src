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

#include "fxbarcode/common/BC_CommonBitArray.h"

#include <utility>

#include "fxbarcode/utils.h"

CBC_CommonBitArray::CBC_CommonBitArray(CBC_CommonBitArray* array) {
  m_size = array->GetSize();
  m_bits = array->GetBits();
}

CBC_CommonBitArray::CBC_CommonBitArray() {
  m_bits.resize(1);
  m_size = 0;
}

CBC_CommonBitArray::CBC_CommonBitArray(int32_t size) {
  m_bits.resize((size + 31) >> 5);
  m_size = size;
}

CBC_CommonBitArray::~CBC_CommonBitArray() {}

size_t CBC_CommonBitArray::GetSize() {
  return m_size;
}

std::vector<int32_t>& CBC_CommonBitArray::GetBits() {
  return m_bits;
}

size_t CBC_CommonBitArray::GetSizeInBytes() {
  return (m_size + 7) >> 3;
}

bool CBC_CommonBitArray::Get(size_t i) {
  return (m_bits[i >> 5] & (1 << (i & 0x1f))) != 0;
}

void CBC_CommonBitArray::Set(size_t i) {
  m_bits[i >> 5] |= 1 << (i & 0x1F);
}

void CBC_CommonBitArray::Flip(size_t i) {
  m_bits[i >> 5] ^= 1 << (i & 0x1F);
}

void CBC_CommonBitArray::SetBulk(size_t i, int32_t newBits) {
  m_bits[i >> 5] = newBits;
}

void CBC_CommonBitArray::Clear() {
  for (auto& value : m_bits)
    value = 0;
}

int32_t* CBC_CommonBitArray::GetBitArray() {
  return m_bits.data();
}

void CBC_CommonBitArray::Reverse() {
  std::vector<int32_t> newBits(m_bits.size());
  for (size_t i = 0; i < m_size; i++) {
    if (Get(m_size - i - 1))
      newBits[i >> 5] |= 1 << (i & 0x1F);
  }
  m_bits = std::move(newBits);
}
