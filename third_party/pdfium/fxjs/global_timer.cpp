// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/global_timer.h"

GlobalTimer::GlobalTimer(CJS_App* pObj,
                         CPDFSDK_FormFillEnvironment* pFormFillEnv,
                         CJS_Runtime* pRuntime,
                         int nType,
                         const WideString& script,
                         uint32_t dwElapse,
                         uint32_t dwTimeOut)
    : m_nTimerID(0),
      m_pEmbedApp(pObj),
      m_bProcessing(false),
      m_nType(nType),
      m_dwTimeOut(dwTimeOut),
      m_swJScript(script),
      m_pRuntime(pRuntime),
      m_pFormFillEnv(pFormFillEnv) {
  CFX_SystemHandler* pHandler = m_pFormFillEnv->GetSysHandler();
  m_nTimerID = pHandler->SetTimer(dwElapse, Trigger);
  if (m_nTimerID)
    (*GetGlobalTimerMap())[m_nTimerID] = this;
}

GlobalTimer::~GlobalTimer() {
  if (!m_nTimerID)
    return;

  if (GetRuntime())
    m_pFormFillEnv->GetSysHandler()->KillTimer(m_nTimerID);

  GetGlobalTimerMap()->erase(m_nTimerID);
}

// static
void GlobalTimer::Trigger(int nTimerID) {
  auto it = GetGlobalTimerMap()->find(nTimerID);
  if (it == GetGlobalTimerMap()->end())
    return;

  GlobalTimer* pTimer = it->second;
  if (pTimer->m_bProcessing)
    return;

  pTimer->m_bProcessing = true;
  if (pTimer->m_pEmbedApp)
    pTimer->m_pEmbedApp->TimerProc(pTimer);

  // Timer proc may have destroyed timer, find it again.
  it = GetGlobalTimerMap()->find(nTimerID);
  if (it == GetGlobalTimerMap()->end())
    return;

  pTimer = it->second;
  pTimer->m_bProcessing = false;
  if (pTimer->IsOneShot())
    pTimer->m_pEmbedApp->CancelProc(pTimer);
}

// static
void GlobalTimer::Cancel(int nTimerID) {
  auto it = GetGlobalTimerMap()->find(nTimerID);
  if (it == GetGlobalTimerMap()->end())
    return;

  GlobalTimer* pTimer = it->second;
  pTimer->m_pEmbedApp->CancelProc(pTimer);
}

// static
GlobalTimer::TimerMap* GlobalTimer::GetGlobalTimerMap() {
  // Leak the timer array at shutdown.
  static auto* s_TimerMap = new TimerMap;
  return s_TimerMap;
}
