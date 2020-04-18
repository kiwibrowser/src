// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_TEMPLATE_H_
#define FXJS_XFA_CJX_TEMPLATE_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_model.h"

class CXFA_Template;

class CJX_Template : public CJX_Model {
 public:
  explicit CJX_Template(CXFA_Template* tmpl);
  ~CJX_Template() override;

  /* The docs list a |createNode| method on Template but that method already
   * exists on Model, also the |createNode| docs say it exists on Model not
   * on Template so I'm leaving |createNode| out of the template methods. */
  JS_METHOD(execCalculate, CJX_Template);
  JS_METHOD(execInitialize, CJX_Template);
  JS_METHOD(execValidate, CJX_Template);
  JS_METHOD(formNodes, CJX_Template);
  JS_METHOD(recalculate, CJX_Template);
  JS_METHOD(remerge, CJX_Template);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_TEMPLATE_H_
