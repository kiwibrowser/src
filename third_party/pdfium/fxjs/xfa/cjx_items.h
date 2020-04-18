// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_ITEMS_H_
#define FXJS_XFA_CJX_ITEMS_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Items;

class CJX_Items : public CJX_Node {
 public:
  explicit CJX_Items(CXFA_Items* node);
  ~CJX_Items() override;

  JS_PROP(presence);
  JS_PROP(ref);
  JS_PROP(save);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_ITEMS_H_
