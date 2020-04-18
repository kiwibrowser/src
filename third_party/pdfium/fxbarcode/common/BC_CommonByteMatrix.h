// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_COMMON_BC_COMMONBYTEMATRIX_H_
#define FXBARCODE_COMMON_BC_COMMONBYTEMATRIX_H_

#include <stdint.h>

#include "core/fxcrt/fx_system.h"

class CBC_CommonByteMatrix {
 public:
  CBC_CommonByteMatrix(int32_t width, int32_t height);
  virtual ~CBC_CommonByteMatrix();

  int32_t GetHeight();
  int32_t GetWidth();
  uint8_t Get(int32_t x, int32_t y);
  uint8_t* GetArray();

  void Set(int32_t x, int32_t y, int32_t value);
  void Set(int32_t x, int32_t y, uint8_t value);
  void clear(uint8_t value);
  virtual void Init();

 private:
  uint8_t* m_bytes;
  int32_t m_width;
  int32_t m_height;
};

#endif  // FXBARCODE_COMMON_BC_COMMONBYTEMATRIX_H_
