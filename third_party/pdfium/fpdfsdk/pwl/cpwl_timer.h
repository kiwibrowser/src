// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_PWL_CPWL_TIMER_H_
#define FPDFSDK_PWL_CPWL_TIMER_H_

#include "core/fxcrt/unowned_ptr.h"

class CFX_SystemHandler;
class CPWL_TimerHandler;

class CPWL_Timer {
 public:
  CPWL_Timer(CPWL_TimerHandler* pAttached, CFX_SystemHandler* pSystemHandler);
  virtual ~CPWL_Timer();

  static void TimerProc(int32_t idEvent);

  int32_t SetPWLTimer(int32_t nElapse);
  void KillPWLTimer();

 private:
  int32_t m_nTimerID;
  UnownedPtr<CPWL_TimerHandler> m_pAttached;
  UnownedPtr<CFX_SystemHandler> m_pSystemHandler;
};

#endif  // FPDFSDK_PWL_CPWL_TIMER_H_
