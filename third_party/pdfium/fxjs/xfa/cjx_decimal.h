// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_DECIMAL_H_
#define FXJS_XFA_CJX_DECIMAL_H_

#include "fxjs/xfa/cjx_content.h"

class CXFA_Decimal;

class CJX_Decimal : public CJX_Content {
 public:
  explicit CJX_Decimal(CXFA_Decimal* node);
  ~CJX_Decimal() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(fracDigits);
  JS_PROP(leadDigits);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(value);
};

#endif  // FXJS_XFA_CJX_DECIMAL_H_
