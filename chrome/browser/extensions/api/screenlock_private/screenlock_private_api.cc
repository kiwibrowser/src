// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/screenlock_private/screenlock_private_api.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/easy_unlock/chrome_proximity_auth_client.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/screenlock_private.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/event_router.h"

namespace extensions {

namespace screenlock = api::screenlock_private;

namespace {

screenlock::AuthType FromLockHandlerAuthType(
    proximity_auth::mojom::AuthType auth_type) {
  switch (auth_type) {
    case proximity_auth::mojom::AuthType::OFFLINE_PASSWORD:
      return screenlock::AUTH_TYPE_OFFLINEPASSWORD;
    case proximity_auth::mojom::AuthType::NUMERIC_PIN:
      return screenlock::AUTH_TYPE_NUMERICPIN;
    case proximity_auth::mojom::AuthType::USER_CLICK:
      return screenlock::AUTH_TYPE_USERCLICK;
    case proximity_auth::mojom::AuthType::ONLINE_SIGN_IN:
      // Apps should treat forced online sign in same as system password.
      return screenlock::AUTH_TYPE_OFFLINEPASSWORD;
    case proximity_auth::mojom::AuthType::EXPAND_THEN_USER_CLICK:
      // This type is used for public sessions, which do not support screen
      // locking.
      NOTREACHED();
      return screenlock::AUTH_TYPE_NONE;
    case proximity_auth::mojom::AuthType::FORCE_OFFLINE_PASSWORD:
      return screenlock::AUTH_TYPE_OFFLINEPASSWORD;
  }
  NOTREACHED();
  return screenlock::AUTH_TYPE_OFFLINEPASSWORD;
}

}  // namespace

ScreenlockPrivateGetLockedFunction::ScreenlockPrivateGetLockedFunction() {}

ScreenlockPrivateGetLockedFunction::~ScreenlockPrivateGetLockedFunction() {}

bool ScreenlockPrivateGetLockedFunction::RunAsync() {
  SetResult(std::make_unique<base::Value>(
      proximity_auth::ScreenlockBridge::Get()->IsLocked()));
  SendResponse(error_.empty());
  return true;
}

ScreenlockPrivateSetLockedFunction::ScreenlockPrivateSetLockedFunction() {}

ScreenlockPrivateSetLockedFunction::~ScreenlockPrivateSetLockedFunction() {}

bool ScreenlockPrivateSetLockedFunction::RunAsync() {
  std::unique_ptr<screenlock::SetLocked::Params> params(
      screenlock::SetLocked::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  chromeos::EasyUnlockService* service =
      chromeos::EasyUnlockService::Get(GetProfile());
  if (params->locked) {
    if (extension()->id() == extension_misc::kEasyUnlockAppId &&
        AppWindowRegistry::Get(browser_context())
            ->GetAppWindowForAppAndKey(extension()->id(),
                                       "easy_unlock_pairing")) {
      // Mark the Easy Unlock behaviour on the lock screen as the one initiated
      // by the Easy Unlock setup app as a trial one.
      // TODO(tbarzic): Move this logic to a new easyUnlockPrivate function.
      service->SetTrialRun();
    }
    proximity_auth::ScreenlockBridge::Get()->Lock();
  } else {
    proximity_auth::ScreenlockBridge::Get()->Unlock(AccountId::FromUserEmail(
        service->proximity_auth_client()->GetAuthenticatedUsername()));
  }
  SendResponse(error_.empty());
  return true;
}

ScreenlockPrivateAcceptAuthAttemptFunction::
    ScreenlockPrivateAcceptAuthAttemptFunction() {}

ScreenlockPrivateAcceptAuthAttemptFunction::
    ~ScreenlockPrivateAcceptAuthAttemptFunction() {}

ExtensionFunction::ResponseAction
ScreenlockPrivateAcceptAuthAttemptFunction::Run() {
  std::unique_ptr<screenlock::AcceptAuthAttempt::Params> params(
      screenlock::AcceptAuthAttempt::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  chromeos::EasyUnlockService* service =
      chromeos::EasyUnlockService::Get(profile);
  if (service)
    service->FinalizeUnlock(params->accept);
  return RespondNow(NoArguments());
}

ScreenlockPrivateEventRouter::ScreenlockPrivateEventRouter(
    content::BrowserContext* context)
    : browser_context_(context) {
  proximity_auth::ScreenlockBridge::Get()->AddObserver(this);
}

ScreenlockPrivateEventRouter::~ScreenlockPrivateEventRouter() {}

void ScreenlockPrivateEventRouter::OnScreenDidLock(
    proximity_auth::ScreenlockBridge::LockHandler::ScreenType screen_type) {
  DispatchEvent(events::SCREENLOCK_PRIVATE_ON_CHANGED,
                screenlock::OnChanged::kEventName,
                std::make_unique<base::Value>(true));
}

void ScreenlockPrivateEventRouter::OnScreenDidUnlock(
    proximity_auth::ScreenlockBridge::LockHandler::ScreenType screen_type) {
  DispatchEvent(events::SCREENLOCK_PRIVATE_ON_CHANGED,
                screenlock::OnChanged::kEventName,
                std::make_unique<base::Value>(false));
}

void ScreenlockPrivateEventRouter::OnFocusedUserChanged(
    const AccountId& account_id) {}

void ScreenlockPrivateEventRouter::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::Value> arg) {
  std::unique_ptr<base::ListValue> args(new base::ListValue());
  if (arg)
    args->Append(std::move(arg));
  std::unique_ptr<Event> event(
      new Event(histogram_value, event_name, std::move(args)));
  EventRouter::Get(browser_context_)->BroadcastEvent(std::move(event));
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ScreenlockPrivateEventRouter>>::DestructorAtExit
    g_screenlock_private_api_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<ScreenlockPrivateEventRouter>*
ScreenlockPrivateEventRouter::GetFactoryInstance() {
  return g_screenlock_private_api_factory.Pointer();
}

void ScreenlockPrivateEventRouter::Shutdown() {
  proximity_auth::ScreenlockBridge::Get()->RemoveObserver(this);
}

bool ScreenlockPrivateEventRouter::OnAuthAttempted(
    proximity_auth::mojom::AuthType auth_type,
    const std::string& value) {
  EventRouter* router = EventRouter::Get(browser_context_);
  if (!router->HasEventListener(screenlock::OnAuthAttempted::kEventName))
    return false;

  std::unique_ptr<base::ListValue> args(new base::ListValue());
  args->AppendString(screenlock::ToString(FromLockHandlerAuthType(auth_type)));
  args->AppendString(value);

  std::unique_ptr<Event> event(
      new Event(events::SCREENLOCK_PRIVATE_ON_AUTH_ATTEMPTED,
                screenlock::OnAuthAttempted::kEventName, std::move(args)));
  router->BroadcastEvent(std::move(event));
  return true;
}

}  // namespace extensions
