// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_INSTANCEMANAGER_H_
#define FXJS_XFA_CJX_INSTANCEMANAGER_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_InstanceManager;

class CJX_InstanceManager : public CJX_Node {
 public:
  explicit CJX_InstanceManager(CXFA_InstanceManager* mgr);
  ~CJX_InstanceManager() override;

  JS_METHOD(addInstance, CJX_InstanceManager);
  JS_METHOD(insertInstance, CJX_InstanceManager);
  JS_METHOD(moveInstance, CJX_InstanceManager);
  JS_METHOD(removeInstance, CJX_InstanceManager);
  JS_METHOD(setInstances, CJX_InstanceManager);

  JS_PROP(count);
  JS_PROP(max);
  JS_PROP(min);

  int32_t MoveInstance(int32_t iTo, int32_t iFrom);

 private:
  int32_t SetInstances(int32_t iDesired);

  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_INSTANCEMANAGER_H_
