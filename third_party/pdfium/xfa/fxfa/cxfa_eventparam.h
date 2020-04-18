// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_EVENTPARAM_H_
#define XFA_FXFA_CXFA_EVENTPARAM_H_

#include "xfa/fxfa/fxfa_basic.h"

class CXFA_Node;

enum XFA_EVENTTYPE {
  XFA_EVENT_Click,
  XFA_EVENT_Change,
  XFA_EVENT_DocClose,
  XFA_EVENT_DocReady,
  XFA_EVENT_Enter,
  XFA_EVENT_Exit,
  XFA_EVENT_Full,
  XFA_EVENT_IndexChange,
  XFA_EVENT_Initialize,
  XFA_EVENT_MouseDown,
  XFA_EVENT_MouseEnter,
  XFA_EVENT_MouseExit,
  XFA_EVENT_MouseUp,
  XFA_EVENT_PostExecute,
  XFA_EVENT_PostOpen,
  XFA_EVENT_PostPrint,
  XFA_EVENT_PostSave,
  XFA_EVENT_PostSign,
  XFA_EVENT_PostSubmit,
  XFA_EVENT_PreExecute,
  XFA_EVENT_PreOpen,
  XFA_EVENT_PrePrint,
  XFA_EVENT_PreSave,
  XFA_EVENT_PreSign,
  XFA_EVENT_PreSubmit,
  XFA_EVENT_Ready,
  XFA_EVENT_InitCalculate,
  XFA_EVENT_InitVariables,
  XFA_EVENT_Calculate,
  XFA_EVENT_Validate,
  XFA_EVENT_Unknown,
};

class CXFA_EventParam {
 public:
  CXFA_EventParam();
  ~CXFA_EventParam();
  CXFA_EventParam(const CXFA_EventParam& other);

  void Reset();

  CXFA_Node* m_pTarget;
  XFA_EVENTTYPE m_eType;
  WideString m_wsResult;
  bool m_bCancelAction;
  int32_t m_iCommitKey;
  bool m_bKeyDown;
  bool m_bModifier;
  bool m_bReenter;
  int32_t m_iSelEnd;
  int32_t m_iSelStart;
  bool m_bShift;
  WideString m_wsChange;
  WideString m_wsFullText;
  WideString m_wsNewContentType;
  WideString m_wsNewText;
  WideString m_wsPrevContentType;
  WideString m_wsPrevText;
  WideString m_wsSoapFaultCode;
  WideString m_wsSoapFaultString;
  bool m_bIsFormReady;
};

#endif  // XFA_FXFA_CXFA_EVENTPARAM_H_
