// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SUBFORM_H_
#define FXJS_XFA_CJX_SUBFORM_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_container.h"

class CXFA_Delta;

class CJX_Subform : public CJX_Container {
 public:
  explicit CJX_Subform(CXFA_Node* container);
  ~CJX_Subform() override;

  JS_METHOD(execCalculate, CJX_Subform);
  JS_METHOD(execEvent, CJX_Subform);
  JS_METHOD(execInitialize, CJX_Subform);
  JS_METHOD(execValidate, CJX_Subform);

  JS_PROP(allowMacro);
  JS_PROP(anchorType);
  JS_PROP(colSpan);
  JS_PROP(columnWidths);
  JS_PROP(h);
  JS_PROP(hAlign);
  JS_PROP(instanceIndex);
  JS_PROP(layout);
  JS_PROP(locale);
  JS_PROP(maxH);
  JS_PROP(maxW);
  JS_PROP(minH);
  JS_PROP(minW);
  JS_PROP(presence);
  JS_PROP(relevant);
  JS_PROP(restoreState);
  JS_PROP(scope);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(validationMessage);
  JS_PROP(vAlign);
  JS_PROP(w);
  JS_PROP(x);
  JS_PROP(y);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_SUBFORM_H_
