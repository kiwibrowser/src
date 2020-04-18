// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_LAYOUTPSEUDOMODEL_H_
#define FXJS_XFA_CJX_LAYOUTPSEUDOMODEL_H_

#include <vector>

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_object.h"

enum XFA_LAYOUTMODEL_HWXY {
  XFA_LAYOUTMODEL_H,
  XFA_LAYOUTMODEL_W,
  XFA_LAYOUTMODEL_X,
  XFA_LAYOUTMODEL_Y
};

class CFXJSE_Value;
class CScript_LayoutPseudoModel;
class CXFA_LayoutProcessor;
class CXFA_Node;

class CJX_LayoutPseudoModel : public CJX_Object {
 public:
  explicit CJX_LayoutPseudoModel(CScript_LayoutPseudoModel* model);
  ~CJX_LayoutPseudoModel() override;

  JS_METHOD(absPage, CJX_LayoutPseudoModel);
  JS_METHOD(absPageCount, CJX_LayoutPseudoModel);
  JS_METHOD(absPageCountInBatch, CJX_LayoutPseudoModel);
  JS_METHOD(absPageInBatch, CJX_LayoutPseudoModel);
  JS_METHOD(absPageSpan, CJX_LayoutPseudoModel);
  JS_METHOD(h, CJX_LayoutPseudoModel);
  JS_METHOD(page, CJX_LayoutPseudoModel);
  JS_METHOD(pageContent, CJX_LayoutPseudoModel);
  JS_METHOD(pageCount, CJX_LayoutPseudoModel);
  JS_METHOD(pageSpan, CJX_LayoutPseudoModel);
  JS_METHOD(relayout, CJX_LayoutPseudoModel);
  JS_METHOD(relayoutPageArea, CJX_LayoutPseudoModel);
  JS_METHOD(sheet, CJX_LayoutPseudoModel);
  JS_METHOD(sheetCount, CJX_LayoutPseudoModel);
  JS_METHOD(sheetCountInBatch, CJX_LayoutPseudoModel);
  JS_METHOD(sheetInBatch, CJX_LayoutPseudoModel);
  JS_METHOD(w, CJX_LayoutPseudoModel);
  JS_METHOD(x, CJX_LayoutPseudoModel);
  JS_METHOD(y, CJX_LayoutPseudoModel);

  JS_PROP(ready);

 private:
  CJS_Return NumberedPageCount(CFX_V8* runtime, bool bNumbered);
  CJS_Return HWXY(CFX_V8* runtime,
                  const std::vector<v8::Local<v8::Value>>& params,
                  XFA_LAYOUTMODEL_HWXY layoutModel);
  std::vector<CXFA_Node*> GetObjArray(CXFA_LayoutProcessor* pDocLayout,
                                      int32_t iPageNo,
                                      const WideString& wsType,
                                      bool bOnPageArea);
  CJS_Return PageInternals(CFX_V8* runtime,
                           const std::vector<v8::Local<v8::Value>>& params,
                           bool bAbsPage);

  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_LAYOUTPSEUDOMODEL_H_
