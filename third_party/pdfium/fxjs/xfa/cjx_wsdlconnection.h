// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_WSDLCONNECTION_H_
#define FXJS_XFA_CJX_WSDLCONNECTION_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_WsdlConnection;

class CJX_WsdlConnection : public CJX_Node {
 public:
  explicit CJX_WsdlConnection(CXFA_WsdlConnection* connection);
  ~CJX_WsdlConnection() override;

  JS_METHOD(execute, CJX_WsdlConnection);

  JS_PROP(dataDescription);
  JS_PROP(execute);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_WSDLCONNECTION_H_
