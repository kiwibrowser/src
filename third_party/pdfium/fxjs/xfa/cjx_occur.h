// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_OCCUR_H_
#define FXJS_XFA_CJX_OCCUR_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Occur;

class CJX_Occur : public CJX_Node {
 public:
  explicit CJX_Occur(CXFA_Occur* node);
  ~CJX_Occur() override;

  JS_PROP(initial);
  JS_PROP(max);
  JS_PROP(min);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_OCCUR_H_
