// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/ios_chrome_sync_client.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
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
#include "components/history/core/browser/typed_url_sync_bridge.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/sync/browser/password_model_worker.h"
#include "components/reading_list/core/reading_list_model.h"
#include "components/search_engines/search_engine_data_type_controller.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync/driver/sync_util.h"
#include "components/sync/engine/passive_model_worker.h"
#include "components/sync/engine/sequenced_model_worker.h"
#include "components/sync/engine/ui_model_worker.h"
#include "components/sync/user_events/user_event_service.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_sessions/favicon_cache.h"
#include "components/sync_sessions/local_session_event_router.h"
#include "components/sync_sessions/session_sync_bridge.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/invalidation/ios_chrome_profile_invalidation_provider_factory.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#include "ios/chrome/browser/sync/glue/sync_start_util.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/sessions/ios_chrome_local_session_event_router.h"
#include "ios/chrome/browser/tabs/tab_model_synced_window_delegate_getter.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/web_data_service_factory.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/web_thread.h"
#include "ui/base/device_form_factor.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// iOS implementation of SyncSessionsClient. Needs to be in a separate class
// due to possible multiple inheritance issues, wherein IOSChromeSyncClient
// might inherit from other interfaces with same methods.
class SyncSessionsClientImpl : public sync_sessions::SyncSessionsClient {
 public:
  explicit SyncSessionsClientImpl(ios::ChromeBrowserState* browser_state)
      : browser_state_(browser_state),
        window_delegates_getter_(
            std::make_unique<TabModelSyncedWindowDelegatesGetter>()),
        local_session_event_router_(
            std::make_unique<IOSChromeLocalSessionEventRouter>(
                browser_state_,
                this,
                ios::sync_start_util::GetFlareForSyncableService(
                    browser_state_->GetStatePath()))) {}

  ~SyncSessionsClientImpl() override {}

  // SyncSessionsClient implementation.
  favicon::FaviconService* GetFaviconService() override {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    return ios::FaviconServiceFactory::GetForBrowserState(
        browser_state_, ServiceAccessType::IMPLICIT_ACCESS);
  }

  history::HistoryService* GetHistoryService() override {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    return ios::HistoryServiceFactory::GetForBrowserState(
        browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
  }

  bool ShouldSyncURL(const GURL& url) const override {
    if (url == kChromeUIHistoryURL) {
      // The history page is treated specially as we want it to trigger syncable
      // events for UI purposes.
      return true;
    }
    return url.is_valid() && !url.SchemeIs(kChromeUIScheme) &&
           !url.SchemeIsFile();
  }

  sync_sessions::SyncedWindowDelegatesGetter* GetSyncedWindowDelegatesGetter()
      override {
    return window_delegates_getter_.get();
  }

  sync_sessions::LocalSessionEventRouter* GetLocalSessionEventRouter()
      override {
    return local_session_event_router_.get();
  }

 private:
  ios::ChromeBrowserState* const browser_state_;
  const std::unique_ptr<sync_sessions::SyncedWindowDelegatesGetter>
      window_delegates_getter_;
  const std::unique_ptr<IOSChromeLocalSessionEventRouter>
      local_session_event_router_;

  DISALLOW_COPY_AND_ASSIGN(SyncSessionsClientImpl);
};

}  // namespace

IOSChromeSyncClient::IOSChromeSyncClient(ios::ChromeBrowserState* browser_state)
    : browser_state_(browser_state),
      sync_sessions_client_(
          std::make_unique<SyncSessionsClientImpl>(browser_state)),
      weak_ptr_factory_(this) {}

IOSChromeSyncClient::~IOSChromeSyncClient() {}

void IOSChromeSyncClient::Initialize() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);

  web_data_service_ =
      ios::WebDataServiceFactory::GetAutofillWebDataForBrowserState(
          browser_state_, ServiceAccessType::IMPLICIT_ACCESS);
  db_thread_ =
      web_data_service_ ? web_data_service_->GetDBTaskRunner() : nullptr;
  password_store_ = IOSChromePasswordStoreFactory::GetForBrowserState(
      browser_state_, ServiceAccessType::IMPLICIT_ACCESS);

  // Component factory may already be set in tests.
  if (!GetSyncApiComponentFactory()) {
    component_factory_.reset(new browser_sync::ProfileSyncComponentsFactoryImpl(
        this, ::GetChannel(), ::GetVersionString(),
        ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_TABLET,
        *base::CommandLine::ForCurrentProcess(),
        prefs::kSavingBrowserHistoryDisabled,
        web::WebThread::GetTaskRunnerForThread(web::WebThread::UI), db_thread_,
        web_data_service_, password_store_));
  }
}

