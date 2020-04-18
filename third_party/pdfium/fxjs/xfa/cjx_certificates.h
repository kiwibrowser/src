// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_CERTIFICATES_H_
#define FXJS_XFA_CJX_CERTIFICATES_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Certificates;

class CJX_Certificates : public CJX_Node {
 public:
  explicit CJX_Certificates(CXFA_Certificates* node);
  ~CJX_Certificates() override;

  JS_PROP(credentialServerPolicy);
  JS_PROP(url);
  JS_PROP(urlPolicy);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_CERTIFICATES_H_
