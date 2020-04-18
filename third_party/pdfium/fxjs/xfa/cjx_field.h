// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_FIELD_H_
#define FXJS_XFA_CJX_FIELD_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_container.h"

class CXFA_Field;

class CJX_Field : public CJX_Container {
 public:
  explicit CJX_Field(CXFA_Field* field);
  ~CJX_Field() override;

  JS_METHOD(addItem, CJX_Field);
  JS_METHOD(boundItem, CJX_Field);
  JS_METHOD(clearItems, CJX_Field);
  JS_METHOD(deleteItem, CJX_Field);
  JS_METHOD(execCalculate, CJX_Field);
  JS_METHOD(execEvent, CJX_Field);
  JS_METHOD(execInitialize, CJX_Field);
  JS_METHOD(execValidate, CJX_Field);
  JS_METHOD(getDisplayItem, CJX_Field);
  JS_METHOD(getItemState, CJX_Field);
  JS_METHOD(getSaveItem, CJX_Field);
  JS_METHOD(setItemState, CJX_Field);

  JS_PROP(defaultValue); /* {default} */
  JS_PROP(access);
  JS_PROP(accessKey);
  JS_PROP(anchorType);
  JS_PROP(borderColor);
  JS_PROP(borderWidth);
  JS_PROP(colSpan);
  JS_PROP(editValue);
  JS_PROP(fillColor);
  JS_PROP(fontColor);
  JS_PROP(formatMessage);
  JS_PROP(formattedValue);
  JS_PROP(h);
  JS_PROP(hAlign);
  JS_PROP(locale);
  JS_PROP(mandatory);
  JS_PROP(mandatoryMessage);
  JS_PROP(maxH);
  JS_PROP(maxW);
  JS_PROP(minH);
  JS_PROP(minW);
  JS_PROP(parentSubform);
  JS_PROP(presence);
  JS_PROP(rawValue);
  JS_PROP(relevant);
  JS_PROP(rotate);
  JS_PROP(selectedIndex);
  JS_PROP(use);
  JS_PROP(usehref);
  JS_PROP(validationMessage);
  JS_PROP(vAlign);
  JS_PROP(w);
  JS_PROP(x);
  JS_PROP(y);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_FIELD_H_
