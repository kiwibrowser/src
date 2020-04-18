// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_SOURCE_H_
#define FXJS_XFA_CJX_SOURCE_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_Source;

class CJX_Source : public CJX_Node {
 public:
  explicit CJX_Source(CXFA_Source* src);
  ~CJX_Source() override;

  JS_METHOD(addNew, CJX_Source);
  JS_METHOD(cancel, CJX_Source);
  JS_METHOD(cancelBatch, CJX_Source);
  JS_METHOD(close, CJX_Source);
  JS_METHOD(deleteItem /*delete*/, CJX_Source);
  JS_METHOD(first, CJX_Source);
  JS_METHOD(hasDataChanged, CJX_Source);
  JS_METHOD(isBOF, CJX_Source);
  JS_METHOD(isEOF, CJX_Source);
  JS_METHOD(last, CJX_Source);
  JS_METHOD(next, CJX_Source);
  JS_METHOD(open, CJX_Source);
  JS_METHOD(previous, CJX_Source);
  JS_METHOD(requery, CJX_Source);
  JS_METHOD(resync, CJX_Source);
  JS_METHOD(update, CJX_Source);
  JS_METHOD(updateBatch, CJX_Source);

  JS_PROP(db);
  JS_PROP(use);
  JS_PROP(usehref);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_SOURCE_H_
