// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_MUS_CHANGE_COMPLETION_WAITER_H_
#define UI_AURA_TEST_MUS_CHANGE_COMPLETION_WAITER_H_

#include "base/callback_forward.h"
#include "base/run_loop.h"
#include "ui/aura/mus/window_tree_client_test_observer.h"

namespace aura {
class WindowTreeClient;
enum class ChangeType;

namespace test {

// A class which will Wait for next change of |type| to complete.
class ChangeCompletionWaiter : public WindowTreeClientTestObserver {
 public:
  ChangeCompletionWaiter(ChangeType type, bool success);
  ~ChangeCompletionWaiter() override;

  // Wait for the first change that occurred after construction of this object
  // of |type| to complete. May return immediately if it's already done. Returns
  // true if a change was encountered whose success state matches that of
  // the supplied success state, false otherwise.
  bool Wait();

 private:
  // WindowTreeClientTestObserver:
  void OnChangeStarted(uint32_t change_id,
                       aura::ChangeType type) override;
  void OnChangeCompleted(uint32_t change_id,
                         aura::ChangeType type,
                         bool success) override;

  base::RunLoop run_loop_;
  base::Closure quit_closure_;

  enum class WaitState {
    NOT_STARTED,
    WAITING,
    RECEIVED
  } state_;

  WindowTreeClient* client_;
  aura::ChangeType type_;
  uint32_t change_id_;
  bool success_;
  bool success_matched_ = true;

  DISALLOW_COPY_AND_ASSIGN(ChangeCompletionWaiter);
};

// Under mus and mash, waits until there are no more pending changes for
// |client|. If |client| is null, the default is used. Returns immediately under
// classic ash because window operations are synchronous.
void WaitForAllChangesToComplete(WindowTreeClient* client = nullptr);

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_MUS_CHANGE_COMPLETION_WAITER_H_
