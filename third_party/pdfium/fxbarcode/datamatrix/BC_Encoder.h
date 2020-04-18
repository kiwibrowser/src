// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_ENCODER_H_
#define FXBARCODE_DATAMATRIX_BC_ENCODER_H_

#include <stdint.h>

class CBC_EncoderContext;

class CBC_Encoder {
 public:
  CBC_Encoder();
  virtual ~CBC_Encoder();

  virtual int32_t getEncodingMode() = 0;
  virtual void Encode(CBC_EncoderContext& context, int32_t& e) = 0;
};

#endif  // FXBARCODE_DATAMATRIX_BC_ENCODER_H_
