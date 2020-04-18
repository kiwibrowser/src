// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

namespace {

// The html file to receive key events, prevent defaults and export all the
// events with "getKeyEventReport()" function. It has two magic keys: pressing
// "S" to enter fullscreen mode; pressing "X" to indicate the end of all the
// keys (see FinishTestAndVerifyResult() function).
constexpr char kFullscreenKeyboardLockHTML[] =
    "/fullscreen_keyboardlock/fullscreen_keyboardlock.html";

// On MacOSX command key is used for most of the shortcuts, so replace it with
// control to reduce the complexity of comparison of the results.
void NormalizeMetaKeyForMacOS(std::string* output) {
#if defined(OS_MACOSX)
  base::ReplaceSubstringsAfterOffset(output, 0, "MetaLeft", "ControlLeft");
#endif
}

}  // namespace

class BrowserCommandControllerInteractiveTest : public InProcessBrowserTest {
 public:
  BrowserCommandControllerInteractiveTest() = default;
  ~BrowserCommandControllerInteractiveTest() override = default;

 protected:
  // Starts |kFullscreenKeyboardLockHTML| in a new tab and waits for load.
  void StartFullscreenLockPage();

  // Sends a control or command + |key| shortcut to the focused window. Shift
  // modifier will be added if |shift| is true.
  void SendShortcut(ui::KeyboardCode key, bool shift = false);

  // Sends a control or command + shift + |key| shortcut to the focused window.
  void SendShiftShortcut(ui::KeyboardCode key);

  // Sends a fullscreen shortcut to the focused window and wait for the
  // operation to take effect.
  void SendFullscreenShortcutAndWait();

  // Sends a KeyS to the focused window to trigger JavaScript fullscreen and
  // wait for the operation to take effect.
  void SendJsFullscreenShortcutAndWait();

  // Sends an ESC to the focused window.
  void SendEscape();

  // Sends an ESC to the focused window to exit JavaScript fullscreen and wait
  // for the operation to take effect.
  void SendEscapeAndWaitForExitingFullscreen();

  // Sends a set of preventable shortcuts to the web page and expects them to be
  // prevented.
  void SendShortcutsAndExpectPrevented();

  // Sends a set of preventable shortcuts to the web page and expects them to
  // not be prevented. If |js_fullscreen| is true, the test will use
  // SendJsFullscreenShortcutAndWait() to trigger the fullscreen mode. Otherwise
  // SendFullscreenShortcutAndWait() will be used.
  void SendShortcutsAndExpectNotPrevented(bool js_fullscreen);

  // Sends a magic KeyX to the focused window to stop the test case, receives
  // the result and verifies if it is equal to |expected_result_|.
  void FinishTestAndVerifyResult();

  // Returns whether the active tab is in html fullscreen mode.
  bool IsActiveTabFullscreen() const {
    auto* contents = GetActiveWebContents();
    return contents->GetDelegate()->IsFullscreenForTabOrPending(contents);
  }

  // Returns whether the GetActiveBrowser() is in browser fullscreen mode.
  bool IsInBrowserFullscreen() const {
    return GetActiveBrowser()
               ->exclusive_access_manager()
               ->fullscreen_controller()
               ->IsFullscreenForBrowser();
  }

  content::WebContents* GetActiveWebContents() const {
    return GetActiveBrowser()->tab_strip_model()->GetActiveWebContents();
  }

  // Gets the current active tab index.
  int GetActiveTabIndex() const {
    return GetActiveBrowser()->tab_strip_model()->active_index();
  }

  // Gets the count of tabs in current browser.
  int GetTabCount() const {
    return GetActiveBrowser()->tab_strip_model()->count();
  }

  // Gets the count of browser instances.
  size_t GetBrowserCount() const {
    return BrowserList::GetInstance()->size();
  }

  // Gets the last active Browser instance.
  Browser* GetActiveBrowser() const {
    return BrowserList::GetInstance()->GetLastActive();
  }

