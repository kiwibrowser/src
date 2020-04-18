// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/autotest_private/autotest_private_api.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/autotest_private.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"

#if defined(OS_CHROMEOS)
#include "ash/public/interfaces/ash_message_center_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/feature_list.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/system/input_device_settings.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chrome/common/chrome_features.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/printing/printer_configuration.h"
#include "components/user_manager/user_manager.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "net/base/filename_util.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/ui_base_features.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#endif

namespace extensions {
namespace {

std::unique_ptr<base::ListValue> GetHostPermissions(const Extension* ext,
                                                    bool effective_perm) {
  const PermissionsData* permissions_data = ext->permissions_data();
  const URLPatternSet& pattern_set =
      effective_perm ? permissions_data->GetEffectiveHostPermissions()
                     : permissions_data->active_permissions().explicit_hosts();

  auto permissions = std::make_unique<base::ListValue>();
  for (URLPatternSet::const_iterator perm = pattern_set.begin();
       perm != pattern_set.end();
       ++perm) {
    permissions->AppendString(perm->GetAsString());
  }

  return permissions;
}

std::unique_ptr<base::ListValue> GetAPIPermissions(const Extension* ext) {
  auto permissions = std::make_unique<base::ListValue>();
  std::set<std::string> perm_list =
      ext->permissions_data()->active_permissions().GetAPIsAsStrings();
  for (std::set<std::string>::const_iterator perm = perm_list.begin();
       perm != perm_list.end(); ++perm) {
    permissions->AppendString(*perm);
  }
  return permissions;
}

bool IsTestMode(content::BrowserContext* context) {
  return AutotestPrivateAPI::GetFactoryInstance()->Get(context)->test_mode();
}

#if defined(OS_CHROMEOS)
std::string ConvertToString(message_center::NotificationType type) {
  switch (type) {
    case message_center::NOTIFICATION_TYPE_SIMPLE:
      return "simple";
    case message_center::NOTIFICATION_TYPE_BASE_FORMAT:
      return "base_format";
    case message_center::NOTIFICATION_TYPE_IMAGE:
      return "image";
    case message_center::NOTIFICATION_TYPE_MULTIPLE:
      return "multiple";
    case message_center::NOTIFICATION_TYPE_PROGRESS:
      return "progress";
    case message_center::NOTIFICATION_TYPE_CUSTOM:
      return "custom";
  }
  return "unknown";
}

std::unique_ptr<base::DictionaryValue> MakeDictionaryFromNotification(
    const message_center::Notification& notification) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString("id", notification.id());
  result->SetString("type", ConvertToString(notification.type()));
  result->SetString("title", notification.title());
  result->SetString("message", notification.message());
  result->SetInteger("priority", notification.priority());
  result->SetInteger("progress", notification.progress());
  return result;
}
#endif

}  // namespace

