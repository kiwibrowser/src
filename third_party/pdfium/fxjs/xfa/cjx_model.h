// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_MODEL_H_
#define FXJS_XFA_CJX_MODEL_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_Node;

class CJX_Model : public CJX_Node {
 public:
  explicit CJX_Model(CXFA_Node* obj);
  ~CJX_Model() override;

  JS_METHOD(clearErrorList, CJX_Model);
  JS_METHOD(createNode, CJX_Model);
  JS_METHOD(isCompatibleNS, CJX_Model);

  JS_PROP(aliasNode);
  JS_PROP(context);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_MODEL_H_
