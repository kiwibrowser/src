// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_BORDER_H_
#define FXJS_XFA_CJX_BORDER_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Border;

class CJX_Border : public CJX_Node {
 public:
  explicit CJX_Border(CXFA_Border* node);
  ~CJX_Border() override;

  JS_PROP(breakValue); /* break */
  JS_PROP(hand);
  JS_PROP(presence);
  JS_PROP(relevant);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_BORDER_H_
