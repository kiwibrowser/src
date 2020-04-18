// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/login_ui_test_utils.h"

#include "base/run_loop.h"
#include "base/scoped_observer.h"
#include "base/test/bind_test_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/signin/signin_tracker_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/signin_view_controller_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/browser/ui/webui/signin/signin_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

using content::MessageLoopRunner;

// anonymous namespace for signin with UI helper functions.
namespace {

// When Desktop Identity Consistency (Dice) is enabled, the password field is
// not easily accessible on the Gaia page. This script can be used to return it.
const char kGetPasswordFieldFromDiceSigninPage[] =
    "(function() {"
    "  var e = document.getElementById('password');"
    "  if (e == null) return null;"
    "  return e.querySelector('input[type=password]');"
    "})()";

// The SignInObserver observes the signin manager and blocks until a
// GoogleSigninSucceeded or a GoogleSigninFailed notification is fired.
class SignInObserver : public SigninTracker::Observer {
 public:
  SignInObserver() : seen_(false), running_(false), signed_in_(false) {}

  virtual ~SignInObserver() {}

  // Returns whether a GoogleSigninSucceeded event has happened.
  bool DidSignIn() {
    return signed_in_;
  }

  // Blocks and waits until the user signs in. Wait() does not block if a
  // GoogleSigninSucceeded or a GoogleSigninFailed has already occurred.
  void Wait() {
    if (seen_)
      return;

    running_ = true;
    message_loop_runner_ = new MessageLoopRunner;
    message_loop_runner_->Run();
    EXPECT_TRUE(seen_);
  }

  void SigninFailed(const GoogleServiceAuthError& error) override {
    DVLOG(1) << "Google signin failed.";
    QuitLoopRunner();
  }

  void AccountAddedToCookie(const GoogleServiceAuthError& error) override {}

  void SigninSuccess() override {
    DVLOG(1) << "Google signin succeeded.";
    signed_in_ = true;
    QuitLoopRunner();
  }

  void QuitLoopRunner() {
    seen_ = true;
    if (!running_)
      return;
    message_loop_runner_->Quit();
    running_ = false;
  }

 private:
  // Bool to mark an observed event as seen prior to calling Wait(), used to
  // prevent the observer from blocking.
  bool seen_;
  // True is the message loop runner is running.
  bool running_;
  // True if a GoogleSigninSucceeded event has been observed.
  bool signed_in_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;
};

// Synchronously waits for the Sync confirmation to be closed.
class SyncConfirmationClosedObserver : public LoginUIService::Observer {
 public:
  void WaitForConfirmationClosed() {
    if (sync_confirmation_closed_)
      return;

    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  void OnSyncConfirmationUIClosed(
      LoginUIService::SyncConfirmationUIClosedResult result) override {
    sync_confirmation_closed_ = true;
    if (quit_closure_)
      std::move(quit_closure_).Run();
  }

  bool sync_confirmation_closed_ = false;
  base::OnceClosure quit_closure_;
};

void RunLoopFor(base::TimeDelta duration) {
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), duration);
  run_loop.Run();
}

// Returns true if the Dice signin page is used, and false if the embedded
// signin flow is used.
// The Dice signin page is shown in a full tab, whereas the embedded signin flow
// runs inside a WebUI.
bool IsDiceSigninPageEnabled(Profile* profile) {
  signin::AccountConsistencyMethod account_consistency =
      AccountConsistencyModeManager::GetMethodForProfile(profile);
  return (account_consistency != signin::AccountConsistencyMethod::kMirror) &&
         signin::DiceMethodGreaterOrEqual(
             account_consistency,
             signin::AccountConsistencyMethod::kDicePrepareMigration);
}

