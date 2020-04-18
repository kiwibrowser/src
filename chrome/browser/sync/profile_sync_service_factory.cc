// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/profile_sync_service_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/signin/about_signin_internals_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router_factory.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/undo/bookmark_undo_service_factory.h"
#include "chrome/browser/web_data_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/browser_sync/profile_sync_components_factory_impl.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/network_time/network_time_tracker.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/signin_manager_wrapper.h"
#include "components/sync/driver/startup_controller.h"
#include "components/sync/driver/sync_util.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#endif  // !defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/printing/synced_printers_manager_factory.h"
#include "components/sync_wifi/wifi_credential_syncable_service_factory.h"
#endif  // defined(OS_CHROMEOS)

using browser_sync::ProfileSyncService;

namespace {

void UpdateNetworkTimeOnUIThread(base::Time network_time,
                                 base::TimeDelta resolution,
                                 base::TimeDelta latency,
                                 base::TimeTicks post_time) {
  g_browser_process->network_time_tracker()->UpdateNetworkTime(
      network_time, resolution, latency, post_time);
}

void UpdateNetworkTime(const base::Time& network_time,
                       const base::TimeDelta& resolution,
                       const base::TimeDelta& latency) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UpdateNetworkTimeOnUIThread, network_time, resolution,
                     latency, base::TimeTicks::Now()));
}

}  // anonymous namespace

// static
ProfileSyncServiceFactory* ProfileSyncServiceFactory::GetInstance() {
  return base::Singleton<ProfileSyncServiceFactory>::get();
}

// static
ProfileSyncService* ProfileSyncServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<ProfileSyncService*>(
      GetSyncServiceForBrowserContext(profile));
}

