// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_AURA_TEST_SUITE_SETUP_H_
#define UI_AURA_TEST_AURA_TEST_SUITE_SETUP_H_

#include <memory>

#include "base/macros.h"
#include "ui/base/ui_features.h"

namespace ui {
class ContextFactory;
class InputDeviceClient;
}  // namespace ui

namespace aura {

class Env;

#if BUILDFLAG(ENABLE_MUS)
class TestWindowTreeClientDelegate;
class TestWindowTreeClientSetup;
#endif

// Use this in TestSuites that use aura. It configures aura appropriately based
// on the command line.
class AuraTestSuiteSetup {
 public:
  AuraTestSuiteSetup();
  ~AuraTestSuiteSetup();

 private:
#if BUILDFLAG(ENABLE_MUS)
  void ConfigureMus();
#endif

  std::unique_ptr<aura::Env> env_;

#if BUILDFLAG(ENABLE_MUS)
  std::unique_ptr<ui::ContextFactory> context_factory_;
  std::unique_ptr<TestWindowTreeClientDelegate>
      test_window_tree_client_delegate_;
  std::unique_ptr<TestWindowTreeClientSetup> window_tree_client_setup_;
#endif

#if defined(USE_OZONE)
  std::unique_ptr<ui::InputDeviceClient> input_device_client_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AuraTestSuiteSetup);
};

}  // namespace aura

#endif  // UI_AURA_TEST_AURA_TEST_SUITE_SETUP_H_
