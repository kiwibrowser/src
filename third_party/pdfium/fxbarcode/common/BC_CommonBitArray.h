// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_COMMON_BC_COMMONBITARRAY_H_
#define FXBARCODE_COMMON_BC_COMMONBITARRAY_H_

#include <stdint.h>

#include <vector>

class CBC_CommonBitArray {
 public:
  explicit CBC_CommonBitArray(CBC_CommonBitArray* array);
  explicit CBC_CommonBitArray(int32_t size);
  CBC_CommonBitArray();
  virtual ~CBC_CommonBitArray();

  size_t GetSize();
  size_t GetSizeInBytes();
  std::vector<int32_t>& GetBits();
  int32_t* GetBitArray();
  bool Get(size_t i);
  void Set(size_t i);
  void Flip(size_t i);
  void SetBulk(size_t i, int32_t newBits);
  void Reverse();
  void Clear();

 private:
  size_t m_size;
  std::vector<int32_t> m_bits;
};

#endif  // FXBARCODE_COMMON_BC_COMMONBITARRAY_H_
