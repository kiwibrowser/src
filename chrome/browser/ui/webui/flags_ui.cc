// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/flags_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/flags_ui/flags_ui_constants.h"
#include "components/flags_ui/flags_ui_pref_names.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "components/strings/grit/components_strings.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_CHROMEOS)
#include "base/sys_info.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos.h"
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos_factory.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/owner_flags_storage.h"
#include "components/account_id/account_id.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_manager/user_manager.h"
#endif

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateFlagsUIHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIFlagsHost);

  source->AddLocalizedString(flags_ui::kFlagsRestartNotice,
                             IDS_FLAGS_UI_RELAUNCH_NOTICE);
  source->AddString(flags_ui::kVersion, version_info::GetVersionNumber());

#if defined(OS_CHROMEOS)
  if (!user_manager::UserManager::Get()->IsCurrentUserOwner() &&
      base::SysInfo::IsRunningOnChromeOS()) {
    // Set the string to show which user can actually change the flags.
    std::string owner;
    chromeos::CrosSettings::Get()->GetString(chromeos::kDeviceOwner, &owner);
    source->AddString(flags_ui::kOwnerEmail, base::UTF8ToUTF16(owner));
  } else {
    // The warning will be only shown on ChromeOS, when the current user is not
    // the owner.
    source->AddString(flags_ui::kOwnerEmail, base::string16());
  }
#endif

  source->AddResourcePath(flags_ui::kFlagsJS, IDR_FLAGS_UI_FLAGS_JS);
  source->SetDefaultResource(IDR_FLAGS_UI_FLAGS_HTML);
  source->UseGzip();
  return source;
}

////////////////////////////////////////////////////////////////////////////////
//
// FlagsDOMHandler
//
////////////////////////////////////////////////////////////////////////////////

// The handler for Javascript messages for the about:flags page.
class FlagsDOMHandler : public WebUIMessageHandler {
 public:
  FlagsDOMHandler() : access_(flags_ui::kGeneralAccessFlagsOnly),
                      experimental_features_requested_(false) {
  }
  ~FlagsDOMHandler() override {}

  // Initializes the DOM handler with the provided flags storage and flags
  // access. If there were flags experiments requested from javascript before
  // this was called, it calls |HandleRequestExperimentalFeatures| again.
  void Init(flags_ui::FlagsStorage* flags_storage,
            flags_ui::FlagAccess access);

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Callback for the "requestExperimentFeatures" message.
  void HandleRequestExperimentalFeatures(const base::ListValue* args);

  // Callback for the "enableExperimentalFeature" message.
  void HandleEnableExperimentalFeatureMessage(const base::ListValue* args);

  // Callback for the "setOriginListFlag" message.
  void HandleSetOriginListFlagMessage(const base::ListValue* args);

  // Callback for the "restartBrowser" message. Restores all tabs on restart.
  void HandleRestartBrowser(const base::ListValue* args);

  // Callback for the "resetAllFlags" message.
  void HandleResetAllFlags(const base::ListValue* args);

 private:
  std::unique_ptr<flags_ui::FlagsStorage> flags_storage_;
  flags_ui::FlagAccess access_;
  bool experimental_features_requested_;

  DISALLOW_COPY_AND_ASSIGN(FlagsDOMHandler);
};

void FlagsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      flags_ui::kRequestExperimentalFeatures,
      base::BindRepeating(&FlagsDOMHandler::HandleRequestExperimentalFeatures,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kEnableExperimentalFeature,
      base::BindRepeating(
          &FlagsDOMHandler::HandleEnableExperimentalFeatureMessage,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kSetOriginListFlag,
      base::BindRepeating(&FlagsDOMHandler::HandleSetOriginListFlagMessage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kRestartBrowser,
      base::BindRepeating(&FlagsDOMHandler::HandleRestartBrowser,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kResetAllFlags,
      base::BindRepeating(&FlagsDOMHandler::HandleResetAllFlags,
                          base::Unretained(this)));
}

void FlagsDOMHandler::Init(flags_ui::FlagsStorage* flags_storage,
                           flags_ui::FlagAccess access) {
  flags_storage_.reset(flags_storage);
  access_ = access;

  if (experimental_features_requested_)
    HandleRequestExperimentalFeatures(nullptr);
}

void FlagsDOMHandler::HandleRequestExperimentalFeatures(
    const base::ListValue* args) {
  experimental_features_requested_ = true;
  // Bail out if the handler hasn't been initialized yet. The request will be
  // handled after the initialization.
  if (!flags_storage_)
    return;

  base::DictionaryValue results;

  std::unique_ptr<base::ListValue> supported_features(new base::ListValue);
  std::unique_ptr<base::ListValue> unsupported_features(new base::ListValue);
  about_flags::GetFlagFeatureEntries(flags_storage_.get(),
                                     access_,
                                     supported_features.get(),
                                     unsupported_features.get());
  results.Set(flags_ui::kSupportedFeatures, std::move(supported_features));
  results.Set(flags_ui::kUnsupportedFeatures, std::move(unsupported_features));
  results.SetBoolean(flags_ui::kNeedsRestart,
                     about_flags::IsRestartNeededToCommitChanges());
  results.SetBoolean(flags_ui::kShowOwnerWarning,
                     access_ == flags_ui::kGeneralAccessFlagsOnly);

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
  version_info::Channel channel = chrome::GetChannel();
  results.SetBoolean(flags_ui::kShowBetaChannelPromotion,
                     channel == version_info::Channel::STABLE);
  results.SetBoolean(flags_ui::kShowDevChannelPromotion,
                     channel == version_info::Channel::BETA);
#else
  results.SetBoolean(flags_ui::kShowBetaChannelPromotion, false);
  results.SetBoolean(flags_ui::kShowDevChannelPromotion, false);
#endif
  web_ui()->CallJavascriptFunctionUnsafe(flags_ui::kReturnExperimentalFeatures,
                                         results);
}

void FlagsDOMHandler::HandleEnableExperimentalFeatureMessage(
    const base::ListValue* args) {
  DCHECK(flags_storage_);
  DCHECK_EQ(2u, args->GetSize());
  if (args->GetSize() != 2)
    return;

  std::string entry_internal_name;
  std::string enable_str;
  if (!args->GetString(0, &entry_internal_name) ||
      !args->GetString(1, &enable_str))
    return;

  about_flags::SetFeatureEntryEnabled(flags_storage_.get(), entry_internal_name,
                                      enable_str == "true");
}

void FlagsDOMHandler::HandleSetOriginListFlagMessage(
    const base::ListValue* args) {
  DCHECK(flags_storage_);
  if (args->GetSize() != 2) {
    NOTREACHED();
    return;
  }

  std::string entry_internal_name;
  std::string value_str;
  if (!args->GetString(0, &entry_internal_name) ||
      !args->GetString(1, &value_str) || entry_internal_name.empty()) {
    NOTREACHED();
    return;
  }

  about_flags::SetOriginListFlag(entry_internal_name, value_str,
                                 flags_storage_.get());
}

void FlagsDOMHandler::HandleRestartBrowser(const base::ListValue* args) {
  DCHECK(flags_storage_);
#if defined(OS_CHROMEOS)
  // On ChromeOS be less intrusive and restart inside the user session after
  // we apply the newly selected flags.
  base::CommandLine user_flags(base::CommandLine::NO_PROGRAM);
  about_flags::ConvertFlagsToSwitches(flags_storage_.get(),
                                      &user_flags,
                                      flags_ui::kAddSentinels);

  // Apply additional switches from policy that should not be dropped when
  // applying flags..
  chromeos::UserSessionManager::MaybeAppendPolicySwitches(
      Profile::FromWebUI(web_ui())->GetPrefs(), &user_flags);

  base::CommandLine::StringVector flags;
  // argv[0] is the program name |base::CommandLine::NO_PROGRAM|.
  flags.assign(user_flags.argv().begin() + 1, user_flags.argv().end());
  VLOG(1) << "Restarting to apply per-session flags...";
  AccountId account_id =
      user_manager::UserManager::Get()->GetActiveUser()->GetAccountId();
  chromeos::UserSessionManager::GetInstance()->SetSwitchesForUser(
      account_id,
      chromeos::UserSessionManager::CommandLineSwitchesType::
          kPolicyAndFlagsAndKioskControl,
      flags);
#endif
  chrome::AttemptRestart();
}

void FlagsDOMHandler::HandleResetAllFlags(const base::ListValue* args) {
  DCHECK(flags_storage_);
  about_flags::ResetAllFlags(flags_storage_.get());
}


#if defined(OS_CHROMEOS)
// On ChromeOS verifying if the owner is signed in is async operation and only
// after finishing it the UI can be properly populated. This function is the
// callback for whether the owner is signed in. It will respectively pick the
// proper PrefService for the flags interface.
void FinishInitialization(base::WeakPtr<FlagsUI> flags_ui,
                          Profile* profile,
                          FlagsDOMHandler* dom_handler,
                          bool current_user_is_owner) {
  // If the flags_ui has gone away, there's nothing to do.
  if (!flags_ui)
    return;

  // On Chrome OS the owner can set system wide flags and other users can only
  // set flags for their own session.
  // Note that |dom_handler| is owned by the web ui that owns |flags_ui|, so
  // it is still alive if |flags_ui| is.
  if (current_user_is_owner) {
    chromeos::OwnerSettingsServiceChromeOS* service =
        chromeos::OwnerSettingsServiceChromeOSFactory::GetForBrowserContext(
            profile);
    dom_handler->Init(new chromeos::about_flags::OwnerFlagsStorage(
                          profile->GetPrefs(), service),
                      flags_ui::kOwnerAccessToFlags);
  } else {
    dom_handler->Init(
        new flags_ui::PrefServiceFlagsStorage(profile->GetPrefs()),
        flags_ui::kGeneralAccessFlagsOnly);
  }
}
#endif

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// FlagsUI
//
///////////////////////////////////////////////////////////////////////////////

FlagsUI::FlagsUI(content::WebUI* web_ui)
    : WebUIController(web_ui),
      weak_factory_(this) {
  Profile* profile = Profile::FromWebUI(web_ui);

  auto handler_owner = std::make_unique<FlagsDOMHandler>();
  FlagsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));

#if defined(OS_CHROMEOS)
  if (base::SysInfo::IsRunningOnChromeOS() &&
      chromeos::OwnerSettingsServiceChromeOSFactory::GetForBrowserContext(
          profile)) {
    chromeos::OwnerSettingsServiceChromeOS* service =
        chromeos::OwnerSettingsServiceChromeOSFactory::GetForBrowserContext(
            profile);
    service->IsOwnerAsync(base::Bind(
        &FinishInitialization, weak_factory_.GetWeakPtr(), profile, handler));
  } else {
    FinishInitialization(weak_factory_.GetWeakPtr(), profile, handler,
                         false /* current_user_is_owner */);
  }
#else
  handler->Init(
      new flags_ui::PrefServiceFlagsStorage(g_browser_process->local_state()),
      flags_ui::kOwnerAccessToFlags);
#endif

  // Set up the about:flags source.
  content::WebUIDataSource::Add(profile, CreateFlagsUIHTMLSource());
}

FlagsUI::~FlagsUI() {
}

// static
base::RefCountedMemory* FlagsUI::GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
      IDR_FLAGS_FAVICON, scale_factor);
}