syncer::SyncService* IOSChromeSyncClient::GetSyncService() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state_);
}

PrefService* IOSChromeSyncClient::GetPrefService() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return browser_state_->GetPrefs();
}

base::FilePath IOSChromeSyncClient::GetLocalSyncBackendFolder() {
  return base::FilePath();
}

bookmarks::BookmarkModel* IOSChromeSyncClient::GetBookmarkModel() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return ios::BookmarkModelFactory::GetForBrowserState(browser_state_);
}

favicon::FaviconService* IOSChromeSyncClient::GetFaviconService() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return ios::FaviconServiceFactory::GetForBrowserState(
      browser_state_, ServiceAccessType::IMPLICIT_ACCESS);
}

history::HistoryService* IOSChromeSyncClient::GetHistoryService() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return ios::HistoryServiceFactory::GetForBrowserState(
      browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
}

bool IOSChromeSyncClient::HasPasswordStore() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return password_store_ != nullptr;
}

autofill::PersonalDataManager* IOSChromeSyncClient::GetPersonalDataManager() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return autofill::PersonalDataManagerFactory::GetForBrowserState(
      browser_state_);
}

base::Closure IOSChromeSyncClient::GetPasswordStateChangedCallback() {
  return base::Bind(
      &IOSChromePasswordStoreFactory::OnPasswordsSyncedStatePotentiallyChanged,
      base::Unretained(browser_state_));
}

syncer::SyncApiComponentFactory::RegisterDataTypesMethod
IOSChromeSyncClient::GetRegisterPlatformTypesCallback() {
  // The iOS port does not have any platform-specific datatypes.
  return syncer::SyncApiComponentFactory::RegisterDataTypesMethod();
}

BookmarkUndoService* IOSChromeSyncClient::GetBookmarkUndoServiceIfExists() {
  return ios::BookmarkUndoServiceFactory::GetForBrowserStateIfExists(
      browser_state_);
}

invalidation::InvalidationService*
IOSChromeSyncClient::GetInvalidationService() {
  invalidation::ProfileInvalidationProvider* provider =
      IOSChromeProfileInvalidationProviderFactory::GetForBrowserState(
          browser_state_);
  if (provider)
    return provider->GetInvalidationService();
  return nullptr;
}

scoped_refptr<syncer::ExtensionsActivity>
IOSChromeSyncClient::GetExtensionsActivity() {
  return nullptr;
}

sync_sessions::SyncSessionsClient*
IOSChromeSyncClient::GetSyncSessionsClient() {
  return sync_sessions_client_.get();
}

