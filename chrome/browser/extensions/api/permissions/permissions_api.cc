// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api.h"

#include <memory>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/permissions/permissions_api_helpers.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_management.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/permissions.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/permissions/permissions_info.h"

namespace extensions {

using api::permissions::Permissions;

namespace Contains = api::permissions::Contains;
namespace GetAll = api::permissions::GetAll;
namespace Remove = api::permissions::Remove;
namespace Request  = api::permissions::Request;
namespace helpers = permissions_api_helpers;

namespace {

const char kBlockedByEnterprisePolicy[] =
    "Permissions are blocked by enterprise policy.";
const char kCantRemoveRequiredPermissionsError[] =
    "You cannot remove required permissions.";
const char kNotInOptionalPermissionsError[] =
    "Optional permissions must be listed in extension manifest.";
const char kNotWhitelistedError[] =
    "The optional permissions API does not support '*'.";
const char kUserGestureRequiredError[] =
    "This function must be called during a user gesture";

enum AutoConfirmForTest {
  DO_NOT_SKIP = 0,
  PROCEED,
  ABORT
};
AutoConfirmForTest auto_confirm_for_tests = DO_NOT_SKIP;
bool ignore_user_gesture_for_tests = false;

}  // namespace

ExtensionFunction::ResponseAction PermissionsContainsFunction::Run() {
  std::unique_ptr<Contains::Params> params(Contains::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  // NOTE: |permissions| is not used to make any security decisions. Therefore,
  // it is entirely fine to set |allow_file_access| to true below. This will
  // avoid throwing error when extension() doesn't have access to file://.
  std::string error;
  std::unique_ptr<const PermissionSet> permissions =
      helpers::UnpackPermissionSet(params->permissions,
                                   true /* allow_file_access */, &error);
  if (!permissions.get())
    return RespondNow(Error(error));

  return RespondNow(ArgumentList(Contains::Results::Create(
      extension()->permissions_data()->active_permissions().Contains(
          *permissions))));
}

ExtensionFunction::ResponseAction PermissionsGetAllFunction::Run() {
  std::unique_ptr<Permissions> permissions = helpers::PackPermissionSet(
      extension()->permissions_data()->active_permissions());
  return RespondNow(ArgumentList(GetAll::Results::Create(*permissions)));
}

ExtensionFunction::ResponseAction PermissionsRemoveFunction::Run() {
  std::unique_ptr<Remove::Params> params(Remove::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string error;
  std::unique_ptr<const PermissionSet> permissions =
      helpers::UnpackPermissionSet(params->permissions,
                                   ExtensionPrefs::Get(browser_context())
                                       ->AllowFileAccess(extension_->id()),
                                   &error);

  if (!permissions.get())
    return RespondNow(Error(error));

  // Make sure they're only trying to remove permissions supported by this API.
  APIPermissionSet apis = permissions->apis();
  for (const APIPermission* permission : apis) {
    if (!permission->info()->supports_optional())
      return RespondNow(Error(kNotWhitelistedError, permission->name()));
  }

  // Make sure we only remove optional permissions, and not required
  // permissions. Sadly, for some reason we support having a permission be both
  // optional and required (and should assume its required), so we need both of
  // these checks.
  // TODO(devlin): *Why* do we support that? Should be a load error.
  const PermissionSet& optional =
      PermissionsParser::GetOptionalPermissions(extension());
  const PermissionSet& required =
      PermissionsParser::GetRequiredPermissions(extension());
  if (!optional.Contains(*permissions) ||
      !std::unique_ptr<const PermissionSet>(
           PermissionSet::CreateIntersection(*permissions, required))
           ->IsEmpty()) {
    return RespondNow(Error(kCantRemoveRequiredPermissionsError));
  }

  // Only try and remove those permissions that are active on the extension.
  // For backwards compatability with behavior before this check was added, just
  // silently remove any that aren't present.
  permissions = PermissionSet::CreateIntersection(
      *permissions, extension()->permissions_data()->active_permissions());

  PermissionsUpdater(browser_context())
      .RemovePermissions(extension(), *permissions,
                         PermissionsUpdater::REMOVE_SOFT);
  return RespondNow(ArgumentList(Remove::Results::Create(true)));
}

// static
void PermissionsRequestFunction::SetAutoConfirmForTests(bool should_proceed) {
  auto_confirm_for_tests = should_proceed ? PROCEED : ABORT;
}

// static
void PermissionsRequestFunction::SetIgnoreUserGestureForTests(
    bool ignore) {
  ignore_user_gesture_for_tests = ignore;
}

PermissionsRequestFunction::PermissionsRequestFunction() {}

PermissionsRequestFunction::~PermissionsRequestFunction() {}

ExtensionFunction::ResponseAction PermissionsRequestFunction::Run() {
  if (!user_gesture() &&
      !ignore_user_gesture_for_tests &&
      extension_->location() != Manifest::COMPONENT) {
    return RespondNow(Error(kUserGestureRequiredError));
  }

  gfx::NativeWindow native_window =
      ChromeExtensionFunctionDetails(this).GetNativeWindowForUI();
  if (!native_window)
    return RespondNow(Error("Could not find an active window."));

  std::unique_ptr<Request::Params> params(Request::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string error;
  requested_permissions_ = helpers::UnpackPermissionSet(
      params->permissions,
      ExtensionPrefs::Get(browser_context())->AllowFileAccess(extension_->id()),
      &error);
  if (!requested_permissions_.get())
    return RespondNow(Error(error));

  // Make sure they're only requesting permissions supported by this API.
  APIPermissionSet apis = requested_permissions_->apis();
  for (APIPermissionSet::const_iterator i = apis.begin();
       i != apis.end(); ++i) {
    if (!i->info()->supports_optional()) {
      return RespondNow(Error(
          ErrorUtils::FormatErrorMessage(kNotWhitelistedError, i->name())));
    }
  }

  // The requested permissions must be defined as optional in the manifest.
  if (!PermissionsParser::GetOptionalPermissions(extension())
           .Contains(*requested_permissions_)) {
    return RespondNow(Error(kNotInOptionalPermissionsError));
  }

  // Automatically declines api permissions requests, which are blocked by
  // enterprise policy.
  if (!ExtensionManagementFactory::GetForBrowserContext(browser_context())
           ->IsPermissionSetAllowed(extension(), *requested_permissions_)) {
    return RespondNow(Error(kBlockedByEnterprisePolicy));
  }

  // We don't need to prompt the user if the requested permissions are a subset
  // of the granted permissions set.
  std::unique_ptr<const PermissionSet> granted =
      ExtensionPrefs::Get(browser_context())
          ->GetGrantedPermissions(extension()->id());
  if (granted.get() && granted->Contains(*requested_permissions_)) {
    PermissionsUpdater perms_updater(browser_context());
    perms_updater.AddPermissions(extension(), *requested_permissions_);
    return RespondNow(ArgumentList(Request::Results::Create(true)));
  }

  // Filter out the granted permissions so we only prompt for new ones.
  requested_permissions_ =
      PermissionSet::CreateDifference(*requested_permissions_, *granted);

  // Filter out the active permissions.
  requested_permissions_ = PermissionSet::CreateDifference(
      *requested_permissions_,
      extension()->permissions_data()->active_permissions());

  AddRef();  // Balanced in OnInstallPromptDone().

  // We don't need to show the prompt if there are no new warnings, or if
  // we're skipping the confirmation UI. All extension types but INTERNAL
  // are allowed to silently increase their permission level.
  const PermissionMessageProvider* message_provider =
      PermissionMessageProvider::Get();
  bool has_no_warnings =
      message_provider->GetPermissionMessages(
                          message_provider->GetAllPermissionIDs(
                              *requested_permissions_, extension()->GetType()))
          .empty();
  if (auto_confirm_for_tests == PROCEED || has_no_warnings ||
      extension_->location() == Manifest::COMPONENT) {
    OnInstallPromptDone(ExtensionInstallPrompt::Result::ACCEPTED);
    return AlreadyResponded();
  }

  if (auto_confirm_for_tests == ABORT) {
    // Pretend the user clicked cancel.
    OnInstallPromptDone(ExtensionInstallPrompt::Result::USER_CANCELED);
    return AlreadyResponded();
  }

  CHECK_EQ(DO_NOT_SKIP, auto_confirm_for_tests);
  install_ui_.reset(new ExtensionInstallPrompt(
      Profile::FromBrowserContext(browser_context()), native_window));
  install_ui_->ShowDialog(
      base::Bind(&PermissionsRequestFunction::OnInstallPromptDone, this),
      extension(), nullptr,
      std::make_unique<ExtensionInstallPrompt::Prompt>(
          ExtensionInstallPrompt::PERMISSIONS_PROMPT),
      requested_permissions_->Clone(),
      ExtensionInstallPrompt::GetDefaultShowDialogCallback());

  // ExtensionInstallPrompt::ShowDialog() can call the response synchronously.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void PermissionsRequestFunction::OnInstallPromptDone(
    ExtensionInstallPrompt::Result result) {
  bool granted = result == ExtensionInstallPrompt::Result::ACCEPTED;
  if (granted) {
    PermissionsUpdater perms_updater(browser_context());
    perms_updater.AddPermissions(extension(), *requested_permissions_);
  }

  Respond(ArgumentList(Request::Results::Create(granted)));
  Release();  // Balanced in Run().
}

}  // namespace extensions