// static
syncer::SyncService* ProfileSyncServiceFactory::GetSyncServiceForBrowserContext(
    content::BrowserContext* context) {
  if (!ProfileSyncService::IsSyncAllowedByFlag()) {
    return nullptr;
  }

  return static_cast<syncer::SyncService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ProfileSyncServiceFactory::ProfileSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "ProfileSyncService",
        BrowserContextDependencyManager::GetInstance()) {
  // The ProfileSyncService depends on various SyncableServices being around
  // when it is shut down.  Specify those dependencies here to build the proper
  // destruction order.
  DependsOn(AboutSigninInternalsFactory::GetInstance());
  DependsOn(autofill::PersonalDataManagerFactory::GetInstance());
  DependsOn(BookmarkModelFactory::GetInstance());
  DependsOn(BookmarkUndoServiceFactory::GetInstance());
  DependsOn(browser_sync::UserEventServiceFactory::GetInstance());
  DependsOn(ChromeSigninClientFactory::GetInstance());
  DependsOn(dom_distiller::DomDistillerServiceFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(gcm::GCMProfileServiceFactory::GetInstance());
#if !defined(OS_ANDROID)
  DependsOn(GlobalErrorServiceFactory::GetInstance());
  DependsOn(ThemeServiceFactory::GetInstance());
#endif  // !defined(OS_ANDROID)
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(IdentityManagerFactory::GetInstance());
  DependsOn(invalidation::ProfileInvalidationProviderFactory::GetInstance());
  DependsOn(PasswordStoreFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(SpellcheckServiceFactory::GetInstance());
#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
  DependsOn(SupervisedUserSettingsServiceFactory::GetInstance());
#endif  // BUILDFLAG(ENABLE_SUPERVISED_USERS)
  DependsOn(sync_sessions::SyncSessionsWebContentsRouterFactory::GetInstance());
  DependsOn(TemplateURLServiceFactory::GetInstance());
  DependsOn(WebDataServiceFactory::GetInstance());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
#if defined(OS_CHROMEOS)
  DependsOn(chromeos::SyncedPrintersManagerFactory::GetInstance());
  DependsOn(sync_wifi::WifiCredentialSyncableServiceFactory::GetInstance());
#endif  // defined(OS_CHROMEOS)

  // The following have not been converted to KeyedServices yet,
  // and for now they are explicitly destroyed after the
  // BrowserContextDependencyManager is told to DestroyBrowserContextServices,
  // so they will be around when the ProfileSyncService is destroyed.

  // DependsOn(FaviconServiceFactory::GetInstance());
}

ProfileSyncServiceFactory::~ProfileSyncServiceFactory() {
}

KeyedService* ProfileSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  ProfileSyncService::InitParams init_params;

  Profile* profile = Profile::FromBrowserContext(context);

  init_params.network_time_update_callback = base::Bind(&UpdateNetworkTime);
  init_params.base_directory = profile->GetPath();
  init_params.url_request_context = profile->GetRequestContext();
  init_params.debug_identifier = profile->GetDebugName();
  init_params.channel = chrome::GetChannel();

  if (!client_factory_) {
    init_params.sync_client =
        std::make_unique<browser_sync::ChromeSyncClient>(profile);
  } else {
    init_params.sync_client = client_factory_->Run(profile);
  }

  bool local_sync_backend_enabled = false;
// Since the local sync backend is currently only supported on Windows don't
// even check the pref on other os-es.
#if defined(OS_WIN)
  syncer::SyncPrefs prefs(profile->GetPrefs());
  local_sync_backend_enabled = prefs.IsLocalSyncEnabled();
  UMA_HISTOGRAM_BOOLEAN("Sync.Local.Enabled", local_sync_backend_enabled);

  if (local_sync_backend_enabled) {
    base::FilePath local_sync_backend_folder =
        init_params.sync_client->GetLocalSyncBackendFolder();

    // If the user has not specified a folder and we can't get the default
    // roaming profile location the sync service will not be created.
    UMA_HISTOGRAM_BOOLEAN("Sync.Local.RoamingProfileUnavailable",
                          local_sync_backend_folder.empty());
    if (local_sync_backend_folder.empty())
      return nullptr;

    init_params.signin_scoped_device_id_callback =
        base::BindRepeating([]() { return std::string("local_device"); });

    init_params.start_behavior = ProfileSyncService::AUTO_START;
  }
#endif  // defined(OS_WIN)

  if (!local_sync_backend_enabled) {
    // Always create the GCMProfileService instance such that we can listen to
    // the profile notifications and purge the GCM store when the profile is
    // being signed out.
    gcm::GCMProfileServiceFactory::GetForProfile(profile);

    // TODO(atwilson): Change AboutSigninInternalsFactory to load on startup
    // once http://crbug.com/171406 has been fixed.
    AboutSigninInternalsFactory::GetForProfile(profile);

    init_params.signin_wrapper = std::make_unique<SigninManagerWrapper>(
        IdentityManagerFactory::GetForProfile(profile),
        SigninManagerFactory::GetForProfile(profile));
    // Note: base::Unretained(signin_client) is safe because the SigninClient is
    // guaranteed to outlive the PSS, per a DependsOn() above (and because PSS
    // clears the callback in its Shutdown()).
    init_params.signin_scoped_device_id_callback = base::BindRepeating(
        &SigninClient::GetSigninScopedDeviceId,
        base::Unretained(ChromeSigninClientFactory::GetForProfile(profile)));
    init_params.oauth2_token_service =
        ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
    init_params.gaia_cookie_manager_service =
        GaiaCookieManagerServiceFactory::GetForProfile(profile);

    // TODO(tim): Currently, AUTO/MANUAL settings refer to the *first* time sync
    // is set up and *not* a browser restart for a manual-start platform (where
    // sync has already been set up, and should be able to start without user
    // intervention). We can get rid of the browser_default eventually, but
    // need to take care that ProfileSyncService doesn't get tripped up between
    // those two cases. Bug 88109.
    init_params.start_behavior = browser_defaults::kSyncAutoStarts
                                     ? ProfileSyncService::AUTO_START
                                     : ProfileSyncService::MANUAL_START;
  }

  auto pss = std::make_unique<ProfileSyncService>(std::move(init_params));

  // Will also initialize the sync client.
  pss->Initialize();
  return pss.release();
}

// static
bool ProfileSyncServiceFactory::HasProfileSyncService(Profile* profile) {
  return GetInstance()->GetServiceForBrowserContext(profile, false) != nullptr;
}

// static
void ProfileSyncServiceFactory::SetSyncClientFactoryForTest(
    SyncClientFactory* client_factory) {
  client_factory_ = client_factory;
}

// static
ProfileSyncServiceFactory::SyncClientFactory*
    ProfileSyncServiceFactory::client_factory_ = nullptr;
