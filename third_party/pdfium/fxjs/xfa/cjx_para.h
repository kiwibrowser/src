// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PARA_H_
#define FXJS_XFA_CJX_PARA_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Para;

class CJX_Para : public CJX_Node {
 public:
  explicit CJX_Para(CXFA_Para* node);
  ~CJX_Para() override;

  JS_PROP(hAlign);
  JS_PROP(lineHeight);
  JS_PROP(marginLeft);
  JS_PROP(marginRight);
  JS_PROP(preserve);
  JS_PROP(radixOffset);
  JS_PROP(spaceAbove);
  JS_PROP(spaceBelow);
  JS_PROP(tabDefault);
  JS_PROP(tabStops);
  JS_PROP(textIndent);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(vAlign);
};

#endif  // FXJS_XFA_CJX_PARA_H_
