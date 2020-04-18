// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_BARCODE_H_
#define FXJS_XFA_CJX_BARCODE_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Barcode;

class CJX_Barcode : public CJX_Node {
 public:
  explicit CJX_Barcode(CXFA_Barcode* arc);
  ~CJX_Barcode() override;

  JS_PROP(charEncoding);
  JS_PROP(checksum);
  JS_PROP(dataColumnCount);
  JS_PROP(dataLength);
  JS_PROP(dataPrep);
  JS_PROP(dataRowCount);
  JS_PROP(endChar);
  JS_PROP(errorCorrectionLevel);
  JS_PROP(moduleHeight);
  JS_PROP(moduleWidth);
  JS_PROP(printCheckDigit);
  JS_PROP(rowColumnRatio);
  JS_PROP(startChar);
  JS_PROP(textLocation);
  JS_PROP(truncate);
  JS_PROP(type);
  JS_PROP(upsMode);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(wideNarrowRatio);
};

#endif  // FXJS_XFA_CJX_BARCODE_H_
