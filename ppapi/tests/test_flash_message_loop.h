// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAPPI_TESTS_TEST_FLASH_MESSAGE_LOOP_H_
#define PAPPI_TESTS_TEST_FLASH_MESSAGE_LOOP_H_

#include <string>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/tests/test_case.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace pp {
namespace flash {
class MessageLoop;
}
}

class TestFlashMessageLoop : public TestCase {
 public:
  explicit TestFlashMessageLoop(TestingInstance* instance);
  ~TestFlashMessageLoop() override;

  // TestCase implementation.
  void RunTests(const std::string& filter) override;

  void clear_instance_so() { instance_so_ = nullptr; }

  void DidRunScriptCallback();

 private:
  // ScriptableObject implementation.
  class InstanceSO;

  // TestCase protected overrides.
  pp::deprecated::ScriptableObject* CreateTestObject() override;

  std::string TestBasics();
  std::string TestRunWithoutQuit();
  std::string TestSuspendScriptCallbackWhileRunning();

  void TestSuspendScriptCallbackTask(int32_t unused);
  void QuitMessageLoopTask(int32_t unused);
  void DestroyMessageLoopResourceTask(int32_t unused);

  pp::flash::MessageLoop* message_loop_;

  // The scriptable object and result storage for the
  // SuspendScriptCallbackWhileRunning test.
  InstanceSO* instance_so_;
  bool suspend_script_callback_result_;

  pp::CompletionCallbackFactory<TestFlashMessageLoop> callback_factory_;
};

#endif  // PAPPI_TESTS_TEST_FLASH_MESSAGE_LOOP_H_
