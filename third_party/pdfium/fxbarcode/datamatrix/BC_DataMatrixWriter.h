// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_DATAMATRIXWRITER_H_
#define FXBARCODE_DATAMATRIX_BC_DATAMATRIXWRITER_H_

#include "fxbarcode/BC_TwoDimWriter.h"

class CBC_CommonByteMatrix;
class CBC_DefaultPlacement;
class CBC_SymbolInfo;

class CBC_DataMatrixWriter : public CBC_TwoDimWriter {
 public:
  CBC_DataMatrixWriter();
  ~CBC_DataMatrixWriter() override;

  uint8_t* Encode(const WideString& contents,
                  int32_t& outWidth,
                  int32_t& outHeight);

  // CBC_TwoDimWriter
  bool SetErrorCorrectionLevel(int32_t level) override;

 private:
  int32_t m_iCorrectLevel;
};

#endif  // FXBARCODE_DATAMATRIX_BC_DATAMATRIXWRITER_H_
