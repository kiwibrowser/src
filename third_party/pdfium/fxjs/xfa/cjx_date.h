// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_DATE_H_
#define FXJS_XFA_CJX_DATE_H_

#include "fxjs/xfa/cjx_content.h"

class CXFA_Date;

class CJX_Date : public CJX_Content {
 public:
  explicit CJX_Date(CXFA_Date* node);
  ~CJX_Date() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(value);
};

#endif  // FXJS_XFA_CJX_DATE_H_
