// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_TREE_H_
#define FXJS_XFA_CJX_TREE_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_object.h"

class CXFA_Object;
class CXFA_Node;

class CJX_Tree : public CJX_Object {
 public:
  explicit CJX_Tree(CXFA_Object* obj);
  ~CJX_Tree() override;

  JS_METHOD(resolveNode, CJX_Tree);
  JS_METHOD(resolveNodes, CJX_Tree);

  JS_PROP(all);
  JS_PROP(classAll);
  JS_PROP(classIndex);
  JS_PROP(index);
  JS_PROP(name);
  JS_PROP(nodes);
  JS_PROP(parent);
  JS_PROP(somExpression);

 private:
  void ResolveNodeList(CFXJSE_Value* pValue,
                       WideString wsExpression,
                       uint32_t dwFlag,
                       CXFA_Node* refNode);

  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_TREE_H_
