// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_ENV_TEST_HELPER_H_
#define UI_AURA_TEST_ENV_TEST_HELPER_H_

#include <utility>

#include "base/macros.h"
#include "ui/aura/env.h"
#include "ui/aura/env_input_state_controller.h"
#include "ui/aura/input_state_lookup.h"

namespace aura {
namespace test {

// Used to set the WindowTreeClient of Env. The constructor installs the
// supplied WindowTreeClient and the destructor restores the WindowTreeClient
// to what it previously was.
class EnvWindowTreeClientSetter {
 public:
  explicit EnvWindowTreeClientSetter(WindowTreeClient* client);
  ~EnvWindowTreeClientSetter();

 private:
  void SetWindowTreeClient(WindowTreeClient* client);

  WindowTreeClient* supplied_client_;
  WindowTreeClient* previous_client_;

  DISALLOW_COPY_AND_ASSIGN(EnvWindowTreeClientSetter);
};

class EnvTestHelper {
 public:
  EnvTestHelper() : EnvTestHelper(Env::GetInstance()) {}
  explicit EnvTestHelper(Env* env) : env_(env) {}
  ~EnvTestHelper() {}

  void SetInputStateLookup(
      std::unique_ptr<InputStateLookup> input_state_lookup) {
    env_->input_state_lookup_ = std::move(input_state_lookup);
  }

  void ResetEventState() {
    env_->mouse_button_flags_ = 0;
    env_->is_touch_down_ = false;
    env_->last_mouse_location_ = gfx::Point();
    env_->env_controller_->touch_ids_down_ = 0;
  }

  void SetMode(Env::Mode mode) {
    env_->mode_ = mode;
    if (mode == Env::Mode::MUS)
      env_->EnableMusOSExchangeDataProvider();
  }

  // Use to force Env::last_mouse_location() to return the value last set.
  // This matters for MUS, which may not return the last explicitly set
  // location.
  void SetAlwaysUseLastMouseLocation(bool value) {
    env_->always_use_last_mouse_location_ = value;
  }

  WindowTreeClient* GetWindowTreeClient() { return env_->window_tree_client_; }

 private:
  Env* env_;

  DISALLOW_COPY_AND_ASSIGN(EnvTestHelper);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_ENV_TEST_HELPER_H_