ExtensionFunction::ResponseAction AutotestPrivateLogoutFunction::Run() {
  DVLOG(1) << "AutotestPrivateLogoutFunction";
  if (!IsTestMode(browser_context()))
    chrome::AttemptUserExit();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AutotestPrivateRestartFunction::Run() {
  DVLOG(1) << "AutotestPrivateRestartFunction";
  if (!IsTestMode(browser_context()))
    chrome::AttemptRestart();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AutotestPrivateShutdownFunction::Run() {
  std::unique_ptr<api::autotest_private::Shutdown::Params> params(
      api::autotest_private::Shutdown::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateShutdownFunction " << params->force;

  if (!IsTestMode(browser_context()))
    chrome::AttemptExit();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AutotestPrivateLoginStatusFunction::Run() {
  DVLOG(1) << "AutotestPrivateLoginStatusFunction";

#if defined(OS_CHROMEOS)
  LoginScreenClient::Get()->login_screen()->IsReadyForPassword(base::BindOnce(
      &AutotestPrivateLoginStatusFunction::OnIsReadyForPassword, this));
  return RespondLater();
#else
  return RespondNow(OneArgument(std::make_unique<base::DictionaryValue>()));
#endif
}

#if defined(OS_CHROMEOS)
void AutotestPrivateLoginStatusFunction::OnIsReadyForPassword(bool is_ready) {
  auto result = std::make_unique<base::DictionaryValue>();
  const user_manager::UserManager* user_manager =
      user_manager::UserManager::Get();

  // default_screen_locker()->locked() is set when the UI is ready, so this
  // tells us both views based lockscreen UI and screenlocker are ready.
  const bool is_screen_locked =
      !!chromeos::ScreenLocker::default_screen_locker() &&
      chromeos::ScreenLocker::default_screen_locker()->locked();

  if (user_manager) {
    result->SetBoolean("isLoggedIn", user_manager->IsUserLoggedIn());
    result->SetBoolean("isOwner", user_manager->IsCurrentUserOwner());
    result->SetBoolean("isScreenLocked", is_screen_locked);
    result->SetBoolean("isReadyForPassword", is_ready);
    if (user_manager->IsUserLoggedIn()) {
      result->SetBoolean("isRegularUser",
                         user_manager->IsLoggedInAsUserWithGaiaAccount());
      result->SetBoolean("isGuest", user_manager->IsLoggedInAsGuest());
      result->SetBoolean("isKiosk", user_manager->IsLoggedInAsKioskApp());

      const user_manager::User* user = user_manager->GetActiveUser();
      result->SetString("email", user->GetAccountId().GetUserEmail());
      result->SetString("displayEmail", user->display_email());

      std::string user_image;
      switch (user->image_index()) {
        case user_manager::User::USER_IMAGE_EXTERNAL:
          user_image = "file";
          break;

        case user_manager::User::USER_IMAGE_PROFILE:
          user_image = "profile";
          break;

        default:
          user_image = base::IntToString(user->image_index());
          break;
      }
      result->SetString("userImage", user_image);
    }
  }
  Respond(OneArgument(std::move(result)));
}
#endif

ExtensionFunction::ResponseAction AutotestPrivateLockScreenFunction::Run() {
  DVLOG(1) << "AutotestPrivateLockScreenFunction";
#if defined(OS_CHROMEOS)
  chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->
      RequestLockScreen();
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateGetExtensionsInfoFunction::Run() {
  DVLOG(1) << "AutotestPrivateGetExtensionsInfoFunction";

  ExtensionService* service =
      ExtensionSystem::Get(browser_context())->extension_service();
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());
  const ExtensionSet& extensions = registry->enabled_extensions();
  const ExtensionSet& disabled_extensions = registry->disabled_extensions();
  ExtensionActionManager* extension_action_manager =
      ExtensionActionManager::Get(browser_context());

  auto extensions_values = std::make_unique<base::ListValue>();
  ExtensionList all;
  all.insert(all.end(), extensions.begin(), extensions.end());
  all.insert(all.end(), disabled_extensions.begin(), disabled_extensions.end());
  for (ExtensionList::const_iterator it = all.begin();
       it != all.end(); ++it) {
    const Extension* extension = it->get();
    std::string id = extension->id();
    std::unique_ptr<base::DictionaryValue> extension_value(
        new base::DictionaryValue);
    extension_value->SetString("id", id);
    extension_value->SetString("version", extension->VersionString());
    extension_value->SetString("name", extension->name());
    extension_value->SetString("publicKey", extension->public_key());
    extension_value->SetString("description", extension->description());
    extension_value->SetString(
        "backgroundUrl", BackgroundInfo::GetBackgroundURL(extension).spec());
    extension_value->SetString(
        "optionsUrl", OptionsPageInfo::GetOptionsPage(extension).spec());

    extension_value->Set("hostPermissions",
                         GetHostPermissions(extension, false));
    extension_value->Set("effectiveHostPermissions",
                         GetHostPermissions(extension, true));
    extension_value->Set("apiPermissions", GetAPIPermissions(extension));

    Manifest::Location location = extension->location();
    extension_value->SetBoolean("isComponent",
                                location == Manifest::COMPONENT);
    extension_value->SetBoolean("isInternal",
                                location == Manifest::INTERNAL);
    extension_value->SetBoolean("isUserInstalled",
        location == Manifest::INTERNAL ||
        Manifest::IsUnpackedLocation(location));
    extension_value->SetBoolean("isEnabled", service->IsExtensionEnabled(id));
    extension_value->SetBoolean(
        "allowedInIncognito", util::IsIncognitoEnabled(id, browser_context()));
    extension_value->SetBoolean(
        "hasPageAction",
        extension_action_manager->GetPageAction(*extension) != NULL);

    extensions_values->Append(std::move(extension_value));
  }

  std::unique_ptr<base::DictionaryValue> return_value(
      new base::DictionaryValue);
  return_value->Set("extensions", std::move(extensions_values));
  return RespondNow(OneArgument(std::move(return_value)));
}

static int AccessArray(const volatile int arr[], const volatile int *index) {
  return arr[*index];
}

ExtensionFunction::ResponseAction
AutotestPrivateSimulateAsanMemoryBugFunction::Run() {
  DVLOG(1) << "AutotestPrivateSimulateAsanMemoryBugFunction";
  if (!IsTestMode(browser_context())) {
    // This array is volatile not to let compiler optimize us out.
    volatile int testarray[3] = {0, 0, 0};

    // Cause Address Sanitizer to abort this process.
    volatile int index = 5;
    AccessArray(testarray, &index);
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetTouchpadSensitivityFunction::Run() {
  std::unique_ptr<api::autotest_private::SetTouchpadSensitivity::Params> params(
      api::autotest_private::SetTouchpadSensitivity::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetTouchpadSensitivityFunction " << params->value;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetTouchpadSensitivity(
      params->value);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AutotestPrivateSetTapToClickFunction::Run() {
  std::unique_ptr<api::autotest_private::SetTapToClick::Params> params(
      api::autotest_private::SetTapToClick::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetTapToClickFunction " << params->enabled;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetTapToClick(params->enabled);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetThreeFingerClickFunction::Run() {
  std::unique_ptr<api::autotest_private::SetThreeFingerClick::Params> params(
      api::autotest_private::SetThreeFingerClick::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetThreeFingerClickFunction " << params->enabled;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetThreeFingerClick(
      params->enabled);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AutotestPrivateSetTapDraggingFunction::Run() {
  std::unique_ptr<api::autotest_private::SetTapDragging::Params> params(
      api::autotest_private::SetTapDragging::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetTapDraggingFunction " << params->enabled;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetTapDragging(params->enabled);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetNaturalScrollFunction::Run() {
  std::unique_ptr<api::autotest_private::SetNaturalScroll::Params> params(
      api::autotest_private::SetNaturalScroll::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetNaturalScrollFunction " << params->enabled;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetNaturalScroll(
      params->enabled);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetMouseSensitivityFunction::Run() {
  std::unique_ptr<api::autotest_private::SetMouseSensitivity::Params> params(
      api::autotest_private::SetMouseSensitivity::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetMouseSensitivityFunction " << params->value;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetMouseSensitivity(
      params->value);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetPrimaryButtonRightFunction::Run() {
  std::unique_ptr<api::autotest_private::SetPrimaryButtonRight::Params> params(
      api::autotest_private::SetPrimaryButtonRight::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetPrimaryButtonRightFunction " << params->right;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetPrimaryButtonRight(
      params->right);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateSetMouseReverseScrollFunction::Run() {
  std::unique_ptr<api::autotest_private::SetMouseReverseScroll::Params> params(
      api::autotest_private::SetMouseReverseScroll::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AutotestPrivateSetMouseReverseScrollFunction "
           << params->enabled;

#if defined(OS_CHROMEOS)
  chromeos::system::InputDeviceSettings::Get()->SetMouseReverseScroll(
      params->enabled);
#endif
  return RespondNow(NoArguments());
}

AutotestPrivateGetVisibleNotificationsFunction::
    AutotestPrivateGetVisibleNotificationsFunction() = default;
AutotestPrivateGetVisibleNotificationsFunction::
    ~AutotestPrivateGetVisibleNotificationsFunction() = default;

ExtensionFunction::ResponseAction
AutotestPrivateGetVisibleNotificationsFunction::Run() {
  DVLOG(1) << "AutotestPrivateGetVisibleNotificationsFunction";
  std::unique_ptr<base::ListValue> values(new base::ListValue);
#if defined(OS_CHROMEOS)
  if (base::FeatureList::IsEnabled(features::kNativeNotifications) ||
      base::FeatureList::IsEnabled(features::kMash)) {
    auto* connection = content::ServiceManagerConnection::GetForProcess();
    connection->GetConnector()->BindInterface(ash::mojom::kServiceName,
                                              &controller_);
    controller_->GetActiveNotifications(base::BindOnce(
        &AutotestPrivateGetVisibleNotificationsFunction::OnGotNotifications,
        this));
    return RespondLater();
  }

  CHECK(message_center::MessageCenter::Get());
  for (auto* notification :
       message_center::MessageCenter::Get()->GetVisibleNotifications()) {
    values->Append(MakeDictionaryFromNotification(*notification));
  }

#endif
  return RespondNow(OneArgument(std::move(values)));
}

#if defined(OS_CHROMEOS)
void AutotestPrivateGetVisibleNotificationsFunction::OnGotNotifications(
    const std::vector<message_center::Notification>& notifications) {
  auto values = std::make_unique<base::ListValue>();
  for (const auto& notification : notifications) {
    values->Append(MakeDictionaryFromNotification(notification));
  }
  Respond(OneArgument(std::move(values)));
}

// static
std::string AutotestPrivateGetPrinterListFunction::GetPrinterType(
    chromeos::CupsPrintersManager::PrinterClass type) {
  switch (type) {
    case chromeos::CupsPrintersManager::PrinterClass::kConfigured:
      return "configured";
    case chromeos::CupsPrintersManager::PrinterClass::kEnterprise:
      return "enterprise";
    case chromeos::CupsPrintersManager::PrinterClass::kAutomatic:
      return "automatic";
    case chromeos::CupsPrintersManager::PrinterClass::kDiscovered:
      return "discovered";
    default:
      return "unknown";
  }
}
#endif

ExtensionFunction::ResponseAction AutotestPrivateGetPrinterListFunction::Run() {
  DVLOG(1) << "AutotestPrivateGetPrinterListFunction";
  auto values = std::make_unique<base::ListValue>();
#if defined(OS_CHROMEOS)
  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<chromeos::CupsPrintersManager> printers_manager =
      chromeos::CupsPrintersManager::Create(profile);
  std::vector<chromeos::CupsPrintersManager::PrinterClass> printer_type = {
      chromeos::CupsPrintersManager::PrinterClass::kConfigured,
      chromeos::CupsPrintersManager::PrinterClass::kEnterprise,
      chromeos::CupsPrintersManager::PrinterClass::kAutomatic};
  for (const auto& type : printer_type) {
    std::vector<chromeos::Printer> printer_list =
        printers_manager->GetPrinters(type);
    for (const auto& printer : printer_list) {
      auto result = std::make_unique<base::DictionaryValue>();
      result->SetString("printerName", printer.display_name());
      result->SetString("printerId", printer.id());
      result->SetString("printerType", GetPrinterType(type));
      values->Append(std::move(result));
    }
  }
#endif
  return RespondNow(OneArgument(std::move(values)));
}

AutotestPrivateUpdatePrinterFunction::AutotestPrivateUpdatePrinterFunction() =
    default;
AutotestPrivateUpdatePrinterFunction::~AutotestPrivateUpdatePrinterFunction() =
    default;

ExtensionFunction::ResponseAction AutotestPrivateUpdatePrinterFunction::Run() {
  std::unique_ptr<api::autotest_private::UpdatePrinter::Params> params(
      api::autotest_private::UpdatePrinter::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  DVLOG(1) << "AutotestPrivateUpdatePrinterFunction";
#if defined(OS_CHROMEOS)
  const api::autotest_private::Printer& js_printer = params->printer;
  chromeos::Printer printer(js_printer.printer_id ? *js_printer.printer_id
                                                  : "");
  printer.set_display_name(js_printer.printer_name);
  if (js_printer.printer_desc)
    printer.set_description(*js_printer.printer_desc);

  if (js_printer.printer_make_and_model)
    printer.set_make_and_model(*js_printer.printer_make_and_model);

  if (js_printer.printer_uri)
    printer.set_uri(*js_printer.printer_uri);

  if (js_printer.printer_ppd) {
    const GURL ppd =
        net::FilePathToFileURL(base::FilePath(*js_printer.printer_ppd));
    if (ppd.is_valid())
      printer.mutable_ppd_reference()->user_supplied_ppd_url = ppd.spec();
    else
      LOG(ERROR) << "Invalid ppd path: " << *js_printer.printer_ppd;
  }
  auto printers_manager = chromeos::CupsPrintersManager::Create(
      ProfileManager::GetActiveUserProfile());
  printers_manager->UpdateConfiguredPrinter(printer);
#endif
  return RespondNow(NoArguments());
}

AutotestPrivateRemovePrinterFunction::AutotestPrivateRemovePrinterFunction() =
    default;
AutotestPrivateRemovePrinterFunction::~AutotestPrivateRemovePrinterFunction() =
    default;

ExtensionFunction::ResponseAction AutotestPrivateRemovePrinterFunction::Run() {
  std::unique_ptr<api::autotest_private::RemovePrinter::Params> params(
      api::autotest_private::RemovePrinter::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  DVLOG(1) << "AutotestPrivateRemovePrinterFunction";
#if defined(OS_CHROMEOS)
  auto printers_manager = chromeos::CupsPrintersManager::Create(
      ProfileManager::GetActiveUserProfile());
  printers_manager->RemoveConfiguredPrinter(params->printer_id);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AutotestPrivateGetPlayStoreStateFunction::Run() {
  DVLOG(1) << "AutotestPrivateGetPlayStoreStateFunction";
  api::autotest_private::PlayStoreState play_store_state;
  play_store_state.allowed = false;
#if defined(OS_CHROMEOS)
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (arc::IsArcAllowedForProfile(profile)) {
    play_store_state.allowed = true;
    play_store_state.enabled =
        std::make_unique<bool>(arc::IsArcPlayStoreEnabledForProfile(profile));
    play_store_state.managed = std::make_unique<bool>(
        arc::IsArcPlayStoreEnabledPreferenceManagedForProfile(profile));
  }
#endif
  return RespondNow(OneArgument(play_store_state.ToValue()));
}

ExtensionFunction::ResponseAction
AutotestPrivateSetPlayStoreEnabledFunction::Run() {
  DVLOG(1) << "AutotestPrivateSetPlayStoreEnabledFunction";
  std::unique_ptr<api::autotest_private::SetPlayStoreEnabled::Params> params(
      api::autotest_private::SetPlayStoreEnabled::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
#if defined(OS_CHROMEOS)
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (arc::IsArcAllowedForProfile(profile)) {
    if (!arc::SetArcPlayStoreEnabledForProfile(profile, params->enabled)) {
      return RespondNow(
          Error("ARC enabled state cannot be changed for the current user"));
    }
    return RespondNow(NoArguments());
  } else {
    return RespondNow(Error("ARC is not available for the current user"));
  }
#endif
  return RespondNow(Error("ARC is not available for the current platform"));
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<AutotestPrivateAPI>>::
    DestructorAtExit g_autotest_private_api_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<AutotestPrivateAPI>*
AutotestPrivateAPI::GetFactoryInstance() {
  return g_autotest_private_api_factory.Pointer();
}

template <>
KeyedService*
BrowserContextKeyedAPIFactory<AutotestPrivateAPI>::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AutotestPrivateAPI();
}

AutotestPrivateAPI::AutotestPrivateAPI() : test_mode_(false) {
}

AutotestPrivateAPI::~AutotestPrivateAPI() {
}

}  // namespace extensions
