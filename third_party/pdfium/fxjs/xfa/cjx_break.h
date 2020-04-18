// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_BREAK_H_
#define FXJS_XFA_CJX_BREAK_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Break;

class CJX_Break : public CJX_Node {
 public:
  explicit CJX_Break(CXFA_Break* node);
  ~CJX_Break() override;

  JS_PROP(after);
  JS_PROP(afterTarget);
  JS_PROP(before);
  JS_PROP(beforeTarget);
  JS_PROP(bookendLeader);
  JS_PROP(bookendTrailer);
  JS_PROP(overflowLeader);
  JS_PROP(overflowTarget);
  JS_PROP(overflowTrailer);
  JS_PROP(startNew);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_BREAK_H_