  // Ensures GetActiveBrowser() is focused.
  void FocusOnLastActiveBrowser() {
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(
        GetActiveBrowser()));
  }

  // Waits until the count of Browser instances becomes |expected|.
  void WaitForBrowserCount(size_t expected) {
    while (GetBrowserCount() != expected)
      content::RunAllPendingInMessageLoop();
  }

  // Waits until the count of the tabs in active Browser instance becomes
  // |expected|.
  void WaitForTabCount(int expected) {
    while (GetTabCount() != expected)
      content::RunAllPendingInMessageLoop();
  }

  // Waits until the index of active tab in active Browser instance becomes
  // |expected|.
  void WaitForActiveTabIndex(int expected) {
    while (GetActiveTabIndex() != expected)
      content::RunAllPendingInMessageLoop();
  }

  // Waits until the index of active tab in active Browser instance is not
  // |expected|.
  void WaitForInactiveTabIndex(int expected) {
    while (GetActiveTabIndex() == expected)
      content::RunAllPendingInMessageLoop();
  }

 private:
  void SetUpOnMainThread() override;

  // The expected output from the web page. This string is generated by
  // appending key presses from Send* functions above.
  std::string expected_result_;

  DISALLOW_COPY_AND_ASSIGN(BrowserCommandControllerInteractiveTest);
};

void BrowserCommandControllerInteractiveTest::StartFullscreenLockPage() {
  // Ensures the initial states.
  ASSERT_EQ(1, GetTabCount());
  ASSERT_EQ(0, GetActiveTabIndex());
  ASSERT_EQ(1U, GetBrowserCount());
  // Add a second tab for counting and focus purposes.
  AddTabAtIndex(1, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK);
  ASSERT_EQ(2, GetTabCount());
  ASSERT_EQ(1U, GetBrowserCount());

  if (!embedded_test_server()->Started())
    ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURLWithDisposition(
      GetActiveBrowser(),
      embedded_test_server()->GetURL(kFullscreenKeyboardLockHTML),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
}

void BrowserCommandControllerInteractiveTest::SendShortcut(
    ui::KeyboardCode key,
    bool shift /* = false */) {
#if defined(OS_MACOSX)
  const bool control_modifier = false;
  const bool command_modifier = true;
#else
  const bool control_modifier = true;
  const bool command_modifier = false;
#endif
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), key, control_modifier, shift, false,
      command_modifier));

  expected_result_ += ui::KeycodeConverter::DomCodeToCodeString(
      ui::UsLayoutKeyboardCodeToDomCode(key));
  expected_result_ += " ctrl:";
  expected_result_ += control_modifier ? "true" : "false";
  expected_result_ += " shift:";
  expected_result_ += shift ? "true" : "false";
  expected_result_ += " alt:false";
  expected_result_ += " meta:";
  expected_result_ += command_modifier ? "true" : "false";
  expected_result_ += '\n';
}

void BrowserCommandControllerInteractiveTest::SendShiftShortcut(
    ui::KeyboardCode key) {
  ASSERT_NO_FATAL_FAILURE(SendShortcut(key, true));
}

void BrowserCommandControllerInteractiveTest::SendFullscreenShortcutAndWait() {
  // On MacOSX, entering and exiting fullscreen are not synchronous. So we wait
  // for the observer to notice the change of fullscreen state.
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  // Enter fullscreen.
#if defined(OS_MACOSX)
  // On MACOSX, Command + Control + F is used.
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_F, true, false, false, true));
#elif defined(OS_CHROMEOS)
  // A dedicated fullscreen key is used on Chrome OS, so send a fullscreen
  // command directly instead, to avoid constructing the key press.
  ASSERT_TRUE(chrome::ExecuteCommand(GetActiveBrowser(), IDC_FULLSCREEN));
#else
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_F11, false, false, false, false));
#endif

// Mac fullscreen is simulated in tests and is performed synchronously with the
// keyboard events. As a result, content doesn't actually know it has entered
// fullscreen. For more details, see ScopedFakeNSWindowFullscreen.
// TODO(crbug.com/837438): Remove this once ScopedFakeNSWindowFullscreen fires
// NOTIFICATION_FULLSCREEN_CHANGED.
#if !defined(OS_MACOSX)
  observer.Wait();
#endif
}

void BrowserCommandControllerInteractiveTest::
    SendJsFullscreenShortcutAndWait() {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_S, false, false, false, false));
  expected_result_ += "KeyS ctrl:false shift:false alt:false meta:false\n";
  observer.Wait();
  ASSERT_TRUE(IsActiveTabFullscreen());
}

void BrowserCommandControllerInteractiveTest::SendEscape() {
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_ESCAPE, false, false, false, false));
  expected_result_ += "Escape ctrl:false shift:false alt:false meta:false\n";
}

