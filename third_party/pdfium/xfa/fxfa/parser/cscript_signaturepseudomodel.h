// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CSCRIPT_SIGNATUREPSEUDOMODEL_H_
#define XFA_FXFA_PARSER_CSCRIPT_SIGNATUREPSEUDOMODEL_H_

#include "fxjs/xfa/cjx_signaturepseudomodel.h"
#include "xfa/fxfa/parser/cxfa_object.h"

class CScript_SignaturePseudoModel : public CXFA_Object {
 public:
  explicit CScript_SignaturePseudoModel(CXFA_Document* pDocument);
  ~CScript_SignaturePseudoModel() override;

  CJX_SignaturePseudoModel* JSSignaturePseudoModel() {
    return static_cast<CJX_SignaturePseudoModel*>(JSObject());
  }
};

#endif  // XFA_FXFA_PARSER_CSCRIPT_SIGNATUREPSEUDOMODEL_H_
