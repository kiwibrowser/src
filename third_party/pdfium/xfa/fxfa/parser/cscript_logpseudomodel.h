// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CSCRIPT_LOGPSEUDOMODEL_H_
#define XFA_FXFA_PARSER_CSCRIPT_LOGPSEUDOMODEL_H_

#include "fxjs/xfa/cjx_logpseudomodel.h"
#include "xfa/fxfa/parser/cxfa_object.h"

class CScript_LogPseudoModel : public CXFA_Object {
 public:
  explicit CScript_LogPseudoModel(CXFA_Document* pDocument);
  ~CScript_LogPseudoModel() override;

  CJX_LogPseudoModel* JSLogPseudoModel() {
    return static_cast<CJX_LogPseudoModel*>(JSObject());
  }
};

#endif  // XFA_FXFA_PARSER_CSCRIPT_LOGPSEUDOMODEL_H_
