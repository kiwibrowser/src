// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_X12ENCODER_H_
#define FXBARCODE_DATAMATRIX_BC_X12ENCODER_H_

#include "fxbarcode/datamatrix/BC_C40Encoder.h"

class CBC_X12Encoder : public CBC_C40Encoder {
 public:
  CBC_X12Encoder();
  ~CBC_X12Encoder() override;

  // CBC_C40Encoder
  int32_t getEncodingMode() override;
  void Encode(CBC_EncoderContext& context, int32_t& e) override;
  void handleEOD(CBC_EncoderContext& context,
                 WideString& buffer,
                 int32_t& e) override;
  int32_t encodeChar(wchar_t c, WideString& sb, int32_t& e) override;
};

#endif  // FXBARCODE_DATAMATRIX_BC_X12ENCODER_H_
