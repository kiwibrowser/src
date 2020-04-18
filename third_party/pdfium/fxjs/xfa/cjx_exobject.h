// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_EXOBJECT_H_
#define FXJS_XFA_CJX_EXOBJECT_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_ExObject;

class CJX_ExObject : public CJX_Node {
 public:
  explicit CJX_ExObject(CXFA_ExObject* node);
  ~CJX_ExObject() override;

  JS_PROP(archive);
  JS_PROP(classId);
  JS_PROP(codeBase);
  JS_PROP(codeType);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_EXOBJECT_H_