void BrowserCommandControllerInteractiveTest ::
    SendEscapeAndWaitForExitingFullscreen() {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_ESCAPE, false, false, false, false));
  observer.Wait();
  ASSERT_FALSE(IsActiveTabFullscreen());
}

void BrowserCommandControllerInteractiveTest::
    SendShortcutsAndExpectPrevented() {
  const int initial_active_index = GetActiveTabIndex();
  const int initial_tab_count = GetTabCount();
  const size_t initial_browser_count = GetBrowserCount();
  // The tab should not be closed.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(initial_tab_count, GetTabCount());
  // The window should not be closed.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_W));
  ASSERT_EQ(initial_browser_count, GetBrowserCount());
  // A new tab should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(initial_tab_count, GetTabCount());
  // A new window should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_N));
  ASSERT_EQ(initial_browser_count, GetBrowserCount());
  // A new incognito window should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_N));
  ASSERT_EQ(initial_browser_count, GetBrowserCount());
  // Last closed tab should not be restored.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_T));
  ASSERT_EQ(initial_tab_count, GetTabCount());
  // Browser should not switch to the next tab.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_TAB));
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());
  // Browser should not switch to the previous tab.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_TAB));
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());
}

void BrowserCommandControllerInteractiveTest::
    SendShortcutsAndExpectNotPrevented(bool js_fullscreen) {
  const int initial_active_index = GetActiveTabIndex();
  const int initial_tab_count = GetTabCount();
  const size_t initial_browser_count = GetBrowserCount();
  const auto enter_fullscreen = [this, js_fullscreen]() {
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(
        this->GetActiveBrowser()));
    if (js_fullscreen) {
      if (!this->IsActiveTabFullscreen()) {
        static const std::string page =
            "<html><head></head><body></body><script>"
            "document.addEventListener('keydown', "
            "    (e) => {"
            "      if (e.code == 'KeyS') { "
            "        document.body.webkitRequestFullscreen();"
            "      }"
            "    });"
            "</script></html>";
        ui_test_utils::NavigateToURLWithDisposition(
            this->GetActiveBrowser(),
            GURL("data:text/html," + page),
            WindowOpenDisposition::CURRENT_TAB,
            ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
        ASSERT_NO_FATAL_FAILURE(this->SendJsFullscreenShortcutAndWait());
      }
    } else {
      if (!this->IsInBrowserFullscreen()) {
        ASSERT_NO_FATAL_FAILURE(this->SendFullscreenShortcutAndWait());
      }
      ASSERT_TRUE(this->IsInBrowserFullscreen());
    }
  };

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new tab should be created and focused.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  WaitForTabCount(initial_tab_count + 1);
  ASSERT_NE(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be closed.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  WaitForTabCount(initial_tab_count);
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new tab should be created and focused.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  WaitForTabCount(initial_tab_count + 1);
  ASSERT_NE(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The previous tab should be focused.
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_TAB, true, true, false, false));
  WaitForActiveTabIndex(initial_active_index);
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be focused.
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_TAB, true, false, false, false));
  WaitForInactiveTabIndex(initial_active_index);
  ASSERT_NE(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be closed.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  WaitForTabCount(initial_tab_count);
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new window should be created and focused.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_N));
  WaitForBrowserCount(initial_browser_count + 1);
  ASSERT_EQ(initial_browser_count + 1, GetBrowserCount());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created window should be closed.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_W));
  WaitForBrowserCount(initial_browser_count);

  ASSERT_EQ(initial_browser_count, GetBrowserCount());
  ASSERT_EQ(initial_active_index, GetActiveTabIndex());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());
}

void BrowserCommandControllerInteractiveTest::FinishTestAndVerifyResult() {
  // The renderer process receives key events through IPC channel,
  // SendKeyPressSync() cannot guarantee the JS has processed the key event it
  // sent. So we sent a KeyX to the webpage to indicate the end of the test
  // case. After processing this key event, web page is safe to send the record
  // back through window.domAutomationController.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_X, false, false, false, false));
  expected_result_ += "KeyX ctrl:false shift:false alt:false meta:false";
  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      GetActiveWebContents()->GetRenderViewHost(),
      "getKeyEventReport();", &result));
  NormalizeMetaKeyForMacOS(&result);
  NormalizeMetaKeyForMacOS(&expected_result_);
  base::TrimWhitespaceASCII(result, base::TRIM_ALL, &result);
  ASSERT_EQ(expected_result_, result);
}

