// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_EXDATA_H_
#define FXJS_XFA_CJX_EXDATA_H_

#include "fxjs/xfa/cjx_content.h"

class CXFA_ExData;

class CJX_ExData : public CJX_Content {
 public:
  explicit CJX_ExData(CXFA_ExData* node);
  ~CJX_ExData() override;

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(contentType);
  JS_PROP(href);
  JS_PROP(maxLength);
  JS_PROP(transferEncoding);
  JS_PROP(use);
  JS_PROP(usehref);
};

#endif  // FXJS_XFA_CJX_EXDATA_H_
