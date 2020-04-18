// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SUBFORMSET_H_
#define FXJS_XFA_CJX_SUBFORMSET_H_

#include "fxjs/xfa/cjx_container.h"

class CXFA_SubformSet;

class CJX_SubformSet : public CJX_Container {
 public:
  explicit CJX_SubformSet(CXFA_SubformSet* node);
  ~CJX_SubformSet() override;

  JS_PROP(instanceIndex);
  JS_PROP(relation);
  JS_PROP(relevant);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_SUBFORMSET_H_
