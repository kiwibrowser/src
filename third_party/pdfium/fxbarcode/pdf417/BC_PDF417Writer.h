// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_PDF417_BC_PDF417WRITER_H_
#define FXBARCODE_PDF417_BC_PDF417WRITER_H_

#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "fxbarcode/BC_TwoDimWriter.h"

class CBC_PDF417Writer : public CBC_TwoDimWriter {
 public:
  CBC_PDF417Writer();
  ~CBC_PDF417Writer() override;

  uint8_t* Encode(const WideString& contents,
                  int32_t& outWidth,
                  int32_t& outHeight);

  // CBC_TwoDimWriter
  bool SetErrorCorrectionLevel(int32_t level) override;

  void SetTruncated(bool truncated);

 private:
  void rotateArray(std::vector<uint8_t>& bitarray,
                   int32_t width,
                   int32_t height);
  bool m_bTruncated;
};

#endif  // FXBARCODE_PDF417_BC_PDF417WRITER_H_
