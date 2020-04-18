// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PACKET_H_
#define FXJS_XFA_CJX_PACKET_H_

#include "fxjs/CJX_Define.h"
#include "fxjs/xfa/cjx_node.h"

class CXFA_Packet;

class CJX_Packet : public CJX_Node {
 public:
  explicit CJX_Packet(CXFA_Packet* packet);
  ~CJX_Packet() override;

  JS_METHOD(getAttribute, CJX_Packet);
  JS_METHOD(removeAttribute, CJX_Packet);
  JS_METHOD(setAttribute, CJX_Packet);

  JS_PROP(content);

 private:
  static const CJX_MethodSpec MethodSpecs[];
};

#endif  // FXJS_XFA_CJX_PACKET_H_
