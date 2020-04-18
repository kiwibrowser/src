// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_CAPTION_H_
#define FXJS_XFA_CJX_CAPTION_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Caption;

class CJX_Caption : public CJX_Node {
 public:
  explicit CJX_Caption(CXFA_Caption* node);
  ~CJX_Caption() override;

  JS_PROP(placement);
  JS_PROP(presence);
  JS_PROP(reserve);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_CAPTION_H_
