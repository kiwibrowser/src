// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_FORM_H_
#define FXJS_XFA_CJX_FORM_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_model.h"

class CXFA_Form;

class CJX_Form : public CJX_Model {
 public:
  explicit CJX_Form(CXFA_Form* form);
  ~CJX_Form() override;

  JS_METHOD(execCalculate, CJX_Form);
  JS_METHOD(execInitialize, CJX_Form);
  JS_METHOD(execValidate, CJX_Form);
  JS_METHOD(formNodes, CJX_Form);
  JS_METHOD(recalculate, CJX_Form);
  JS_METHOD(remerge, CJX_Form);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_FORM_H_