// Returns the render frame host where Gaia credentials can be filled in.
content::RenderFrameHost* GetSigninFrame(content::WebContents* web_contents) {
  if (IsDiceSigninPageEnabled(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
    // Dice displays the Gaia page directly in a tab.
    return web_contents->GetMainFrame();
  } else {
    // Embedded signin flow, uses a sub-frame in WebUI.
    return signin::GetAuthFrame(web_contents, "signin-frame");
  }
}

// Waits until the condition is met, by polling.
void WaitUntilCondition(const base::RepeatingCallback<bool()>& condition,
                        const std::string& error_message) {
  for (int attempt = 0; attempt < 10; ++attempt) {
    if (condition.Run())
      return;
    RunLoopFor(base::TimeDelta::FromMilliseconds(1000));
  }

  FAIL() << error_message;
}

// Evaluates a boolean script expression in the signin frame.
bool EvaluateBooleanScriptInSigninFrame(Browser* browser,
                                        const std::string& script) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  bool result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetSigninFrame(web_contents),
      "window.domAutomationController.send(" + script + ");", &result));
  return result;
}

// Returns whether an element with id |element_id| exists in the signin page.
bool ElementExistsByIdInSigninFrame(Browser* browser,
                                    const std::string& element_id) {
  return EvaluateBooleanScriptInSigninFrame(
      browser, "document.getElementById('" + element_id + "') != null");
}

// Blocks until an element with an id from |element_ids| exists in the signin
// page.
void WaitUntilAnyElementExistsInSigninFrame(
    Browser* browser,
    const std::vector<std::string>& element_ids) {
  WaitUntilCondition(
      base::BindLambdaForTesting([&browser, &element_ids]() -> bool {
        for (const std::string& element_id : element_ids) {
          if (ElementExistsByIdInSigninFrame(browser, element_id))
            return true;
        }
        return false;
      }),
      "Could not find elements in the signin frame");
}

}  // namespace

namespace login_ui_test_utils {
class SigninViewControllerTestUtil {
 public:
  static bool TryDismissSyncConfirmationDialog(Browser* browser) {
#if defined(OS_CHROMEOS)
    NOTREACHED();
    return false;
#else
    SigninViewController* signin_view_controller =
        browser->signin_view_controller();
    DCHECK_NE(signin_view_controller, nullptr);
    if (!signin_view_controller->ShowsModalDialog())
      return false;
    content::WebContents* dialog_web_contents =
        signin_view_controller->GetModalDialogWebContentsForTesting();
    DCHECK_NE(dialog_web_contents, nullptr);
    std::string message;
    std::string find_button_js =
        "if (document.readyState != 'complete') {"
        "  window.domAutomationController.send('DocumentNotReady');"
        "} else if (document.getElementById('confirmButton') == null) {"
        "  window.domAutomationController.send('NotFound');"
        "} else {"
        "  window.domAutomationController.send('Ok');"
        "}";
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        dialog_web_contents, find_button_js, &message));
    if (message != "Ok")
      return false;

    // This cannot be a synchronous call, because it closes the window as a side
    // effect, which may cause the javascript execution to never finish.
    content::ExecuteScriptAsync(
        dialog_web_contents,
        "document.getElementById('confirmButton').click();");
    return true;
#endif
  }
};

void WaitUntilUIReady(Browser* browser) {
  std::string message;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      browser->tab_strip_model()->GetActiveWebContents(),
      "if (!inline.login.getAuthExtHost())"
      "  inline.login.initialize();"
      "var handler = function() {"
      "  window.domAutomationController.send('ready');"
      "};"
      "if (inline.login.isAuthReady())"
      "  handler();"
      "else"
      "  inline.login.getAuthExtHost().addEventListener('ready', handler);",
      &message));
  ASSERT_EQ("ready", message);
}

void SigninInNewGaiaFlow(Browser* browser,
                         const std::string& email,
                         const std::string& password) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  WaitUntilAnyElementExistsInSigninFrame(browser, {"identifierId"});
  std::string js = "document.getElementById('identifierId').value = '" + email +
                   "'; document.getElementById('identifierNext').click();";
  ASSERT_TRUE(content::ExecuteScript(GetSigninFrame(web_contents), js));

  // Fill the password input field.
  if (IsDiceSigninPageEnabled(browser->profile())) {
    std::string password_script = kGetPasswordFieldFromDiceSigninPage;
    // Wait until the password field exists.
    WaitUntilCondition(
        base::BindLambdaForTesting([&browser, &password_script]() -> bool {
          return EvaluateBooleanScriptInSigninFrame(
              browser, password_script + " != null");
        }),
        "Could not find Dice password field");
    js = password_script + ".value = '" + password + "';";
  } else {
    WaitUntilAnyElementExistsInSigninFrame(browser, {"password"});
    js = "document.getElementById('password').value = '" + password + "';";
  }

  js += "document.getElementById('passwordNext').click();";
  ASSERT_TRUE(content::ExecuteScript(GetSigninFrame(web_contents), js));
}

