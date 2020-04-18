// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SPEAK_H_
#define FXJS_XFA_CJX_SPEAK_H_

#include "fxjs/xfa/cjx_textnode.h"

class CXFA_Speak;

class CJX_Speak : public CJX_TextNode {
 public:
  explicit CJX_Speak(CXFA_Speak* node);
  ~CJX_Speak() override;

  JS_PROP(disable);
  JS_PROP(priority);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_SPEAK_H_