void BrowserCommandControllerInteractiveTest::SetUpOnMainThread() {
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(GetActiveBrowser()));
}

IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       ShortcutsShouldTakeEffectInWindowMode) {
  ASSERT_EQ(1, GetTabCount());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(2, GetTabCount());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(3, GetTabCount());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(2, GetTabCount());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(1, GetTabCount());
  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_TRUE(IsInBrowserFullscreen());
  ASSERT_FALSE(IsActiveTabFullscreen());
}

IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       UnpreservedShortcutsShouldBePreventable) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  // The browser print function should be blocked by the web page.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_P));
  // The system print function should be blocked by the web page.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_P));
  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

#if defined(OS_MACOSX)
// TODO(zijiehe): Figure out why this test crashes on Mac OSX. The suspicious
// command is "SendFullscreenShortcutAndWait()". See, http://crbug.com/738949.
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen \
  DISABLED_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen
#else
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen \
  KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen
#endif
IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_TRUE(IsInBrowserFullscreen());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
  // Current page should not exit browser fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendEscape());

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());

  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_FALSE(IsInBrowserFullscreen());
}

IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForEsc) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendJsFullscreenShortcutAndWait());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
  // Current page should exit HTML fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendEscapeAndWaitForExitingFullscreen());

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

#if defined(OS_MACOSX)
// Triggers a DCHECK in MacViews: http://crbug.com/823478
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11 \
    DISABLED_KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11
#else
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11 \
    KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11
#endif
IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    MAYBE_KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendJsFullscreenShortcutAndWait());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
#if defined(OS_MACOSX)
  // On 10.9 or earlier, sending the exit fullscreen shortcut will crash the
  // binary. See http://crbug.com/740250.
  if (base::mac::IsAtLeastOS10_10()) {
    // Current page should exit browser fullscreen mode.
    ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
    ASSERT_FALSE(IsActiveTabFullscreen());
    ASSERT_FALSE(IsInBrowserFullscreen());
  }
#else
  // Current page should exit browser fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_FALSE(IsInBrowserFullscreen());
#endif

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

#if defined(OS_MACOSX)
// TODO(zijiehe): Figure out why this test crashes on Mac OSX. The suspicious
// command is "SendFullscreenShortcutAndWait()". See, http://crbug.com/738949.
#define MAYBE_ShortcutsShouldTakeEffectInBrowserFullscreen \
        DISABLED_ShortcutsShouldTakeEffectInBrowserFullscreen
#else
#define MAYBE_ShortcutsShouldTakeEffectInBrowserFullscreen \
        ShortcutsShouldTakeEffectInBrowserFullscreen
#endif
IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       MAYBE_ShortcutsShouldTakeEffectInBrowserFullscreen) {
#if defined(OS_MACOSX)
  // On 10.9 or earlier, sending the exit fullscreen shortcut will crash the
  // binary. See http://crbug.com/740250.
  if (base::mac::IsAtMostOS10_9())
    return;
#endif
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectNotPrevented(false));
}

#if !defined(OS_MACOSX)
// HTML fullscreen is automatically exited after some commands are executed,
// such as Ctrl + T (new tab). But some commands won't have this effect, such as
// Ctrl + N (new window).
// On Mac OSX, AppKit implementation is used for HTML fullscreen mode. Entering
// and exiting AppKit fullscreen mode triggers an animation. A
// FullscreenChangeObserver is needed to ensure the animation is finished. But
// the FullscreenChangeObserver won't finish if the command actually won't cause
// the page to exit fullscreen mode. So we need to maintain a list of exiting /
// non-exiting commands, which is not the goal of this test.

#if defined(OS_CHROMEOS)
// This test is flaky on ChromeOS, see http://crbug.com/754878.
#define MAYBE_ShortcutsShouldTakeEffectInJsFullscreen \
        DISABLED_ShortcutsShouldTakeEffectInJsFullscreen
#else
#define MAYBE_ShortcutsShouldTakeEffectInJsFullscreen \
        ShortcutsShouldTakeEffectInJsFullscreen
#endif
IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       MAYBE_ShortcutsShouldTakeEffectInJsFullscreen) {
// This test is flaky. See http://crbug.com/759704.
// TODO(zijiehe): Find out the root cause.
#if defined(OS_LINUX)
  return;
#endif
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectNotPrevented(true));
}

#endif
