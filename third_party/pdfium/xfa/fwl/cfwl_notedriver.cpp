// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_notedriver.h"

#include <algorithm>
#include <utility>

#include "core/fxcrt/fx_extension.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"
#include "xfa/fwl/cfwl_app.h"
#include "xfa/fwl/cfwl_eventtarget.h"
#include "xfa/fwl/cfwl_form.h"
#include "xfa/fwl/cfwl_messagekey.h"
#include "xfa/fwl/cfwl_messagekillfocus.h"
#include "xfa/fwl/cfwl_messagemouse.h"
#include "xfa/fwl/cfwl_messagemousewheel.h"
#include "xfa/fwl/cfwl_messagesetfocus.h"
#include "xfa/fwl/cfwl_noteloop.h"
#include "xfa/fwl/cfwl_widgetmgr.h"

CFWL_NoteDriver::CFWL_NoteDriver()
    : m_pHover(nullptr),
      m_pFocus(nullptr),
      m_pGrab(nullptr),
      m_pNoteLoop(pdfium::MakeUnique<CFWL_NoteLoop>()) {
  PushNoteLoop(m_pNoteLoop.get());
}

CFWL_NoteDriver::~CFWL_NoteDriver() {}

void CFWL_NoteDriver::SendEvent(CFWL_Event* pNote) {
  for (const auto& pair : m_eventTargets) {
    if (pair.second->IsValid())
      pair.second->ProcessEvent(pNote);
  }
}

void CFWL_NoteDriver::RegisterEventTarget(CFWL_Widget* pListener,
                                          CFWL_Widget* pEventSource) {
  uint32_t key = pListener->GetEventKey();
  if (key == 0) {
    do {
      key = rand();
    } while (key == 0 || pdfium::ContainsKey(m_eventTargets, key));
    pListener->SetEventKey(key);
  }
  if (!m_eventTargets[key])
    m_eventTargets[key] = pdfium::MakeUnique<CFWL_EventTarget>(pListener);

  m_eventTargets[key]->SetEventSource(pEventSource);
}

void CFWL_NoteDriver::UnregisterEventTarget(CFWL_Widget* pListener) {
  uint32_t key = pListener->GetEventKey();
  if (key == 0)
    return;

  auto it = m_eventTargets.find(key);
  if (it != m_eventTargets.end())
    it->second->FlagInvalid();
}

void CFWL_NoteDriver::PushNoteLoop(CFWL_NoteLoop* pNoteLoop) {
  m_NoteLoopQueue.push_back(pNoteLoop);
}

CFWL_NoteLoop* CFWL_NoteDriver::PopNoteLoop() {
  if (m_NoteLoopQueue.empty())
    return nullptr;

  CFWL_NoteLoop* p = m_NoteLoopQueue.back();
  m_NoteLoopQueue.pop_back();
  return p;
}

bool CFWL_NoteDriver::SetFocus(CFWL_Widget* pFocus) {
  if (m_pFocus == pFocus)
    return true;

  CFWL_Widget* pPrev = m_pFocus;
  m_pFocus = pFocus;
  if (pPrev) {
    if (IFWL_WidgetDelegate* pDelegate = pPrev->GetDelegate()) {
      CFWL_MessageKillFocus ms(pPrev, pPrev);
      pDelegate->OnProcessMessage(&ms);
    }
  }
  if (pFocus) {
    CFWL_Widget* pWidget =
        pFocus->GetOwnerApp()->GetWidgetMgr()->GetSystemFormWidget(pFocus);
    CFWL_Form* pForm = static_cast<CFWL_Form*>(pWidget);
    if (pForm)
      pForm->SetSubFocus(pFocus);

    if (IFWL_WidgetDelegate* pDelegate = pFocus->GetDelegate()) {
      CFWL_MessageSetFocus ms(nullptr, pFocus);
      pDelegate->OnProcessMessage(&ms);
    }
  }
  return true;
}

void CFWL_NoteDriver::Run() {
#if _FX_OS_ == _FX_OS_LINUX_ || _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  for (;;) {
    CFWL_NoteLoop* pTopLoop = GetTopLoop();
    if (!pTopLoop || !pTopLoop->ContinueModal())
      break;
    UnqueueMessageAndProcess(pTopLoop);
  }
