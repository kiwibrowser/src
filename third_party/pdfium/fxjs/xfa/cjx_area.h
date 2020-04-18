// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_AREA_H_
#define FXJS_XFA_CJX_AREA_H_

#include "fxjs/xfa/cjx_container.h"

class CXFA_Area;

class CJX_Area : public CJX_Container {
 public:
  explicit CJX_Area(CXFA_Area* node);
  ~CJX_Area() override;

  JS_PROP(colSpan);
  JS_PROP(relevant);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(x);
  JS_PROP(y);
};

#endif  // FXJS_XFA_CJX_AREA_H_
