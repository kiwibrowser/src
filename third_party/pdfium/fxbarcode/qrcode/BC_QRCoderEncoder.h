// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_QRCODE_BC_QRCODERENCODER_H_
#define FXBARCODE_QRCODE_BC_QRCODERENCODER_H_

#include <utility>
#include <vector>

#include "core/fxcrt/fx_string.h"

class CBC_QRCoder;
class CBC_QRCoderErrorCorrectionLevel;

class CBC_QRCoderEncoder {
 public:
  CBC_QRCoderEncoder();
  ~CBC_QRCoderEncoder();

  static bool Encode(const WideString& content,
                     const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                     CBC_QRCoder* qrCode);
};

#endif  // FXBARCODE_QRCODE_BC_QRCODERENCODER_H_
