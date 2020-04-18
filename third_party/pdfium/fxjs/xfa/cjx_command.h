// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_COMMAND_H_
#define FXJS_XFA_CJX_COMMAND_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Command;

class CJX_Command : public CJX_Node {
 public:
  explicit CJX_Command(CXFA_Command* node);
  ~CJX_Command() override;

  JS_PROP(timeout);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_COMMAND_H_
