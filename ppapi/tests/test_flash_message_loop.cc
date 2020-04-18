// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash_message_loop.h"

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/dev/scriptable_object_deprecated.h"
#include "ppapi/cpp/logging.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash_message_loop.h"
#include "ppapi/tests/testing_instance.h"

namespace {

const char kDidRunScriptCallback[] = "DidRunScriptCallback";

}  // namespace

class TestFlashMessageLoop::InstanceSO
    : public pp::deprecated::ScriptableObject {
 public:
  explicit InstanceSO(TestFlashMessageLoop* owner) : owner_(owner) {}

  ~InstanceSO() override {
    if (owner_)
      owner_->clear_instance_so();
  }

  // pp::deprecated::ScriptableObject overrides.
  bool HasMethod(const pp::Var& name, pp::Var* exception) override {
    if (!name.is_string())
      return false;
    return name.AsString() == kDidRunScriptCallback;
  }

  pp::Var Call(const pp::Var& method_name,
               const std::vector<pp::Var>& args,
               pp::Var* exception) override {
    if (!method_name.is_string())
      return false;
    std::string name = method_name.AsString();

    if (name == kDidRunScriptCallback) {
      if (args.size() != 0) {
        *exception = pp::Var("Bad argument to DidRunScriptCallback()");
      } else if (owner_) {
        owner_->DidRunScriptCallback();
      }
    } else {
      *exception = pp::Var("Bad function call");
    }

    return pp::Var();
  }

  void clear_owner() { owner_ = nullptr; }

 private:
  TestFlashMessageLoop* owner_;
};

REGISTER_TEST_CASE(FlashMessageLoop);

TestFlashMessageLoop::TestFlashMessageLoop(TestingInstance* instance)
    : TestCase(instance),
      message_loop_(nullptr),
      instance_so_(nullptr),
      suspend_script_callback_result_(false),
      callback_factory_(this) {}

TestFlashMessageLoop::~TestFlashMessageLoop() {
  PP_DCHECK(!message_loop_);

  ResetTestObject();
  if (instance_so_)
    instance_so_->clear_owner();
}

void TestFlashMessageLoop::RunTests(const std::string& filter) {
  RUN_TEST(Basics, filter);
  RUN_TEST(RunWithoutQuit, filter);
  RUN_TEST(SuspendScriptCallbackWhileRunning, filter);
}

void TestFlashMessageLoop::DidRunScriptCallback() {
  // Script callbacks are not supposed to run while the Flash message loop is
  // running.
  if (message_loop_)
    suspend_script_callback_result_ = false;
}

pp::deprecated::ScriptableObject* TestFlashMessageLoop::CreateTestObject() {
  if (!instance_so_)
    instance_so_ = new InstanceSO(this);
  return instance_so_;
}

std::string TestFlashMessageLoop::TestBasics() {
  message_loop_ = new pp::flash::MessageLoop(instance_);

  pp::CompletionCallback callback = callback_factory_.NewCallback(
      &TestFlashMessageLoop::QuitMessageLoopTask);
  pp::Module::Get()->core()->CallOnMainThread(0, callback);
  int32_t result = message_loop_->Run();

  ASSERT_TRUE(message_loop_);
  delete message_loop_;
  message_loop_ = nullptr;

  ASSERT_EQ(PP_OK, result);
  PASS();
}

std::string TestFlashMessageLoop::TestRunWithoutQuit() {
  message_loop_ = new pp::flash::MessageLoop(instance_);

  pp::CompletionCallback callback = callback_factory_.NewCallback(
      &TestFlashMessageLoop::DestroyMessageLoopResourceTask);
  pp::Module::Get()->core()->CallOnMainThread(0, callback);
  int32_t result = message_loop_->Run();

  if (message_loop_) {
    delete message_loop_;
    message_loop_ = nullptr;
    ASSERT_TRUE(false);
  }

  ASSERT_EQ(PP_ERROR_ABORTED, result);
  PASS();
}

std::string TestFlashMessageLoop::TestSuspendScriptCallbackWhileRunning() {
  suspend_script_callback_result_ = true;
  message_loop_ = new pp::flash::MessageLoop(instance_);

  pp::CompletionCallback callback = callback_factory_.NewCallback(
      &TestFlashMessageLoop::TestSuspendScriptCallbackTask);
  pp::Module::Get()->core()->CallOnMainThread(0, callback);
  message_loop_->Run();

  ASSERT_TRUE(message_loop_);
  delete message_loop_;
  message_loop_ = nullptr;

  ASSERT_TRUE(suspend_script_callback_result_);
  PASS();
}

void TestFlashMessageLoop::TestSuspendScriptCallbackTask(int32_t unused) {
  pp::Var exception;
  pp::Var rev = instance_->ExecuteScript(
      "(function() {"
      "  function delayedHandler() {"
      "    document.getElementById('plugin').DidRunScriptCallback();"
      "  }"
      "  setTimeout(delayedHandler, 1);"
      "})()",
      &exception);
  if (!exception.is_undefined())
    suspend_script_callback_result_ = false;

  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&TestFlashMessageLoop::QuitMessageLoopTask);
  pp::Module::Get()->core()->CallOnMainThread(500, callback);
}

void TestFlashMessageLoop::QuitMessageLoopTask(int32_t unused) {
  if (message_loop_)
    message_loop_->Quit();
  else
    PP_NOTREACHED();
}

void TestFlashMessageLoop::DestroyMessageLoopResourceTask(int32_t unused) {
  if (message_loop_) {
    delete message_loop_;
    message_loop_ = nullptr;
  } else {
    PP_NOTREACHED();
  }
}
