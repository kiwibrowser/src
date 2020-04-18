// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_WAITER_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_WAITER_H_

#include <list>

#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

// DialogEventWaiter is used to wait on specific events that may have occured
// before the call to Wait(), or after, in which case a RunLoop is used.
//
// Usage:
// waiter_ = std::make_unique<DialogEventWaiter>({ ... });
//
// Do stuff, which (a)synchronously calls waiter_->OnEvent(...).
//
// waiter_->Wait();  <- Will either return right away if events were observed,
//                   <- or use a RunLoop's Run/Quit to wait.
template <typename DialogEvent>
class DialogEventWaiter {
 public:
  explicit DialogEventWaiter(std::list<DialogEvent> event_sequence);
  ~DialogEventWaiter();

  // Either returns right away if all events were observed between this
  // object's construction and this call to Wait(), or use a RunLoop to wait
  // for them.
  void Wait();

  // Observes an event (quits the RunLoop if we are done waiting).
  void OnEvent(DialogEvent event);

 private:
  std::list<DialogEvent> events_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(DialogEventWaiter);
};

template <typename DialogEvent>
DialogEventWaiter<DialogEvent>::DialogEventWaiter(
    std::list<DialogEvent> event_sequence)
    : events_(std::move(event_sequence)) {}

template <typename DialogEvent>
DialogEventWaiter<DialogEvent>::~DialogEventWaiter() {}

template <typename DialogEvent>
void DialogEventWaiter<DialogEvent>::Wait() {
  if (events_.empty())
    return;

  DCHECK(!run_loop_.running());
  run_loop_.Run();
}

template <typename DialogEvent>
void DialogEventWaiter<DialogEvent>::OnEvent(DialogEvent event) {
  if (events_.empty())
    return;

  ASSERT_EQ(events_.front(), event);
  events_.pop_front();
  // Only quit the loop if no other events are expected.
  if (events_.empty() && run_loop_.running())
    run_loop_.Quit();
}

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_EVENT_WAITER_H_