#endif  // _FX_OS_ == _FX_OS_LINUX_ || _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
}

void CFWL_NoteDriver::NotifyTargetHide(CFWL_Widget* pNoteTarget) {
  if (m_pFocus == pNoteTarget)
    m_pFocus = nullptr;
  if (m_pHover == pNoteTarget)
    m_pHover = nullptr;
  if (m_pGrab == pNoteTarget)
    m_pGrab = nullptr;
}

void CFWL_NoteDriver::NotifyTargetDestroy(CFWL_Widget* pNoteTarget) {
  if (m_pFocus == pNoteTarget)
    m_pFocus = nullptr;
  if (m_pHover == pNoteTarget)
    m_pHover = nullptr;
  if (m_pGrab == pNoteTarget)
    m_pGrab = nullptr;

  UnregisterEventTarget(pNoteTarget);

  for (CFWL_Widget* pWidget : m_Forms) {
    CFWL_Form* pForm = static_cast<CFWL_Form*>(pWidget);
    if (!pForm)
      continue;

    CFWL_Widget* pSubFocus = pForm->GetSubFocus();
    if (!pSubFocus)
      return;

    if (pSubFocus == pNoteTarget)
      pForm->SetSubFocus(nullptr);
  }
}

void CFWL_NoteDriver::RegisterForm(CFWL_Widget* pForm) {
  if (!pForm || pdfium::ContainsValue(m_Forms, pForm))
    return;

  m_Forms.push_back(pForm);
  if (m_Forms.size() == 1 && !m_NoteLoopQueue.empty() && m_NoteLoopQueue[0])
    m_NoteLoopQueue[0]->SetMainForm(pForm);
}

void CFWL_NoteDriver::UnRegisterForm(CFWL_Widget* pForm) {
  auto iter = std::find(m_Forms.begin(), m_Forms.end(), pForm);
  if (iter != m_Forms.end())
    m_Forms.erase(iter);
}

void CFWL_NoteDriver::QueueMessage(std::unique_ptr<CFWL_Message> pMessage) {
  m_NoteQueue.push_back(std::move(pMessage));
}

void CFWL_NoteDriver::UnqueueMessageAndProcess(CFWL_NoteLoop* pNoteLoop) {
  if (m_NoteQueue.empty())
    return;

  std::unique_ptr<CFWL_Message> pMessage = std::move(m_NoteQueue.front());
  m_NoteQueue.pop_front();
  if (!IsValidMessage(pMessage.get()))
    return;

  ProcessMessage(std::move(pMessage));
}

CFWL_NoteLoop* CFWL_NoteDriver::GetTopLoop() const {
  return !m_NoteLoopQueue.empty() ? m_NoteLoopQueue.back() : nullptr;
}

void CFWL_NoteDriver::ProcessMessage(std::unique_ptr<CFWL_Message> pMessage) {
  CFWL_Widget* pMessageForm = pMessage->m_pDstTarget;
  if (!pMessageForm)
    return;

  if (!DispatchMessage(pMessage.get(), pMessageForm))
    return;

  if (pMessage->GetType() == CFWL_Message::Type::Mouse)
    MouseSecondary(pMessage.get());
}

bool CFWL_NoteDriver::DispatchMessage(CFWL_Message* pMessage,
                                      CFWL_Widget* pMessageForm) {
  switch (pMessage->GetType()) {
    case CFWL_Message::Type::SetFocus: {
      if (!DoSetFocus(pMessage, pMessageForm))
        return false;
      break;
    }
    case CFWL_Message::Type::KillFocus: {
      if (!DoKillFocus(pMessage, pMessageForm))
        return false;
      break;
    }
    case CFWL_Message::Type::Key: {
      if (!DoKey(pMessage, pMessageForm))
        return false;
      break;
    }
    case CFWL_Message::Type::Mouse: {
      if (!DoMouse(pMessage, pMessageForm))
        return false;
      break;
    }
    case CFWL_Message::Type::MouseWheel: {
      if (!DoWheel(pMessage, pMessageForm))
        return false;
      break;
    }
    default:
      break;
  }
  if (IFWL_WidgetDelegate* pDelegate = pMessage->m_pDstTarget->GetDelegate())
    pDelegate->OnProcessMessage(pMessage);

  return true;
}

