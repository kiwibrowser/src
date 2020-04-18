// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_PDF417_BC_PDF417ERRORCORRECTION_H_
#define FXBARCODE_PDF417_BC_PDF417ERRORCORRECTION_H_

#include <stdint.h>

#include "core/fxcrt/fx_string.h"

class CBC_PDF417ErrorCorrection {
 public:
  CBC_PDF417ErrorCorrection();
  virtual ~CBC_PDF417ErrorCorrection();

  static int32_t getErrorCorrectionCodewordCount(int32_t errorCorrectionLevel);
  static bool generateErrorCorrection(const WideString& dataCodewords,
                                      int32_t errorCorrectionLevel,
                                      WideString* result);
};

#endif  // FXBARCODE_PDF417_BC_PDF417ERRORCORRECTION_H_
