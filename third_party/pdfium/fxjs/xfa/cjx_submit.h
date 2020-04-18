// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SUBMIT_H_
#define FXJS_XFA_CJX_SUBMIT_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Submit;

class CJX_Submit : public CJX_Node {
 public:
  explicit CJX_Submit(CXFA_Submit* node);
  ~CJX_Submit() override;

  JS_PROP(embedPDF);
  JS_PROP(format);
  JS_PROP(target);
  JS_PROP(textEncoding);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(xdpContent);
};

#endif  // FXJS_XFA_CJX_SUBMIT_H_