bool CFWL_NoteDriver::DoSetFocus(CFWL_Message* pMessage,
                                 CFWL_Widget* pMessageForm) {
  m_pFocus = pMessage->m_pDstTarget;
  return true;
}

bool CFWL_NoteDriver::DoKillFocus(CFWL_Message* pMessage,
                                  CFWL_Widget* pMessageForm) {
  if (m_pFocus == pMessage->m_pDstTarget)
    m_pFocus = nullptr;
  return true;
}

bool CFWL_NoteDriver::DoKey(CFWL_Message* pMessage, CFWL_Widget* pMessageForm) {
  CFWL_MessageKey* pMsg = static_cast<CFWL_MessageKey*>(pMessage);
#if (_FX_OS_ != _FX_OS_MACOSX_)
  if (pMsg->m_dwCmd == FWL_KeyCommand::KeyDown &&
      pMsg->m_dwKeyCode == FWL_VKEY_Tab) {
    CFWL_WidgetMgr* pWidgetMgr = pMessageForm->GetOwnerApp()->GetWidgetMgr();
    CFWL_Widget* pForm = GetMessageForm(pMsg->m_pDstTarget);
    CFWL_Widget* pFocus = m_pFocus;
    if (m_pFocus && pWidgetMgr->GetSystemFormWidget(m_pFocus) != pForm)
      pFocus = nullptr;

    bool bFind = false;
    CFWL_Widget* pNextTabStop = pWidgetMgr->NextTab(pForm, pFocus, bFind);
    if (!pNextTabStop) {
      bFind = false;
      pNextTabStop = pWidgetMgr->NextTab(pForm, nullptr, bFind);
    }
    if (pNextTabStop == pFocus)
      return true;
    if (pNextTabStop)
      SetFocus(pNextTabStop);
    return true;
  }
#endif

  if (!m_pFocus) {
    if (pMsg->m_dwCmd == FWL_KeyCommand::KeyDown &&
        pMsg->m_dwKeyCode == FWL_VKEY_Return) {
      CFWL_WidgetMgr* pWidgetMgr = pMessageForm->GetOwnerApp()->GetWidgetMgr();
      CFWL_Widget* defButton = pWidgetMgr->GetDefaultButton(pMessageForm);
      if (defButton) {
        pMsg->m_pDstTarget = defButton;
        return true;
      }
    }
    return false;
  }
  pMsg->m_pDstTarget = m_pFocus;
  return true;
}

bool CFWL_NoteDriver::DoMouse(CFWL_Message* pMessage,
                              CFWL_Widget* pMessageForm) {
  CFWL_MessageMouse* pMsg = static_cast<CFWL_MessageMouse*>(pMessage);
  if (pMsg->m_dwCmd == FWL_MouseCommand::Leave ||
      pMsg->m_dwCmd == FWL_MouseCommand::Hover ||
      pMsg->m_dwCmd == FWL_MouseCommand::Enter) {
    return !!pMsg->m_pDstTarget;
  }
  if (pMsg->m_pDstTarget != pMessageForm)
    pMsg->m_pos = pMsg->m_pDstTarget->TransformTo(pMessageForm, pMsg->m_pos);
  if (!DoMouseEx(pMsg, pMessageForm))
    pMsg->m_pDstTarget = pMessageForm;
  return true;
}

bool CFWL_NoteDriver::DoWheel(CFWL_Message* pMessage,
                              CFWL_Widget* pMessageForm) {
  CFWL_WidgetMgr* pWidgetMgr = pMessageForm->GetOwnerApp()->GetWidgetMgr();
  if (!pWidgetMgr)
    return false;

  CFWL_MessageMouseWheel* pMsg = static_cast<CFWL_MessageMouseWheel*>(pMessage);
  CFWL_Widget* pDst = pWidgetMgr->GetWidgetAtPoint(pMessageForm, pMsg->m_pos);
  if (!pDst)
    return false;

  pMsg->m_pos = pMessageForm->TransformTo(pDst, pMsg->m_pos);
  pMsg->m_pDstTarget = pDst;
  return true;
}

