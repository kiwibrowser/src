// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/chrome_sync_client.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/syslog_logging.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sync/glue/sync_start_util.h"
#include "chrome/browser/sync/glue/theme_data_type_controller.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router.h"
#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router_factory.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/sync/browser_synced_window_delegates_getter.h"
#include "chrome/browser/undo/bookmark_undo_service_factory.h"
#include "chrome/browser/web_data_service_factory.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/autofill/core/browser/webdata/autocomplete_sync_bridge.h"
#include "components/autofill/core/browser/webdata/autofill_profile_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_wallet_metadata_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_wallet_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/browser_sync/profile_sync_components_factory_impl.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "components/history/core/browser/history_model_worker.h"
#include "components/history/core/browser/history_service.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/sync/browser/password_model_worker.h"
#include "components/search_engines/search_engine_data_type_controller.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/sync/base/pref_names.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/driver/async_directory_type_controller.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync/driver/sync_util.h"
#include "components/sync/engine/passive_model_worker.h"
#include "components/sync/engine/sequenced_model_worker.h"
#include "components/sync/engine/ui_model_worker.h"
#include "components/sync/user_events/user_event_service.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_sessions/favicon_cache.h"
#include "components/sync_sessions/session_sync_bridge.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/base/device_form_factor.h"

#if BUILDFLAG(ENABLE_APP_LIST)
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service_factory.h"
#endif  // BUILDFLAG(ENABLE_APP_LIST)

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/api/storage/settings_sync_util.h"
#include "chrome/browser/extensions/extension_sync_service.h"
#include "chrome/browser/sync/glue/extension_data_type_controller.h"
#include "chrome/browser/sync/glue/extension_setting_data_type_controller.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_sync_data_type_controller.h"
#include "chrome/browser/supervised_user/supervised_user_whitelist_service.h"
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_service.h"
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)

#if defined(OS_ANDROID)
#include "chrome/browser/sync/glue/synced_window_delegates_getter_android.h"
#endif  // defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/printing/printers_sync_bridge.h"
#include "chrome/browser/chromeos/printing/synced_printers_manager.h"
#include "chrome/browser/chromeos/printing/synced_printers_manager_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_package_sync_data_type_controller.h"
#include "chrome/browser/ui/app_list/arc/arc_package_syncable_service.h"
#include "components/sync_wifi/wifi_credential_syncable_service.h"
#include "components/sync_wifi/wifi_credential_syncable_service_factory.h"
#endif  // defined(OS_CHROMEOS)

using content::BrowserThread;
#if BUILDFLAG(ENABLE_EXTENSIONS)
using browser_sync::ExtensionDataTypeController;
using browser_sync::ExtensionSettingDataTypeController;
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
using browser_sync::SearchEngineDataTypeController;
using syncer::AsyncDirectoryTypeController;

namespace browser_sync {

namespace {
#if defined(OS_WIN)
const base::FilePath::CharType kLoopbackServerBackendFilename[] =
    FILE_PATH_LITERAL("profile.pb");
#endif  // defined(OS_WIN)
}  // namespace

// Chrome implementation of SyncSessionsClient. Needs to be in a separate class
// due to possible multiple inheritance issues, wherein ChromeSyncClient might
// inherit from other interfaces with same methods.
class SyncSessionsClientImpl : public sync_sessions::SyncSessionsClient {
 public:
  explicit SyncSessionsClientImpl(Profile* profile) : profile_(profile) {
    window_delegates_getter_.reset(
#if defined(OS_ANDROID)
        // Android doesn't have multi-profile support, so no need to pass the
        // profile in.
        new SyncedWindowDelegatesGetterAndroid());
#else   // defined(OS_ANDROID)
        new browser_sync::BrowserSyncedWindowDelegatesGetter(profile));
#endif  // defined(OS_ANDROID)
  }
  ~SyncSessionsClientImpl() override {}

