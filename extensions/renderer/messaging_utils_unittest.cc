// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#include "extensions/grit/extensions_renderer_resources.h"
#include "extensions/renderer/module_system_test.h"

namespace extensions {
namespace {

class MessagingUtilsUnittest : public ModuleSystemTest {
 protected:
  void RegisterTestModule(const char* code) {
    env()->RegisterModule(
        "test",
        base::StringPrintf(
            "var assert = requireNative('assert');\n"
            "var AssertTrue = assert.AssertTrue;\n"
            "var AssertFalse = assert.AssertFalse;\n"
            "var messagingUtils = require('messaging_utils');\n"
            "%s",
            code));
  }

 private:
  void SetUp() override {
    ModuleSystemTest::SetUp();

    env()->RegisterModule("messaging_utils", IDR_MESSAGING_UTILS_JS);
  }
};

TEST_F(MessagingUtilsUnittest, TestNothing) {
  ExpectNoAssertionsMade();
}

TEST_F(MessagingUtilsUnittest, NoArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments();\n"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, ZeroArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments([]);"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, TooManyArgumentsNoOptions) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c', 'd']);\n"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, TooManyArgumentsWithOptions) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c', 'd', 'e'], true);\n"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, FinalArgumentIsNotAFunctionNoOptions) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c']);\n"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, FinalArgumentIsNotAFunctionWithOptions) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c', 'd'], true);\n"
      "AssertTrue(args === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  // Because the request argument is required, a single argument must get
  // mapped to it rather than to the optional targetId argument.
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(['a']);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringAndOneNullArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  // Explicitly specifying null as the request is allowed.
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(['a', null]);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] == 'a');\n"
      "AssertTrue(args[1] === null);\n"
      "AssertTrue(args[2] === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneNullAndOneStringArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments([null, 'a']);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringAndOneFunctionArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  // When the arguments are a string and a function, the function is
  // unambiguously the responseCallback. Because the request argument is
  // required, the remaining argument must get mapped to it rather than to the
  // optional targetId argument.
  RegisterTestModule(
      "var cb = function() {};\n"
      "var args = messagingUtils.alignSendMessageArguments(['a', cb]);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] == cb);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringAndOneObjectArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  // This tests an ambiguous set of arguments when options are present:
  // chrome.runtime.sendMessage('target', {'msg': 'this is a message'});
  // vs.
  // chrome.runtime.sendMessage('request', {'includeTlsChannelId': true});
  //
  // The question is whether the string should map to the target and the
  // dictionary to the message, or whether the string should map to the message
  // and the dictionary to the options. Because the target and message arguments
  // predate the options argument, we bind the string in this case to the
  // targetId.
  RegisterTestModule(
      "var obj = {'b': true};\n"
      "var args = messagingUtils.alignSendMessageArguments(['a', obj], true);\n"
      "AssertTrue(args.length == 4);\n"
      "AssertTrue(args[0] == 'a');\n"
      "AssertTrue(args[1] == obj);\n"
      "AssertTrue(args[2] === null);\n"
      "AssertTrue(args[3] === null);");
  env()->module_system()->Require("test");
}

TEST_F(MessagingUtilsUnittest, TwoObjectArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  // When two non-string arguments are provided and options are present, the
  // two arguments must match request and options, respectively, because
  // targetId must be a string.
  RegisterTestModule(
      "var obj1 = {'a': 'foo'};\n"
      "var obj2 = {'b': 'bar'};\n"
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    [obj1, obj2], true);\n"
      "AssertTrue(args.length == 4);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == obj1);\n"
      "AssertTrue(args[2] == obj2);\n"
      "AssertTrue(args[3] === null);");
  env()->module_system()->Require("test");
}

}  // namespace
}  // namespace extensions
