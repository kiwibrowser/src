// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_MANIFEST_H_
#define FXJS_XFA_CJX_MANIFEST_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_Manifest;

class CJX_Manifest : public CJX_Node {
 public:
  explicit CJX_Manifest(CXFA_Manifest* manifest);
  ~CJX_Manifest() override;

  JS_METHOD(evaluate, CJX_Manifest);

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(action);
  JS_PROP(use);
  JS_PROP(usehref);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_MANIFEST_H_
