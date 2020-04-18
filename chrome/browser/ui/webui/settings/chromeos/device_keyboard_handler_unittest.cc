// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/device_keyboard_handler.h"

#include <memory>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/test/test_web_ui.h"
#include "services/ui/public/cpp/input_devices/input_device_client_test_api.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/input_device.h"

namespace chromeos {
namespace settings {

namespace {

class TestKeyboardHandler : public KeyboardHandler {
 public:
  // Pull WebUIMessageHandler::set_web_ui() into public so tests can call it.
  using KeyboardHandler::set_web_ui;
};

}  // namespace

class KeyboardHandlerTest : public testing::Test {
 public:
  KeyboardHandlerTest() : handler_test_api_(&handler_) {
    handler_.set_web_ui(&web_ui_);
    handler_.RegisterMessages();
    handler_.AllowJavascriptForTesting();

    // Make sure that we start out without any keyboards reported.
    input_device_client_test_api_.SetKeyboardDevices({});
  }

 protected:
  // Updates out-params from the last message sent to WebUI about a change to
  // which keys should be shown. False is returned if the message was invalid or
  // not found.
  bool GetLastShowKeysChangedMessage(bool* has_caps_lock_out,
                                     bool* has_diamond_key_out)
      WARN_UNUSED_RESULT {
    for (auto it = web_ui_.call_data().rbegin();
         it != web_ui_.call_data().rend(); ++it) {
      const content::TestWebUI::CallData* data = it->get();
      std::string name;
      if (data->function_name() != "cr.webUIListenerCallback" ||
          !data->arg1()->GetAsString(&name) ||
          name != KeyboardHandler::kShowKeysChangedName) {
        continue;
      }
      return data->arg2()->GetAsBoolean(has_caps_lock_out) &&
             data->arg3()->GetAsBoolean(has_diamond_key_out);
    }
    return false;
  }

  // Returns true if the last keys-changed message reported that a Caps Lock key
  // is present and false otherwise. A failure is added if a message wasn't
  // found.
  bool HasCapsLock() {
    bool has_caps_lock = false, has_diamond_key = false;
    if (!GetLastShowKeysChangedMessage(&has_caps_lock, &has_diamond_key)) {
      ADD_FAILURE() << "Didn't get " << KeyboardHandler::kShowKeysChangedName;
      return false;
    }
    return has_caps_lock;
  }

  // Returns true if the last keys-changed message reported that a "diamond" key
  // is present and false otherwise. A failure is added if a message wasn't
  // found.
  bool HasDiamondKey() {
    bool has_caps_lock = false, has_diamond_key = false;
    if (!GetLastShowKeysChangedMessage(&has_caps_lock, &has_diamond_key)) {
      ADD_FAILURE() << "Didn't get " << KeyboardHandler::kShowKeysChangedName;
      return false;
    }
    return has_diamond_key;
  }

  ui::InputDeviceClientTestApi input_device_client_test_api_;
  content::TestWebUI web_ui_;
  TestKeyboardHandler handler_;
  KeyboardHandler::TestAPI handler_test_api_;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardHandlerTest);
};

TEST_F(KeyboardHandlerTest, DefaultKeys) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      chromeos::switches::kHasChromeOSKeyboard);
  handler_test_api_.Initialize();
  EXPECT_FALSE(HasCapsLock());
  EXPECT_FALSE(HasDiamondKey());
}

TEST_F(KeyboardHandlerTest, NonChromeOSKeyboard) {
  // If kHasChromeOSKeyboard isn't passed, we should assume there's a Caps Lock
  // key.
  handler_test_api_.Initialize();
  EXPECT_TRUE(HasCapsLock());
  EXPECT_FALSE(HasDiamondKey());
}

TEST_F(KeyboardHandlerTest, ExternalKeyboard) {
  // An internal keyboard shouldn't change the defaults.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      chromeos::switches::kHasChromeOSKeyboard);
  input_device_client_test_api_.SetKeyboardDevices(std::vector<ui::InputDevice>{
      {1, ui::INPUT_DEVICE_INTERNAL, "internal keyboard"}});
  handler_test_api_.Initialize();
  EXPECT_FALSE(HasCapsLock());
  EXPECT_FALSE(HasDiamondKey());

  // Simulate an external keyboard being connected. We should assume there's a
  // Caps Lock key now.
  input_device_client_test_api_.SetKeyboardDevices(std::vector<ui::InputDevice>{
      {2, ui::INPUT_DEVICE_EXTERNAL, "external keyboard"}});
  EXPECT_TRUE(HasCapsLock());
  EXPECT_FALSE(HasDiamondKey());

  // Disconnect the external keyboard and check that the key goes away.
  input_device_client_test_api_.SetKeyboardDevices({});
  EXPECT_FALSE(HasCapsLock());
  EXPECT_FALSE(HasDiamondKey());
}

TEST_F(KeyboardHandlerTest, DiamondKey) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      chromeos::switches::kHasChromeOSKeyboard);
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      chromeos::switches::kHasChromeOSDiamondKey);
  handler_test_api_.Initialize();
  EXPECT_FALSE(HasCapsLock());
  EXPECT_TRUE(HasDiamondKey());
}

}  // namespace settings
}  // namespace chromeos
