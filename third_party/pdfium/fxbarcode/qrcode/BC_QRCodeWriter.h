// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_QRCODE_BC_QRCODEWRITER_H_
#define FXBARCODE_QRCODE_BC_QRCODEWRITER_H_

#include "fxbarcode/BC_TwoDimWriter.h"

class CBC_TwoDimWriter;
class CBC_QRCodeWriter : public CBC_TwoDimWriter {
 public:
  CBC_QRCodeWriter();
  ~CBC_QRCodeWriter() override;

  static void ReleaseAll();

  uint8_t* Encode(const WideString& contents,
                  int32_t ecLevel,
                  int32_t& outWidth,
                  int32_t& outHeight);

  // CBC_TwoDimWriter
  bool SetErrorCorrectionLevel(int32_t level) override;
};

#endif  // FXBARCODE_QRCODE_BC_QRCODEWRITER_H_
