// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_NODE_H_
#define FXJS_XFA_CJX_NODE_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_object.h"
#include "fxjs/xfa/cjx_tree.h"
#include "xfa/fxfa/fxfa_basic.h"

class CXFA_Node;

class CJX_Node : public CJX_Tree {
 public:
  explicit CJX_Node(CXFA_Node* node);
  ~CJX_Node() override;

  JS_METHOD(applyXSL, CJX_Node);
  JS_METHOD(assignNode, CJX_Node);
  JS_METHOD(clone, CJX_Node);
  JS_METHOD(getAttribute, CJX_Node);
  JS_METHOD(getElement, CJX_Node);
  JS_METHOD(isPropertySpecified, CJX_Node);
  JS_METHOD(loadXML, CJX_Node);
  JS_METHOD(saveFilteredXML, CJX_Node);
  JS_METHOD(saveXML, CJX_Node);
  JS_METHOD(setAttribute, CJX_Node);
  JS_METHOD(setElement, CJX_Node);

  JS_PROP(id);
  JS_PROP(isContainer);
  JS_PROP(isNull);
  JS_PROP(model);
  JS_PROP(ns);
  JS_PROP(oneOfChild);

  CXFA_Node* GetXFANode();
  const CXFA_Node* GetXFANode() const;

 protected:
  int32_t execSingleEventByName(const WideStringView& wsEventName,
                                XFA_Element eType);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_NODE_H_
