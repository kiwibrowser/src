// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_FONT_H_
#define FXJS_XFA_CJX_FONT_H_

#include "fxjs/xfa/cjx_node.h"

class CXFA_Font;

class CJX_Font : public CJX_Node {
 public:
  explicit CJX_Font(CXFA_Font* node);
  ~CJX_Font() override;

  JS_PROP(baselineShift);
  JS_PROP(fontHorizontalScale);
  JS_PROP(fontVerticalScale);
  JS_PROP(kerningMode);
  JS_PROP(letterSpacing);
  JS_PROP(lineThrough);
  JS_PROP(lineThroughPeriod);
  JS_PROP(overline);
  JS_PROP(overlinePeriod);
  JS_PROP(posture);
  JS_PROP(size);
  JS_PROP(typeface);
  JS_PROP(underline);
  JS_PROP(underlinePeriod);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(weight);
};

#endif  // FXJS_XFA_CJX_FONT_H_
