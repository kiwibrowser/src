// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_BREAKBEFORE_H_
#define FXJS_XFA_CJX_BREAKBEFORE_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_BreakBefore;

class CJX_BreakBefore : public CJX_Node {
 public:
  explicit CJX_BreakBefore(CXFA_BreakBefore* node);
  ~CJX_BreakBefore() override;

  JS_PROP(leader);
  JS_PROP(startNew);
  JS_PROP(target);
  JS_PROP(targetType);
  JS_PROP(trailer);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_BREAKBEFORE_H_
