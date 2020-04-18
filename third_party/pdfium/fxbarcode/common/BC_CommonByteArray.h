// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_COMMON_BC_COMMONBYTEARRAY_H_
#define FXBARCODE_COMMON_BC_COMMONBYTEARRAY_H_

#include <stdint.h>

#include <vector>

// TODO(weili): The usage of this class should be replaced by
// std::vector<uint8_t>.
class CBC_CommonByteArray {
 public:
  CBC_CommonByteArray();
  explicit CBC_CommonByteArray(int32_t size);
  CBC_CommonByteArray(uint8_t* byteArray, int32_t size);
  virtual ~CBC_CommonByteArray();

  int32_t At(int32_t index) const;
  int32_t Size() const;
  bool IsEmpty() const;
  void Set(int32_t index, int32_t value);
  void AppendByte(int32_t value);
  void Reserve(int32_t capacity);
  void Set(const uint8_t* source, int32_t offset, int32_t count);
  void Set(std::vector<uint8_t>* source, int32_t offset, int32_t count);

 private:
  int32_t m_size;
  int32_t m_index;
  uint8_t* m_bytes;
};

#endif  // FXBARCODE_COMMON_BC_COMMONBYTEARRAY_H_