  // SyncSessionsClient implementation.
  favicon::FaviconService* GetFaviconService() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return FaviconServiceFactory::GetForProfile(
        profile_, ServiceAccessType::IMPLICIT_ACCESS);
  }
  history::HistoryService* GetHistoryService() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return HistoryServiceFactory::GetForProfile(
        profile_, ServiceAccessType::EXPLICIT_ACCESS);
  }
  bool ShouldSyncURL(const GURL& url) const override {
    if (url == chrome::kChromeUIHistoryURL) {
      // The history page is treated specially as we want it to trigger syncable
      // events for UI purposes.
      return true;
    }
    return url.is_valid() && !url.SchemeIs(content::kChromeUIScheme) &&
           !url.SchemeIs(chrome::kChromeNativeScheme) && !url.SchemeIsFile();
  }

  sync_sessions::SyncedWindowDelegatesGetter* GetSyncedWindowDelegatesGetter()
      override {
    return window_delegates_getter_.get();
  }

  sync_sessions::LocalSessionEventRouter* GetLocalSessionEventRouter()
      override {
    syncer::SyncableService::StartSyncFlare flare(
        sync_start_util::GetFlareForSyncableService(profile_->GetPath()));
    sync_sessions::SyncSessionsWebContentsRouter* router =
        sync_sessions::SyncSessionsWebContentsRouterFactory::GetForProfile(
            profile_);
    router->InjectStartSyncFlare(flare);
    return router;
  }

 private:
  Profile* profile_;
  std::unique_ptr<sync_sessions::SyncedWindowDelegatesGetter>
      window_delegates_getter_;

  DISALLOW_COPY_AND_ASSIGN(SyncSessionsClientImpl);
};

ChromeSyncClient::ChromeSyncClient(Profile* profile)
    : profile_(profile),
      sync_sessions_client_(new SyncSessionsClientImpl(profile)),
      weak_ptr_factory_(this) {}

ChromeSyncClient::~ChromeSyncClient() {
}

void ChromeSyncClient::Initialize() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  web_data_service_ = WebDataServiceFactory::GetAutofillWebDataForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);
  db_thread_ =
      web_data_service_ ? web_data_service_->GetDBTaskRunner() : nullptr;
  password_store_ = PasswordStoreFactory::GetForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);

  // Component factory may already be set in tests.
  if (!GetSyncApiComponentFactory()) {
    component_factory_ = std::make_unique<ProfileSyncComponentsFactoryImpl>(
        this, chrome::GetChannel(), chrome::GetVersionString(),
        ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_TABLET,
        *base::CommandLine::ForCurrentProcess(),
        prefs::kSavingBrowserHistoryDisabled,
        content::BrowserThread::GetTaskRunnerForThread(
            content::BrowserThread::UI),
        db_thread_, web_data_service_, password_store_);
  }
}

syncer::SyncService* ChromeSyncClient::GetSyncService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return ProfileSyncServiceFactory::GetSyncServiceForBrowserContext(profile_);
}

PrefService* ChromeSyncClient::GetPrefService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return profile_->GetPrefs();
}

base::FilePath ChromeSyncClient::GetLocalSyncBackendFolder() {
  base::FilePath local_sync_backend_folder =
      GetPrefService()->GetFilePath(syncer::prefs::kLocalSyncBackendDir);

#if defined(OS_WIN)
  if (local_sync_backend_folder.empty()) {
    if (!base::PathService::Get(chrome::DIR_ROAMING_USER_DATA,
                                &local_sync_backend_folder)) {
      SYSLOG(WARNING) << "Local sync can not get the roaming profile folder.";
      return base::FilePath();
    }
  }

  // This code as it is now will assume the same profile order is present on
  // all machines, which is not a given. It is to be defined if only the
  // Default profile should get this treatment or all profile as is the case
  // now.
  // TODO(pastarmovj): http://crbug.com/674928 Decide if only the Default one
  // should be considered roamed. For now the code assumes all profiles are
  // created in the same order on all machines.
  local_sync_backend_folder =
      local_sync_backend_folder.Append(profile_->GetPath().BaseName());
  local_sync_backend_folder =
      local_sync_backend_folder.Append(kLoopbackServerBackendFilename);
#endif  // defined(OS_WIN)

  return local_sync_backend_folder;
}

