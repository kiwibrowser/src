// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_LOGPSEUDOMODEL_H_
#define FXJS_XFA_CJX_LOGPSEUDOMODEL_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_object.h"

class CScript_LogPseudoModel;

// TODO(dsinclair): This doesn't exist in the spec. Remove after
// xfa_basic_data_element_script is removed.
class CJX_LogPseudoModel : public CJX_Object {
 public:
  explicit CJX_LogPseudoModel(CScript_LogPseudoModel* model);
  ~CJX_LogPseudoModel() override;

  JS_METHOD(message, CJX_LogPseudoModel);
  JS_METHOD(traceEnabled, CJX_LogPseudoModel);
  JS_METHOD(traceActivate, CJX_LogPseudoModel);
  JS_METHOD(traceDeactivate, CJX_LogPseudoModel);
  JS_METHOD(trace, CJX_LogPseudoModel);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_LOGPSEUDOMODEL_H_
