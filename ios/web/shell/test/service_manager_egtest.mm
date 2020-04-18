// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/run_loop.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/service_manager_connection.h"
#import "ios/web/shell/test/app/web_shell_test_util.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"
#import "ios/web/shell/web_usage_controller.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/test/echo/public/mojom/echo.mojom.h"
#include "services/test/user_id/public/mojom/user_id.mojom.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char* kTestInput = "Livingston, I presume";

// Callback passed to echo::mojom::Echo::EchoString(). Verifies that the echoed
// string has the expected value and sets |echo_callback_called_flag| to true
// to indicate that the callback was invoked. |echo| is passed simply to ensure
// that our connection to the Echo implementation remains alive long enough for
// the callback to reach us.
void OnEchoString(echo::mojom::EchoPtr echo,
                  bool* echo_callback_called_flag,
                  const std::string& echoed_input) {
  GREYAssert(kTestInput == echoed_input,
             @"Unexpected string passed to echo callback: %s",
             echoed_input.c_str());
  *echo_callback_called_flag = true;
}

// Callback passed to web::mojom::WebUsageController::SetWebUsageEnabled().
// Sets |web_usage_controller_callback_called_flag| to true to indicate that
// the callback was invoked. |web_usage_controller_id| is passed simply to
// ensure that our connection to the implementation remains alive long enough
// for the callback to reach us.
void OnWebUsageSet(web::mojom::WebUsageControllerPtr web_usage_controller,
                   bool* web_usage_controller_callback_called_flag) {
  *web_usage_controller_callback_called_flag = true;
}

// Callback passed to user_id::mojom::UserId::GetUserId(). Verifies that the
// passed-back user ID has the expected value and sets
// |user_id_callback_called_flag| to true to indicate that the callback was
// invoked. |user_id| is passed simply to ensure that our connection to the
// UserId implementation remains alive long enough for the callback to reach
// us.
void OnGotUserId(user_id::mojom::UserIdPtr user_id,
                 bool* user_id_callback_called_flag,
                 const std::string& expected_user_id,
                 const std::string& received_user_id) {
  GREYAssert(expected_user_id == received_user_id,
             @"Unexpected User ID passed to user_id callback: %s",
             received_user_id.c_str());
  *user_id_callback_called_flag = true;
}

// Waits until a given callback is invoked (as signalled by that callback
// setting |callback_called_flag| to true).
void WaitForCallback(const std::string& callback_name,
                     bool* callback_called_flag) {
  GREYCondition* condition =
      [GREYCondition conditionWithName:@"Wait for callback"
                                 block:^BOOL {
                                   return *callback_called_flag;
                                 }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for %s callback", callback_name.c_str());
}
}

// Service Manager test cases for the web shell.
@interface ServiceManagerTestCase : WebShellTestCase
@end

@implementation ServiceManagerTestCase

// Tests that it is possible to connect to an all-users embedded service that
// was registered by web_shell.
- (void)testConnectionToAllUsersEmbeddedService {
  // Connect to the echo service and bind an Echo instance.
  echo::mojom::EchoPtr echo;
  web::ServiceManagerConnection* connection =
      web::ServiceManagerConnection::Get();
  DCHECK(connection);
  connection->GetConnector()->BindInterface("echo", mojo::MakeRequest(&echo));

  // Call EchoString, making sure to keep our end of the connection alive
  // until the callback is received.
  echo::mojom::Echo* rawEcho = echo.get();
  bool echoCallbackCalled = false;
  rawEcho->EchoString(
      kTestInput,
      base::BindOnce(&OnEchoString, base::Passed(&echo), &echoCallbackCalled));

  WaitForCallback("EchoString", &echoCallbackCalled);
}

// Tests that it is possible to connect to a per-user embedded service that
// was registered by web_shell.
- (void)testConnectionToPerUserEmbeddedService {
  // Connect to the user ID service and bind a UserId instance.
  user_id::mojom::UserIdPtr userID;
  web::WebState* webState = web::shell_test_util::GetCurrentWebState();
  web::BrowserState::GetConnectorFor(webState->GetBrowserState())
      ->BindInterface("user_id", mojo::MakeRequest(&userID));

  // Call GetUserId(), making sure to keep our end of the connection alive
  // until the callback is received.
  user_id::mojom::UserId* rawUserID = userID.get();
  bool userIDCallbackCalled = false;
  rawUserID->GetUserId(base::BindOnce(
      &OnGotUserId, base::Passed(&userID), &userIDCallbackCalled,
      web::BrowserState::GetServiceUserIdFor(webState->GetBrowserState())));

  WaitForCallback("GetUserId", &userIDCallbackCalled);
}

// Tests that it is possible to connect to a per-WebState interface that is
// provided by web_shell.
- (void)testConnectionToWebStateInterface {
  // Ensure that web usage is disabled to start with.
  web::WebState* webState = web::shell_test_util::GetCurrentWebState();
  webState->SetWebUsageEnabled(false);
  GREYAssert(!webState->IsWebUsageEnabled(),
             @"WebState did not have expected initial state");

  // Connect to a mojom::WebUsageController instance associated with
  // |webState|.
  web::mojom::WebUsageControllerPtr webUsageController;
  webState->BindInterfaceRequestFromMainFrame(
      mojo::MakeRequest(&webUsageController));

  // Call SetWebUsageEnabled(), making sure to keep our end of the connection
  // alive until the callback is received.
  web::mojom::WebUsageController* rawWebUsageController =
      webUsageController.get();
  bool webUsageControllerCallbackCalled = false;
  rawWebUsageController->SetWebUsageEnabled(
      true, base::BindOnce(&OnWebUsageSet, base::Passed(&webUsageController),
                           &webUsageControllerCallbackCalled));

  WaitForCallback("SetWebUsageEnabled", &webUsageControllerCallbackCalled);

  // Verify that the call altered |webState| as expected.
  GREYAssert(webState->IsWebUsageEnabled(),
             @"WebState did not have expected final state");
}

@end