bookmarks::BookmarkModel* ChromeSyncClient::GetBookmarkModel() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return BookmarkModelFactory::GetForBrowserContext(profile_);
}

favicon::FaviconService* ChromeSyncClient::GetFaviconService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);
}

history::HistoryService* ChromeSyncClient::GetHistoryService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
}

bool ChromeSyncClient::HasPasswordStore() {
  return password_store_ != nullptr;
}

autofill::PersonalDataManager* ChromeSyncClient::GetPersonalDataManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return autofill::PersonalDataManagerFactory::GetForProfile(profile_);
}

base::Closure ChromeSyncClient::GetPasswordStateChangedCallback() {
  return base::Bind(
      &PasswordStoreFactory::OnPasswordsSyncedStatePotentiallyChanged,
      base::Unretained(profile_));
}

syncer::SyncApiComponentFactory::RegisterDataTypesMethod
ChromeSyncClient::GetRegisterPlatformTypesCallback() {
  return base::Bind(
#if defined(OS_ANDROID)
      &ChromeSyncClient::RegisterAndroidDataTypes,
#else
      &ChromeSyncClient::RegisterDesktopDataTypes,
#endif  // defined(OS_ANDROID)
      weak_ptr_factory_.GetWeakPtr());
}

BookmarkUndoService* ChromeSyncClient::GetBookmarkUndoServiceIfExists() {
  return BookmarkUndoServiceFactory::GetForProfileIfExists(profile_);
}

invalidation::InvalidationService* ChromeSyncClient::GetInvalidationService() {
  invalidation::ProfileInvalidationProvider* provider =
      invalidation::ProfileInvalidationProviderFactory::GetForProfile(profile_);
  if (provider)
    return provider->GetInvalidationService();
  return nullptr;
}

scoped_refptr<syncer::ExtensionsActivity>
ChromeSyncClient::GetExtensionsActivity() {
  return extensions_activity_monitor_.GetExtensionsActivity();
}

sync_sessions::SyncSessionsClient* ChromeSyncClient::GetSyncSessionsClient() {
  return sync_sessions_client_.get();
}

