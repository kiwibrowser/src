// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_HANDLER_H_
#define FXJS_XFA_CJX_HANDLER_H_

#include "fxjs/xfa/cjx_textnode.h"

class CXFA_Handler;

class CJX_Handler : public CJX_TextNode {
 public:
  explicit CJX_Handler(CXFA_Handler* node);
  ~CJX_Handler() override;

  JS_PROP(type);
  JS_PROP(use);
  JS_PROP(version);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_HANDLER_H_