bool CFWL_NoteDriver::DoMouseEx(CFWL_Message* pMessage,
                                CFWL_Widget* pMessageForm) {
  CFWL_WidgetMgr* pWidgetMgr = pMessageForm->GetOwnerApp()->GetWidgetMgr();
  if (!pWidgetMgr)
    return false;
  CFWL_Widget* pTarget = nullptr;
  if (m_pGrab)
    pTarget = m_pGrab;

  CFWL_MessageMouse* pMsg = static_cast<CFWL_MessageMouse*>(pMessage);
  if (!pTarget)
    pTarget = pWidgetMgr->GetWidgetAtPoint(pMessageForm, pMsg->m_pos);
  if (!pTarget)
    return false;
  if (pTarget && pMessageForm != pTarget)
    pMsg->m_pos = pMessageForm->TransformTo(pTarget, pMsg->m_pos);

  pMsg->m_pDstTarget = pTarget;
  return true;
}

void CFWL_NoteDriver::MouseSecondary(CFWL_Message* pMessage) {
  CFWL_Widget* pTarget = pMessage->m_pDstTarget;
  if (pTarget == m_pHover)
    return;

  CFWL_MessageMouse* pMsg = static_cast<CFWL_MessageMouse*>(pMessage);
  if (m_pHover) {
    CFWL_MessageMouse msLeave(nullptr, m_pHover);
    msLeave.m_pos = pTarget->TransformTo(m_pHover, pMsg->m_pos);
    msLeave.m_dwFlags = 0;
    msLeave.m_dwCmd = FWL_MouseCommand::Leave;
    DispatchMessage(&msLeave, nullptr);
  }
  if (pTarget->GetClassID() == FWL_Type::Form) {
    m_pHover = nullptr;
    return;
  }
  m_pHover = pTarget;

  CFWL_MessageMouse msHover(nullptr, pTarget);
  msHover.m_pos = pMsg->m_pos;
  msHover.m_dwFlags = 0;
  msHover.m_dwCmd = FWL_MouseCommand::Hover;
  DispatchMessage(&msHover, nullptr);
}

bool CFWL_NoteDriver::IsValidMessage(CFWL_Message* pMessage) {
  for (CFWL_NoteLoop* pNoteLoop : m_NoteLoopQueue) {
    CFWL_Widget* pForm = pNoteLoop->GetForm();
    if (pForm && pForm == pMessage->m_pDstTarget)
      return true;
  }
  for (CFWL_Widget* pWidget : m_Forms) {
    CFWL_Form* pForm = static_cast<CFWL_Form*>(pWidget);
    if (pForm == pMessage->m_pDstTarget)
      return true;
  }
  return false;
}

CFWL_Widget* CFWL_NoteDriver::GetMessageForm(CFWL_Widget* pDstTarget) {
  if (m_NoteLoopQueue.empty())
    return nullptr;

  CFWL_Widget* pMessageForm = nullptr;
  if (m_NoteLoopQueue.size() > 1)
    pMessageForm = m_NoteLoopQueue.back()->GetForm();
  else if (!pdfium::ContainsValue(m_Forms, pDstTarget))
    pMessageForm = pDstTarget;

  if (!pMessageForm && pDstTarget) {
    CFWL_WidgetMgr* pWidgetMgr = pDstTarget->GetOwnerApp()->GetWidgetMgr();
    if (!pWidgetMgr)
      return nullptr;

    pMessageForm = pWidgetMgr->GetSystemFormWidget(pDstTarget);
  }
  return pMessageForm;
}

void CFWL_NoteDriver::ClearEventTargets() {
  auto it = m_eventTargets.begin();
  while (it != m_eventTargets.end()) {
    auto old = it++;
    if (!old->second->IsValid())
      m_eventTargets.erase(old);
  }
}
