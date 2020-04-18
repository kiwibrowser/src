// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_DRAW_H_
#define FXJS_XFA_CJX_DRAW_H_

#include "fxjs/xfa/cjx_container.h"

class CXFA_Draw;

class CJX_Draw : public CJX_Container {
 public:
  explicit CJX_Draw(CXFA_Draw* node);
  ~CJX_Draw() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(anchorType);
  JS_PROP(colSpan);
  JS_PROP(h);
  JS_PROP(hAlign);
  JS_PROP(locale);
  JS_PROP(maxH);
  JS_PROP(maxW);
  JS_PROP(minH);
  JS_PROP(minW);
  JS_PROP(presence);
  JS_PROP(rawValue);
  JS_PROP(relevant);
  JS_PROP(rotate);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(vAlign);
  JS_PROP(w);
  JS_PROP(x);
  JS_PROP(y);
};

#endif  // FXJS_XFA_CJX_DRAW_H_
