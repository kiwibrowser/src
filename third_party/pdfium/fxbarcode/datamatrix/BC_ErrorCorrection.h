// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_ERRORCORRECTION_H_
#define FXBARCODE_DATAMATRIX_BC_ERRORCORRECTION_H_

#include "core/fxcrt/widestring.h"

class CBC_SymbolInfo;

class CBC_ErrorCorrection {
 public:
  CBC_ErrorCorrection();
  virtual ~CBC_ErrorCorrection();

  static void Initialize();
  static void Finalize();
  static WideString encodeECC200(WideString codewords,
                                 CBC_SymbolInfo* symbolInfo,
                                 int32_t& e);

 private:
  static const int32_t MODULO_VALUE = 0x12D;

  static int32_t LOG[256];
  static int32_t ALOG[256];

  static WideString createECCBlock(WideString codewords,
                                   int32_t numECWords,
                                   int32_t& e);
  static WideString createECCBlock(WideString codewords,
                                   int32_t start,
                                   int32_t len,
                                   int32_t numECWords,
                                   int32_t& e);
};

#endif  // FXBARCODE_DATAMATRIX_BC_ERRORCORRECTION_H_
