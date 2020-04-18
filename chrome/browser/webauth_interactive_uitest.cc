// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_service_manager_context.h"
#include "device/fido/scoped_virtual_fido_device.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {

class WebAuthFocusTest : public InProcessBrowserTest,
                         public PermissionRequestManager::Observer {
 protected:
  WebAuthFocusTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.ServeFilesFromSourceDirectory("content/test/data");
    ASSERT_TRUE(https_server_.Start());
  }

  GURL GetHttpsURL(const std::string& hostname,
                   const std::string& relative_url) {
    return https_server_.GetURL(hostname, relative_url);
  }

  // PermissionRequestManager::Observer implementation
  void OnBubbleAdded() override {
    // If this object is registered as a PermissionRequestManager observer then
    // it'll attempt to complete all permissions bubbles by sending keystrokes.
    // Note, however, that macOS rejects the permission bubble while other
    // platforms accept it, because there's no key sequence for accepting a
    // bubble on macOS.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](Browser* browser) {
              for (const auto& key : std::vector<ui::KeyboardCode> {
#if defined(OS_WIN) || defined(OS_CHROMEOS)
                     // Press tab (to select the "Allow" button of the
                     // permissions prompt) and then enter to activate it.
                     ui::KeyboardCode::VKEY_TAB, ui::KeyboardCode::VKEY_RETURN,
#elif defined(OS_MACOSX)
                       // There is no way to allow the bubble, we have to
                       // press escape to reject it.
                       ui::KeyboardCode::VKEY_ESCAPE,
#else
                       // Press tab twice (to select the "Allow" button of the
                       // permissions prompt) and then enter to activate it.
                       ui::KeyboardCode::VKEY_TAB,
                       ui::KeyboardCode::VKEY_TAB,
                       ui::KeyboardCode::VKEY_RETURN,
#endif
                   }) {
                ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
                    browser, key,
                    /*control=*/false, /*shift=*/false, /*alt=*/false,
                    /*command=*/false));
              }
            },
            browser()));
  }

 private:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  net::EmbeddedTestServer https_server_;

  DISALLOW_COPY_AND_ASSIGN(WebAuthFocusTest);
};

IN_PROC_BROWSER_TEST_F(WebAuthFocusTest, Focus) {
  // Web Authentication requests will often trigger machine-wide indications,
  // such as a Security Key flashing for a touch. If background tabs were able
  // to trigger this, there would be a risk of user confusion since the user
  // would not know which tab they would be interacting with if they touched a
  // Security Key. Because of that, some Web Authentication APIs require that
  // the frame be in the foreground in a focused window.

  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  ui_test_utils::NavigateToURL(browser(),
                               GetHttpsURL("www.example.com", "/title1.html"));

  device::test::ScopedVirtualFidoDevice virtual_device;

  constexpr char kRegisterTemplate[] =
      "navigator.credentials.create({publicKey: {"
      "  rp: {name: 't'},"
      "  user: {id: new Uint8Array([1]), name: 't', displayName: 't'},"
      "  challenge: new Uint8Array([1,2,3,4]),"
      "  timeout: 10000,"
      "  attestation: '$1',"
      "  pubKeyCredParams: [{type: 'public-key', alg: -7}]"
      "}}).then(c => window.domAutomationController.send('OK'),"
      "         e => window.domAutomationController.send(e.toString()));";
  const std::string register_script = base::ReplaceStringPlaceholders(
      kRegisterTemplate, std::vector<std::string>{"none"}, nullptr);

  content::WebContents* const initial_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  std::string result;
  // When operating in the foreground, the operation should succeed.
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_EQ(result, "OK");

  // Open a new tab to put the previous page in the background.
  chrome::NewTab(browser());

  // When in the background, the same request should result in a focus error.
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  constexpr char kFocusErrorSubstring[] = "the page does not have focus";
  EXPECT_THAT(result, ::testing::HasSubstr(kFocusErrorSubstring));

  // Close the tab and the action should succeed again.
  chrome::CloseTab(browser());
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_EQ(result, "OK");

  // Start the request in the foreground and open a new tab between starting and
  // finishing the request. This should fail because we don't want foreground
  // pages to be able to start a request, open a trusted site in a new
  // tab/window, and have the user believe that they are interacting with that
  // trusted site.
  virtual_device.mutable_state()->simulate_press_callback = base::BindRepeating(
      [](Browser* browser) { chrome::NewTab(browser); }, browser());
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_THAT(result, ::testing::HasSubstr(kFocusErrorSubstring));

  // Close the tab and the action should succeed again.
  chrome::CloseTab(browser());
  virtual_device.mutable_state()->simulate_press_callback.Reset();
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_EQ(result, "OK");

  // Open dev tools and check that operations still succeed.
  DevToolsWindow* dev_tools_window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(
          initial_web_contents, true /* docked, not a separate window */);
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_EQ(result, "OK");
  DevToolsWindowTesting::CloseDevToolsWindowSync(dev_tools_window);

  // Open a second browser window.
  ui_test_utils::BrowserAddedObserver browser_added_observer;
  chrome::NewWindow(browser());
  Browser* new_window = browser_added_observer.WaitForSingleNewBrowser();
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(new_window));

  // Operations in the (now unfocused) window should fail, even though it's
  // still the active tab in that window.
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_THAT(result, ::testing::HasSubstr(kFocusErrorSubstring));

  // Check that closing the window brings things back to a focused state.
  chrome::CloseWindow(new_window);
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(initial_web_contents,
                                                     register_script, &result));
  EXPECT_EQ(result, "OK");

  // Requesting "direct" attestation will trigger a permissions prompt.
  const std::string get_assertion_with_attestation_script =
      base::ReplaceStringPlaceholders(
          kRegisterTemplate, std::vector<std::string>{"direct"}, nullptr);

  PermissionRequestManager* const permission_request_manager =
      PermissionRequestManager::FromWebContents(initial_web_contents);
  // The observer callback will trigger the permissions prompt.
  permission_request_manager->AddObserver(this);
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      initial_web_contents, get_assertion_with_attestation_script, &result));
#if defined(OS_MACOSX)
  // The permissions bubble has to be rejected on macOS because there's no key
  // sequence to accept it. Therefore a NotAllowedError is expected. This is not
  // ideal as a timeout causes the same result, but it is distinct from a focus
  // error.
  EXPECT_THAT(result, ::testing::HasSubstr("NotAllowedError: "));
#else
  EXPECT_EQ(result, "OK");
#endif
  permission_request_manager->RemoveObserver(this);
}

}  // anonymous namespace
