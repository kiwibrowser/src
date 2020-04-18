// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_MARGIN_H_
#define FXJS_XFA_CJX_MARGIN_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Margin;

class CJX_Margin : public CJX_Node {
 public:
  explicit CJX_Margin(CXFA_Margin* node);
  ~CJX_Margin() override;

  JS_PROP(bottomInset);
  JS_PROP(leftInset);
  JS_PROP(rightInset);
  JS_PROP(topInset);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_MARGIN_H_
