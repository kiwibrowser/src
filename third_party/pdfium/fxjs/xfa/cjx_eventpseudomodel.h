// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_EVENTPSEUDOMODEL_H_
#define FXJS_XFA_CJX_EVENTPSEUDOMODEL_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_object.h"

class CFXJSE_Value;
class CScript_EventPseudoModel;

enum class XFA_Event {
  Change = 0,
  CommitKey,
  FullText,
  Keydown,
  Modifier,
  NewContentType,
  NewText,
  PreviousContentType,
  PreviousText,
  Reenter,
  SelectionEnd,
  SelectionStart,
  Shift,
  SoapFaultCode,
  SoapFaultString,
  Target,
  CancelAction
};

class CJX_EventPseudoModel : public CJX_Object {
 public:
  explicit CJX_EventPseudoModel(CScript_EventPseudoModel* model);
  ~CJX_EventPseudoModel() override;

  JS_METHOD(emit, CJX_EventPseudoModel);
  JS_METHOD(reset, CJX_EventPseudoModel);

  JS_PROP(change);
  JS_PROP(commitKey);
  JS_PROP(fullText);
  JS_PROP(keyDown);
  JS_PROP(modifier);
  JS_PROP(newContentType);
  JS_PROP(newText);
  JS_PROP(prevContentType);
  JS_PROP(prevText);
  JS_PROP(reenter);
  JS_PROP(selEnd);
  JS_PROP(selStart);
  JS_PROP(shift);
  JS_PROP(soapFaultCode);
  JS_PROP(soapFaultString);
  JS_PROP(target);

 private:
  void Property(CFXJSE_Value* pValue, XFA_Event dwFlag, bool bSetting);

  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_EVENTPSEUDOMODEL_H_
