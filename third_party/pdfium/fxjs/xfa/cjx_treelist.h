// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_TREELIST_H_
#define FXJS_XFA_CJX_TREELIST_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_list.h"

class CXFA_TreeList;

class CJX_TreeList : public CJX_List {
 public:
  explicit CJX_TreeList(CXFA_TreeList* list);
  ~CJX_TreeList() override;

  JS_METHOD(namedItem, CJX_TreeList);

 private:
  CXFA_TreeList* GetXFATreeList();

  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_TREELIST_H_
