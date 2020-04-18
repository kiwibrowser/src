// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/timer.h"

#include "ppapi/cpp/core.h"
#include "ppapi/cpp/module.h"

namespace chrome_pdf {

Timer::Timer(base::TimeDelta delay) : delay_(delay), callback_factory_(this) {
  PostCallback();
}

Timer::~Timer() = default;

void Timer::PostCallback() {
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&Timer::TimerProc);
  pp::Module::Get()->core()->CallOnMainThread(delay_.InMilliseconds(), callback,
                                              0);
}

void Timer::TimerProc(int32_t /*result*/) {
  PostCallback();
  OnTimer();
}

}  // namespace chrome_pdf
