// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_FLOAT_H_
#define FXJS_XFA_CJX_FLOAT_H_

#include "fxjs/xfa/cjx_content.h"

class CXFA_Float;

class CJX_Float : public CJX_Content {
 public:
  explicit CJX_Float(CXFA_Float* node);
  ~CJX_Float() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(value);
};

#endif  // FXJS_XFA_CJX_FLOAT_H_
