// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/mus/change_completion_waiter.h"

#include "base/callback.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/in_flight_change.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/mus/window_tree_client_private.h"

namespace aura {
namespace test {
namespace {

WindowTreeClient* GetWindowTreeClient() {
  return EnvTestHelper().GetWindowTreeClient();
}

// A class which will Wait until there are no more in-flight-changes. That is,
// all changes sent to mus have been acked.
class AllChangesCompletedWaiter : public WindowTreeClientTestObserver {
 public:
  explicit AllChangesCompletedWaiter(WindowTreeClient* client);
  ~AllChangesCompletedWaiter() override;

  void Wait();

 private:
  // WindowTreeClientTestObserver:
  void OnChangeStarted(uint32_t change_id, aura::ChangeType type) override;
  void OnChangeCompleted(uint32_t change_id,
                         aura::ChangeType type,
                         bool success) override;

  base::RunLoop run_loop_;
  base::Closure quit_closure_;
  WindowTreeClient* client_;

  DISALLOW_COPY_AND_ASSIGN(AllChangesCompletedWaiter);
};

AllChangesCompletedWaiter::AllChangesCompletedWaiter(WindowTreeClient* client)
    : client_(client) {}

AllChangesCompletedWaiter::~AllChangesCompletedWaiter() = default;

void AllChangesCompletedWaiter::Wait() {
  if (!WindowTreeClientPrivate(client_).HasInFlightChanges())
    return;

  client_->AddTestObserver(this);
  quit_closure_ = run_loop_.QuitClosure();
  run_loop_.Run();
  client_->RemoveTestObserver(this);
}

void AllChangesCompletedWaiter::OnChangeStarted(uint32_t change_id,
                                                aura::ChangeType type) {}

void AllChangesCompletedWaiter::OnChangeCompleted(uint32_t change_id,
                                                  aura::ChangeType type,
                                                  bool success) {
  if (!WindowTreeClientPrivate(client_).HasInFlightChanges())
    quit_closure_.Run();
}

}  // namespace

ChangeCompletionWaiter::ChangeCompletionWaiter(ChangeType type, bool success)
    : state_(WaitState::NOT_STARTED),
      client_(GetWindowTreeClient()),
      type_(type),
      success_(success) {
  client_->AddTestObserver(this);
}

ChangeCompletionWaiter::~ChangeCompletionWaiter() {
  client_->RemoveTestObserver(this);
}

bool ChangeCompletionWaiter::Wait() {
  if (state_ != WaitState::RECEIVED) {
    quit_closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
  }
  return success_matched_;
}

void ChangeCompletionWaiter::OnChangeStarted(uint32_t change_id,
                                             aura::ChangeType type) {
  if (state_ == WaitState::NOT_STARTED && type == type_) {
    state_ = WaitState::WAITING;
    change_id_ = change_id;
  }
}

void ChangeCompletionWaiter::OnChangeCompleted(uint32_t change_id,
                                               aura::ChangeType type,
                                               bool success) {
  if (state_ == WaitState::WAITING && change_id_ == change_id) {
    success_matched_ = success_ == success;
    EXPECT_EQ(success_, success);
    state_ = WaitState::RECEIVED;
    if (quit_closure_)
      quit_closure_.Run();
  }
}

void WaitForAllChangesToComplete(WindowTreeClient* client) {
  if (Env::GetInstance()->mode() == Env::Mode::LOCAL)
    return;
  AllChangesCompletedWaiter(client ? client : GetWindowTreeClient()).Wait();
}

}  // namespace test
}  // namespace aura
