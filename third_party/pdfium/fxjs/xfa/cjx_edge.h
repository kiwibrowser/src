// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_EDGE_H_
#define FXJS_XFA_CJX_EDGE_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Edge;

class CJX_Edge : public CJX_Node {
 public:
  explicit CJX_Edge(CXFA_Edge* node);
  ~CJX_Edge() override;

  JS_PROP(cap);
  JS_PROP(presence);
  JS_PROP(stroke);
  JS_PROP(thickness);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_EDGE_H_