base::WeakPtr<syncer::SyncableService>
ChromeSyncClient::GetSyncableServiceForType(syncer::ModelType type) {
  if (!profile_) {  // For tests.
     return base::WeakPtr<syncer::SyncableService>();
  }
  switch (type) {
    case syncer::PREFERENCES:
      return PrefServiceSyncableFromProfile(profile_)
          ->GetSyncableService(syncer::PREFERENCES)
          ->AsWeakPtr();
    case syncer::PRIORITY_PREFERENCES:
      return PrefServiceSyncableFromProfile(profile_)
          ->GetSyncableService(syncer::PRIORITY_PREFERENCES)
          ->AsWeakPtr();
    case syncer::AUTOFILL_PROFILE:
    case syncer::AUTOFILL_WALLET_DATA:
    case syncer::AUTOFILL_WALLET_METADATA: {
      if (!web_data_service_)
        return base::WeakPtr<syncer::SyncableService>();
      if (type == syncer::AUTOFILL_PROFILE) {
        return autofill::AutofillProfileSyncableService::FromWebDataService(
            web_data_service_.get())->AsWeakPtr();
      } else if (type == syncer::AUTOFILL_WALLET_METADATA) {
        return autofill::AutofillWalletMetadataSyncableService::
            FromWebDataService(web_data_service_.get())->AsWeakPtr();
      }
      return autofill::AutofillWalletSyncableService::FromWebDataService(
          web_data_service_.get())->AsWeakPtr();
    }
    case syncer::SEARCH_ENGINES:
      return TemplateURLServiceFactory::GetForProfile(profile_)->AsWeakPtr();
#if BUILDFLAG(ENABLE_EXTENSIONS)
    case syncer::APPS:
    case syncer::EXTENSIONS:
      return ExtensionSyncService::Get(profile_)->AsWeakPtr();
    case syncer::APP_SETTINGS:
    case syncer::EXTENSION_SETTINGS:
      return extensions::settings_sync_util::GetSyncableService(profile_, type)
          ->AsWeakPtr();
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
#if BUILDFLAG(ENABLE_APP_LIST)
    case syncer::APP_LIST:
      return app_list::AppListSyncableServiceFactory::GetForProfile(profile_)->
          AsWeakPtr();
#endif  // BUILDFLAG(ENABLE_APP_LIST)
#if !defined(OS_ANDROID)
    case syncer::THEMES:
      return ThemeServiceFactory::GetForProfile(profile_)->
          GetThemeSyncableService()->AsWeakPtr();
#endif  // !defined(OS_ANDROID)
    case syncer::HISTORY_DELETE_DIRECTIVES: {
      history::HistoryService* history = GetHistoryService();
      return history ? history->AsWeakPtr()
                     : base::WeakPtr<history::HistoryService>();
    }
#if BUILDFLAG(ENABLE_SPELLCHECK)
    case syncer::DICTIONARY:
      return SpellcheckServiceFactory::GetForContext(profile_)->
          GetCustomDictionary()->AsWeakPtr();
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)
    case syncer::FAVICON_IMAGES:
    case syncer::FAVICON_TRACKING: {
      sync_sessions::FaviconCache* favicons =
          ProfileSyncServiceFactory::GetForProfile(profile_)->GetFaviconCache();
      return favicons ? favicons->AsWeakPtr()
                      : base::WeakPtr<syncer::SyncableService>();
    }
#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
    case syncer::SUPERVISED_USER_SETTINGS:
      return SupervisedUserSettingsServiceFactory::GetForProfile(profile_)->
          AsWeakPtr();
    case syncer::SUPERVISED_USER_WHITELISTS: {
      // Unlike other types here, ProfileSyncServiceFactory does not declare a
      // DependsOn the SupervisedUserServiceFactory (in order to avoid circular
      // dependency), which means we cannot assume it is still alive.
      SupervisedUserService* supervised_user_service =
          SupervisedUserServiceFactory::GetForProfileIfExists(profile_);
      if (supervised_user_service)
        return supervised_user_service->GetWhitelistService()->AsWeakPtr();
      return base::WeakPtr<syncer::SyncableService>();
    }
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)
    case syncer::ARTICLES: {
      dom_distiller::DomDistillerService* service =
          dom_distiller::DomDistillerServiceFactory::GetForBrowserContext(
              profile_);
      if (service)
        return service->GetSyncableService()->AsWeakPtr();
      return base::WeakPtr<syncer::SyncableService>();
    }
    case syncer::SESSIONS: {
      return ProfileSyncServiceFactory::GetForProfile(profile_)->
          GetSessionsSyncableService()->AsWeakPtr();
    }
    case syncer::PASSWORDS: {
      return password_store_.get()
                 ? password_store_->GetPasswordSyncableService()
                 : base::WeakPtr<syncer::SyncableService>();
    }
#if defined(OS_CHROMEOS)
    case syncer::WIFI_CREDENTIALS:
      return sync_wifi::WifiCredentialSyncableServiceFactory::
          GetForBrowserContext(profile_)
              ->AsWeakPtr();
    case syncer::ARC_PACKAGE:
      return arc::ArcPackageSyncableService::Get(profile_)->AsWeakPtr();
