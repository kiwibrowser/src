// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_KEEP_H_
#define FXJS_XFA_CJX_KEEP_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Keep;

class CJX_Keep : public CJX_Node {
 public:
  explicit CJX_Keep(CXFA_Keep* node);
  ~CJX_Keep() override;

  JS_PROP(intact);
  JS_PROP(next);
  JS_PROP(previous);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_KEEP_H_
