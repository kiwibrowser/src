// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_EVENT_H_
#define XFA_FXFA_PARSER_CXFA_EVENT_H_

#include "xfa/fxfa/parser/cxfa_node.h"

class CXFA_Script;
class CXFA_Submit;

class CXFA_Event : public CXFA_Node {
 public:
  CXFA_Event(CXFA_Document* doc, XFA_PacketType packet);
  ~CXFA_Event() override;

  XFA_AttributeEnum GetActivity();
  XFA_Element GetEventType() const;
  CXFA_Script* GetScriptIfExists();
  CXFA_Submit* GetSubmitIfExists();
  WideString GetRef();
};

#endif  // XFA_FXFA_PARSER_CXFA_EVENT_H_
