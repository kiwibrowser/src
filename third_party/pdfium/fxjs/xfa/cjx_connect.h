// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_CONNECT_H_
#define FXJS_XFA_CJX_CONNECT_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Connect;

class CJX_Connect : public CJX_Node {
 public:
  explicit CJX_Connect(CXFA_Connect* node);
  ~CJX_Connect() override;

  JS_PROP(connection);
  JS_PROP(delayedOpen);
  JS_PROP(ref);
  JS_PROP(timeout);
  JS_PROP(usage);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_CONNECT_H_
