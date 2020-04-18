// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_KEYUSAGE_H_
#define FXJS_XFA_CJX_KEYUSAGE_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_KeyUsage;

class CJX_KeyUsage : public CJX_Node {
 public:
  explicit CJX_KeyUsage(CXFA_KeyUsage* node);
  ~CJX_KeyUsage() override;

  JS_PROP(crlSign);
  JS_PROP(dataEncipherment);
  JS_PROP(decipherOnly);
  JS_PROP(digitalSignature);
  JS_PROP(encipherOnly);
  JS_PROP(keyAgreement);
  JS_PROP(keyCertSign);
  JS_PROP(keyEncipherment);
  JS_PROP(nonRepudiation);
  JS_PROP(type);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_KEYUSAGE_H_
