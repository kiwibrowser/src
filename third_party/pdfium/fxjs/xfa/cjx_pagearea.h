// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PAGEAREA_H_
#define FXJS_XFA_CJX_PAGEAREA_H_

#include "fxjs/xfa/cjx_container.h"

class CXFA_PageArea;

class CJX_PageArea : public CJX_Container {
 public:
  explicit CJX_PageArea(CXFA_PageArea* node);
  ~CJX_PageArea() override;

  JS_PROP(blankOrNotBlank);
  JS_PROP(initialNumber);
  JS_PROP(numbered);
  JS_PROP(oddOrEven);
  JS_PROP(pagePosition);
  JS_PROP(relevant);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_PAGEAREA_H_
