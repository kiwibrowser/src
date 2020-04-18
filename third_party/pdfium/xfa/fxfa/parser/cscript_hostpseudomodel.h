// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CSCRIPT_HOSTPSEUDOMODEL_H_
#define XFA_FXFA_PARSER_CSCRIPT_HOSTPSEUDOMODEL_H_

#include "fxjs/xfa/cjx_hostpseudomodel.h"
#include "xfa/fxfa/parser/cxfa_object.h"

class CXFA_Document;

class CScript_HostPseudoModel : public CXFA_Object {
 public:
  explicit CScript_HostPseudoModel(CXFA_Document* pDocument);
  ~CScript_HostPseudoModel() override;

  CJX_HostPseudoModel* JSHostPseudoModel() {
    return static_cast<CJX_HostPseudoModel*>(JSObject());
  }
};

#endif  // XFA_FXFA_PARSER_CSCRIPT_HOSTPSEUDOMODEL_H_
