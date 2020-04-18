// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PICTURE_H_
#define FXJS_XFA_CJX_PICTURE_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Picture;

class CJX_Picture : public CJX_Node {
 public:
  explicit CJX_Picture(CXFA_Picture* node);
  ~CJX_Picture() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(value);
};

#endif  // FXJS_XFA_CJX_PICTURE_H_
