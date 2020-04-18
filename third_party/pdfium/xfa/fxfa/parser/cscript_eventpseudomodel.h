// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CSCRIPT_EVENTPSEUDOMODEL_H_
#define XFA_FXFA_PARSER_CSCRIPT_EVENTPSEUDOMODEL_H_

#include "fxjs/xfa/cjx_eventpseudomodel.h"
#include "xfa/fxfa/parser/cxfa_object.h"

class CScript_EventPseudoModel : public CXFA_Object {
 public:
  explicit CScript_EventPseudoModel(CXFA_Document* pDocument);
  ~CScript_EventPseudoModel() override;

  CJX_EventPseudoModel* JSEventPseudoModel() {
    return static_cast<CJX_EventPseudoModel*>(JSObject());
  }
};

#endif  // XFA_FXFA_PARSER_CSCRIPT_EVENTPSEUDOMODEL_H_
