// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_CONTAINER_H_
#define FXJS_XFA_CJX_CONTAINER_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_Node;

class CJX_Container : public CJX_Node {
 public:
  explicit CJX_Container(CXFA_Node* node);
  ~CJX_Container() override;

  JS_METHOD(getDelta, CJX_Container);
  JS_METHOD(getDeltas, CJX_Container);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_CONTAINER_H_
