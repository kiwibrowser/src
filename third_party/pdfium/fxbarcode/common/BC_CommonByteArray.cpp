// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2008 ZXing authors
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
#include "fxbarcode/common/BC_CommonByteArray.h"

#include <algorithm>

#include "core/fxcrt/fx_memory.h"

CBC_CommonByteArray::CBC_CommonByteArray() {
  m_bytes = nullptr;
  m_size = 0;
  m_index = 0;
}
CBC_CommonByteArray::CBC_CommonByteArray(int32_t size) {
  m_size = size;
  m_bytes = FX_Alloc(uint8_t, size);
  memset(m_bytes, 0, size);
  m_index = 0;
}
CBC_CommonByteArray::CBC_CommonByteArray(uint8_t* byteArray, int32_t size) {
  m_size = size;
  m_bytes = FX_Alloc(uint8_t, size);
  memcpy(m_bytes, byteArray, size);
  m_index = size;
}
CBC_CommonByteArray::~CBC_CommonByteArray() {
  FX_Free(m_bytes);
}
int32_t CBC_CommonByteArray::At(int32_t index) const {
  return m_bytes[index] & 0xff;
}
void CBC_CommonByteArray::Set(int32_t index, int32_t value) {
  m_bytes[index] = (uint8_t)value;
}
int32_t CBC_CommonByteArray::Size() const {
  return m_size;
}
bool CBC_CommonByteArray::IsEmpty() const {
  return m_size == 0;
}
void CBC_CommonByteArray::AppendByte(int32_t value) {
  if (m_size == 0 || m_index >= m_size) {
    int32_t newSize = std::max(32, m_size << 1);
    Reserve(newSize);
  }
  m_bytes[m_index] = (uint8_t)value;
  m_index++;
}
void CBC_CommonByteArray::Reserve(int32_t capacity) {
  if (!m_bytes || m_size < capacity) {
    uint8_t* newArray = FX_Alloc(uint8_t, capacity);
    if (m_bytes) {
      memcpy(newArray, m_bytes, m_size);
      memset(newArray + m_size, 0, capacity - m_size);
    } else {
      memset(newArray, 0, capacity);
    }
    FX_Free(m_bytes);
    m_bytes = newArray;
    m_size = capacity;
  }
}
void CBC_CommonByteArray::Set(const uint8_t* source,
                              int32_t offset,
                              int32_t count) {
  FX_Free(m_bytes);
  m_bytes = FX_Alloc(uint8_t, count);
  m_size = count;
  memcpy(m_bytes, source + offset, count);
  m_index = count;
}
void CBC_CommonByteArray::Set(std::vector<uint8_t>* source,
                              int32_t offset,
                              int32_t count) {
  FX_Free(m_bytes);
  m_bytes = FX_Alloc(uint8_t, count);
  m_size = count;
  int32_t i;
  for (i = 0; i < count; i++) {
    m_bytes[i] = source->operator[](i + offset);
  }
  m_index = m_size;
}
