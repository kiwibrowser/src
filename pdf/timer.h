// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_TIMER_H_
#define PDF_TIMER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace chrome_pdf {

// Timer implementation for pepper plugins, based on pp::Core::CallOnMainThread.
// We can not use base::Timer for plugins, because they have no
// base::MessageLoop, on which it is based.
class Timer {
 public:
  explicit Timer(base::TimeDelta delay);
  virtual ~Timer();

  virtual void OnTimer() = 0;

 private:
  void PostCallback();
  void TimerProc(int32_t result);

  const base::TimeDelta delay_;
  pp::CompletionCallbackFactory<Timer> callback_factory_;

  DISALLOW_COPY_AND_ASSIGN(Timer);
};

}  // namespace chrome_pdf

#endif  // PDF_TIMER_H_
