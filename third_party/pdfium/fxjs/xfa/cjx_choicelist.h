// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_CHOICELIST_H_
#define FXJS_XFA_CJX_CHOICELIST_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_ChoiceList;

class CJX_ChoiceList : public CJX_Node {
 public:
  explicit CJX_ChoiceList(CXFA_ChoiceList* node);
  ~CJX_ChoiceList() override;

  JS_PROP(commitOn);
  JS_PROP(open);
  JS_PROP(textEntry);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_CHOICELIST_H_