base::WeakPtr<syncer::SyncableService>
IOSChromeSyncClient::GetSyncableServiceForType(syncer::ModelType type) {
  switch (type) {
    case syncer::PREFERENCES:
      return browser_state_->GetSyncablePrefs()
          ->GetSyncableService(syncer::PREFERENCES)
          ->AsWeakPtr();
    case syncer::PRIORITY_PREFERENCES:
      return browser_state_->GetSyncablePrefs()
          ->GetSyncableService(syncer::PRIORITY_PREFERENCES)
          ->AsWeakPtr();
    case syncer::AUTOFILL_PROFILE:
    case syncer::AUTOFILL_WALLET_DATA:
    case syncer::AUTOFILL_WALLET_METADATA: {
      if (!web_data_service_)
        return base::WeakPtr<syncer::SyncableService>();
      if (type == syncer::AUTOFILL_PROFILE) {
        return autofill::AutofillProfileSyncableService::FromWebDataService(
                   web_data_service_.get())
            ->AsWeakPtr();
      } else if (type == syncer::AUTOFILL_WALLET_METADATA) {
        return autofill::AutofillWalletMetadataSyncableService::
            FromWebDataService(web_data_service_.get())
                ->AsWeakPtr();
      }
      return autofill::AutofillWalletSyncableService::FromWebDataService(
                 web_data_service_.get())
          ->AsWeakPtr();
    }
    case syncer::HISTORY_DELETE_DIRECTIVES: {
      history::HistoryService* history =
          ios::HistoryServiceFactory::GetForBrowserState(
              browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
      return history ? history->AsWeakPtr()
                     : base::WeakPtr<history::HistoryService>();
    }
    case syncer::FAVICON_IMAGES:
    case syncer::FAVICON_TRACKING: {
      sync_sessions::FaviconCache* favicons =
          IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state_)
              ->GetFaviconCache();
      return favicons ? favicons->AsWeakPtr()
                      : base::WeakPtr<syncer::SyncableService>();
    }
    case syncer::ARTICLES: {
      // DomDistillerService is used in iOS ReadingList. The distilled articles
      // are saved separately and must not be synced.
      // Add a not reached to avoid having ARTICLES sync be enabled silently.
      NOTREACHED();
      return base::WeakPtr<syncer::SyncableService>();
    }
    case syncer::SESSIONS: {
      return IOSChromeProfileSyncServiceFactory::GetForBrowserState(
                 browser_state_)
          ->GetSessionsSyncableService()
          ->AsWeakPtr();
    }
    case syncer::PASSWORDS: {
      return password_store_ ? password_store_->GetPasswordSyncableService()
                             : base::WeakPtr<syncer::SyncableService>();
    }
    default:
      NOTREACHED();
      return base::WeakPtr<syncer::SyncableService>();
  }
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
IOSChromeSyncClient::GetControllerDelegateForModelType(syncer::ModelType type) {
  switch (type) {
    case syncer::DEVICE_INFO:
      return IOSChromeProfileSyncServiceFactory::GetForBrowserState(
                 browser_state_)
          ->GetDeviceInfoSyncControllerDelegateOnUIThread();
    case syncer::READING_LIST: {
      ReadingListModel* reading_list_model =
          ReadingListModelFactory::GetForBrowserState(browser_state_);
      return reading_list_model->GetModelTypeSyncBridge()
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
    }
    case syncer::AUTOFILL:
      return autofill::AutocompleteSyncBridge::FromWebDataService(
                 web_data_service_.get())
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
    case syncer::TYPED_URLS: {
      history::HistoryService* history =
          ios::HistoryServiceFactory::GetForBrowserState(
              browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
      return history ? history->GetTypedURLSyncBridge()
                           ->change_processor()
                           ->GetControllerDelegateOnUIThread()
                     : base::WeakPtr<syncer::ModelTypeControllerDelegate>();
    }
    case syncer::USER_EVENTS:
      return IOSUserEventServiceFactory::GetForBrowserState(browser_state_)
          ->GetSyncBridge()
          ->change_processor()
          ->GetControllerDelegateOnUIThread();
    case syncer::SESSIONS:
      return IOSChromeProfileSyncServiceFactory::GetForBrowserState(
                 browser_state_)
          ->GetSessionSyncControllerDelegateOnUIThread();
    default:
      NOTREACHED();
      return base::WeakPtr<syncer::ModelTypeControllerDelegate>();
  }
}

scoped_refptr<syncer::ModelSafeWorker>
IOSChromeSyncClient::CreateModelWorkerForGroup(syncer::ModelSafeGroup group) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  switch (group) {
    case syncer::GROUP_DB:
      return new syncer::SequencedModelWorker(db_thread_, syncer::GROUP_DB);
    case syncer::GROUP_FILE:
      // Not supported on iOS.
      return nullptr;
    case syncer::GROUP_UI:
      return new syncer::UIModelWorker(
          web::WebThread::GetTaskRunnerForThread(web::WebThread::UI));
    case syncer::GROUP_PASSIVE:
      return new syncer::PassiveModelWorker();
    case syncer::GROUP_HISTORY: {
      history::HistoryService* history_service = GetHistoryService();
      if (!history_service)
        return nullptr;
      return new browser_sync::HistoryModelWorker(
          history_service->AsWeakPtr(),
          web::WebThread::GetTaskRunnerForThread(web::WebThread::UI));
    }
    case syncer::GROUP_PASSWORD: {
      if (!password_store_)
        return nullptr;
      return new browser_sync::PasswordModelWorker(password_store_);
    }
    default:
      return nullptr;
  }
}

syncer::SyncApiComponentFactory*
IOSChromeSyncClient::GetSyncApiComponentFactory() {
  return component_factory_.get();
}

void IOSChromeSyncClient::SetSyncApiComponentFactoryForTesting(
    std::unique_ptr<syncer::SyncApiComponentFactory> component_factory) {
  component_factory_ = std::move(component_factory);
}

// static
void IOSChromeSyncClient::GetDeviceInfoTrackers(
    std::vector<const syncer::DeviceInfoTracker*>* trackers) {
  DCHECK(trackers);
  std::vector<ios::ChromeBrowserState*> browser_state_list =
      GetApplicationContext()
          ->GetChromeBrowserStateManager()
          ->GetLoadedBrowserStates();
  for (ios::ChromeBrowserState* browser_state : browser_state_list) {
    browser_sync::ProfileSyncService* profile_sync_service =
        IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state);
    if (profile_sync_service != nullptr) {
      const syncer::DeviceInfoTracker* tracker =
          profile_sync_service->GetDeviceInfoTracker();
      if (tracker != nullptr) {
        trackers->push_back(tracker);
      }
    }
  }
}