void SigninInOldGaiaFlow(Browser* browser,
                         const std::string& email,
                         const std::string& password) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  WaitUntilAnyElementExistsInSigninFrame(browser, {"Email"});
  std::string js = "document.getElementById('Email').value = '" + email + ";" +
                   "document.getElementById('next').click();";
  ASSERT_TRUE(content::ExecuteScript(GetSigninFrame(web_contents), js));

  WaitUntilAnyElementExistsInSigninFrame(browser, {"Passwd"});
  js = "document.getElementById('Passwd').value = '" + password + "';" +
       "document.getElementById('signIn').click();";
  ASSERT_TRUE(content::ExecuteScript(GetSigninFrame(web_contents), js));
}

void ExecuteJsToSigninInSigninFrame(Browser* browser,
                                    const std::string& email,
                                    const std::string& password) {
  WaitUntilAnyElementExistsInSigninFrame(browser, {"identifierNext", "next"});
  if (ElementExistsByIdInSigninFrame(browser, "identifierNext"))
    SigninInNewGaiaFlow(browser, email, password);
  else
    SigninInOldGaiaFlow(browser, email, password);
}

bool SignInWithUI(Browser* browser,
                  const std::string& username,
                  const std::string& password) {
  SignInObserver signin_observer;
  std::unique_ptr<SigninTracker> tracker =
      SigninTrackerFactory::CreateForProfile(browser->profile(),
                                             &signin_observer);
  signin_metrics::AccessPoint access_point =
      signin_metrics::AccessPoint::ACCESS_POINT_MENU;

  if (IsDiceSigninPageEnabled(browser->profile())) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    chrome::ShowBrowserSignin(browser, access_point);
    content::WebContents* active_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    DCHECK(active_contents);
    content::TestNavigationObserver observer(
        active_contents, 1, content::MessageLoopRunner::QuitMode::DEFERRED);
    observer.Wait();
#else
    NOTREACHED();
#endif
  } else {
    GURL signin_url = signin::GetPromoURLForTab(
        access_point, signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
        false);
    DVLOG(1) << "Navigating to " << signin_url;
    // For some tests, the window is not shown yet and this might be the first
    // tab navigation, so GetActiveWebContents() for CURRENT_TAB is NULL. That's
    // why we use NEW_FOREGROUND_TAB rather than the CURRENT_TAB used by default
    // in ui_test_utils::NavigateToURL().
    ui_test_utils::NavigateToURLWithDisposition(
        browser, signin_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    // Wait for the WebUI embedding the signin flow to be ready.
    DVLOG(1) << "Wait for login UI to be ready.";
    WaitUntilUIReady(browser);
  }

  DVLOG(1) << "Sign in user: " << username;
  ExecuteJsToSigninInSigninFrame(browser, username, password);
  signin_observer.Wait();
  return signin_observer.DidSignIn();
}

bool DismissSyncConfirmationDialog(Browser* browser, base::TimeDelta timeout) {
  SyncConfirmationClosedObserver confirmation_closed_observer;
  ScopedObserver<LoginUIService, LoginUIService::Observer>
      scoped_confirmation_closed_observer(&confirmation_closed_observer);
  scoped_confirmation_closed_observer.Add(
      LoginUIServiceFactory::GetForProfile(browser->profile()));

  const base::Time expire_time = base::Time::Now() + timeout;
  while (base::Time::Now() <= expire_time) {
    if (SigninViewControllerTestUtil::TryDismissSyncConfirmationDialog(
            browser)) {
      confirmation_closed_observer.WaitForConfirmationClosed();
      return true;
    }
    RunLoopFor(base::TimeDelta::FromMilliseconds(1000));
  }
  return false;
}

}  // namespace login_ui_test_utils
