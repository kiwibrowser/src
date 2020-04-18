// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SCRIPT_H_
#define FXJS_XFA_CJX_SCRIPT_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Script;

class CJX_Script : public CJX_Node {
 public:
  explicit CJX_Script(CXFA_Script* node);
  ~CJX_Script() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(binding);
  JS_PROP(contentType);
  JS_PROP(runAt);
  JS_PROP(stateless);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(value);
};

#endif  // FXJS_XFA_CJX_SCRIPT_H_