#endif  // defined(OS_CHROMEOS)
    default:
      // The following datatypes still need to be transitioned to the
      // syncer::SyncableService API:
      // Bookmarks
      NOTREACHED();
      return base::WeakPtr<syncer::SyncableService>();
  }
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
ChromeSyncClient::GetControllerDelegateForModelType(syncer::ModelType type) {
  switch (type) {
    case syncer::DEVICE_INFO:
      return ProfileSyncServiceFactory::GetForProfile(profile_)
          ->GetDeviceInfoSyncControllerDelegateOnUIThread();
    case syncer::READING_LIST:
      // Reading List is only supported on iOS at the moment.
      NOTREACHED();
      return base::WeakPtr<syncer::ModelTypeControllerDelegate>();
    case syncer::AUTOFILL:
      return autofill::AutocompleteSyncBridge::FromWebDataService(
                 web_data_service_.get())
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
#if defined(OS_CHROMEOS)
    case syncer::PRINTERS:
      return chromeos::SyncedPrintersManagerFactory::GetForBrowserContext(
                 profile_)
          ->GetSyncBridge()
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
#endif  // defined(OS_CHROMEOS)
    case syncer::TYPED_URLS: {
      // We request history service with explicit access here because this
      // codepath is executed on backend thread while HistoryServiceFactory
      // checks preference value in implicit mode and PrefService expectes calls
      // only from UI thread.
      history::HistoryService* history = HistoryServiceFactory::GetForProfile(
          profile_, ServiceAccessType::EXPLICIT_ACCESS);
      if (!history)
        return base::WeakPtr<syncer::ModelTypeControllerDelegate>();
      return history->GetTypedURLSyncBridge()
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
    }
    case syncer::USER_EVENTS:
      return browser_sync::UserEventServiceFactory::GetForProfile(profile_)
          ->GetSyncBridge()
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
    case syncer::SESSIONS: {
      return ProfileSyncServiceFactory::GetForProfile(profile_)
          ->GetSessionSyncControllerDelegateOnUIThread();
    }
    default:
      NOTREACHED();
      return base::WeakPtr<syncer::ModelTypeControllerDelegate>();
  }
}

scoped_refptr<syncer::ModelSafeWorker>
ChromeSyncClient::CreateModelWorkerForGroup(syncer::ModelSafeGroup group) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  switch (group) {
    case syncer::GROUP_DB:
      return new syncer::SequencedModelWorker(db_thread_, syncer::GROUP_DB);
    // TODO(stanisc): crbug.com/731903: Rename GROUP_FILE to reflect that it is
    // used only for app and extension settings.
    case syncer::GROUP_FILE:
#if BUILDFLAG(ENABLE_EXTENSIONS)
      return new syncer::SequencedModelWorker(
          extensions::GetBackendTaskRunner(), syncer::GROUP_FILE);
#else
      return nullptr;
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
    case syncer::GROUP_UI:
      return new syncer::UIModelWorker(
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI));
    case syncer::GROUP_PASSIVE:
      return new syncer::PassiveModelWorker();
    case syncer::GROUP_HISTORY: {
      history::HistoryService* history_service = GetHistoryService();
      if (!history_service)
        return nullptr;
      return new HistoryModelWorker(
          history_service->AsWeakPtr(),
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI));
    }
    case syncer::GROUP_PASSWORD: {
      if (!password_store_.get())
        return nullptr;
      return new PasswordModelWorker(password_store_);
    }
    default:
      return nullptr;
  }
}

syncer::SyncApiComponentFactory*
ChromeSyncClient::GetSyncApiComponentFactory() {
  return component_factory_.get();
}

void ChromeSyncClient::SetSyncApiComponentFactoryForTesting(
    std::unique_ptr<syncer::SyncApiComponentFactory> component_factory) {
  component_factory_ = std::move(component_factory);
}

// static
void ChromeSyncClient::GetDeviceInfoTrackers(
    std::vector<const syncer::DeviceInfoTracker*>* trackers) {
  DCHECK(trackers);
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  std::vector<Profile*> profile_list = profile_manager->GetLoadedProfiles();
  for (Profile* profile : profile_list) {
    const browser_sync::ProfileSyncService* profile_sync_service =
        ProfileSyncServiceFactory::GetForProfile(profile);
    if (profile_sync_service != nullptr) {
      const syncer::DeviceInfoTracker* tracker =
          profile_sync_service->GetDeviceInfoTracker();
      if (tracker != nullptr) {
        // Even when sync is disabled and/or user is signed out, a tracker will
        // still be present. It will only be missing when the ProfileSyncService
        // has not sufficiently initialized yet.
        trackers->push_back(tracker);
      }
    }
  }
}

void ChromeSyncClient::RegisterDesktopDataTypes(
    syncer::SyncService* sync_service,
    syncer::ModelTypeSet disabled_types,
    syncer::ModelTypeSet enabled_types) {
  base::Closure error_callback =
      base::Bind(&syncer::ReportUnrecoverableError, chrome::GetChannel());

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // App sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!disabled_types.Has(syncer::APPS)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ExtensionDataTypeController>(
            syncer::APPS, error_callback, this, profile_));
  }

  // Extension sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!disabled_types.Has(syncer::EXTENSIONS)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ExtensionDataTypeController>(
            syncer::EXTENSIONS, error_callback, this, profile_));
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if !defined(OS_ANDROID)
  // Theme sync is enabled by default.  Register unless explicitly disabled.
  if (!disabled_types.Has(syncer::THEMES)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ThemeDataTypeController>(error_callback, this,
                                                  profile_));
  }
#endif  // !defined(OS_ANDROID)

  // Search Engine sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!disabled_types.Has(syncer::SEARCH_ENGINES)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<SearchEngineDataTypeController>(
            error_callback, this,
            TemplateURLServiceFactory::GetForProfile(profile_)));
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Extension setting sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!disabled_types.Has(syncer::EXTENSION_SETTINGS)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ExtensionSettingDataTypeController>(
            syncer::EXTENSION_SETTINGS, error_callback, this, profile_));
  }

  // App setting sync is enabled by default.  Register unless explicitly
  // disabled.
  if (!disabled_types.Has(syncer::APP_SETTINGS)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ExtensionSettingDataTypeController>(
            syncer::APP_SETTINGS, error_callback, this, profile_));
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_APP_LIST)
  sync_service->RegisterDataTypeController(
      std::make_unique<AsyncDirectoryTypeController>(
          syncer::APP_LIST, error_callback, this, syncer::GROUP_UI,
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));
#endif  // BUILDFLAG(ENABLE_APP_LIST)

#if defined(OS_LINUX) || defined(OS_WIN)
  // Dictionary sync is enabled by default.
  if (!disabled_types.Has(syncer::DICTIONARY)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<AsyncDirectoryTypeController>(
            syncer::DICTIONARY, error_callback, this, syncer::GROUP_UI,
            BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));
  }
#endif  // defined(OS_LINUX) || defined(OS_WIN)

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
  sync_service->RegisterDataTypeController(
      std::make_unique<SupervisedUserSyncDataTypeController>(
          syncer::SUPERVISED_USER_SETTINGS, error_callback, this, profile_));
  sync_service->RegisterDataTypeController(
      std::make_unique<SupervisedUserSyncDataTypeController>(
          syncer::SUPERVISED_USER_WHITELISTS, error_callback, this, profile_));
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)

#if defined(OS_CHROMEOS)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableWifiCredentialSync) &&
      !disabled_types.Has(syncer::WIFI_CREDENTIALS)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<AsyncDirectoryTypeController>(
            syncer::WIFI_CREDENTIALS, error_callback, this, syncer::GROUP_UI,
            BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));
  }
  if (arc::IsArcAllowedForProfile(profile_)) {
    sync_service->RegisterDataTypeController(
        std::make_unique<ArcPackageSyncDataTypeController>(
            syncer::ARC_PACKAGE, error_callback, this, profile_));
  }
#endif  // defined(OS_CHROMEOS)
}

void ChromeSyncClient::RegisterAndroidDataTypes(
    syncer::SyncService* sync_service,
    syncer::ModelTypeSet disabled_types,
    syncer::ModelTypeSet enabled_types) {
  base::Closure error_callback =
      base::Bind(&syncer::ReportUnrecoverableError, chrome::GetChannel());
#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
  sync_service->RegisterDataTypeController(
      std::make_unique<SupervisedUserSyncDataTypeController>(
          syncer::SUPERVISED_USER_SETTINGS, error_callback, this, profile_));
  sync_service->RegisterDataTypeController(
      std::make_unique<SupervisedUserSyncDataTypeController>(
          syncer::SUPERVISED_USER_WHITELISTS, error_callback, this, profile_));
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)
}

}  // namespace browser_sync
