// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/profile_network_context_service.h"

#include <string>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_accept_language_settings.h"
#include "chrome/browser/net/default_network_context_params.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "net/net_buildflags.h"
#include "services/network/public/cpp/features.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#endif

ProfileNetworkContextService::ProfileNetworkContextService(Profile* profile)
    : profile_(profile), proxy_config_monitor_(profile) {
  quic_allowed_.Init(
      prefs::kQuicAllowed, profile->GetPrefs(),
      base::Bind(&ProfileNetworkContextService::DisableQuicIfNotAllowed,
                 base::Unretained(this)));
  pref_accept_language_.Init(
      prefs::kAcceptLanguages, profile->GetPrefs(),
      base::BindRepeating(&ProfileNetworkContextService::UpdateAcceptLanguage,
                          base::Unretained(this)));
  // The system context must be initialized before any other network contexts.
  // TODO(mmenke): Figure out a way to enforce this.
  g_browser_process->system_network_context_manager()->GetContext();
  DisableQuicIfNotAllowed();
}

ProfileNetworkContextService::~ProfileNetworkContextService() {}

network::mojom::NetworkContextPtr
ProfileNetworkContextService::CreateMainNetworkContext() {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    // |profile_io_data_main_network_context_| may be initialized if
    // SetUpProfileIOdataMainContext was called first.
    if (!profile_io_data_main_network_context_) {
      profile_io_data_context_request_ =
          mojo::MakeRequest(&profile_io_data_main_network_context_);
    }
    return std::move(profile_io_data_main_network_context_);
  }

  network::mojom::NetworkContextPtr network_context;
  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context), CreateMainNetworkContextParams());
  return network_context;
}

network::mojom::NetworkContextPtr
ProfileNetworkContextService::CreateNetworkContextForPartition(
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  DCHECK(base::FeatureList::IsEnabled(network::features::kNetworkService));
  network::mojom::NetworkContextPtr network_context;
  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context),
      CreateNetworkContextParams(in_memory, relative_partition_path));
  return network_context;
}

void ProfileNetworkContextService::SetUpProfileIODataMainContext(
    network::mojom::NetworkContextRequest* network_context_request,
    network::mojom::NetworkContextParamsPtr* network_context_params) {
  DCHECK(network_context_request);
  DCHECK(network_context_params);

  // This may be called either before or after CreateMainNetworkContext().
  if (!profile_io_data_context_request_.is_pending()) {
    *network_context_request =
        mojo::MakeRequest(&profile_io_data_main_network_context_);
  } else {
    *network_context_request = std::move(profile_io_data_context_request_);
  }

  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    *network_context_params = CreateMainNetworkContextParams();
    return;
  }

  // Just use default if network service is enabled, to avoid the legacy
  // in-process URLRequestContext from fighting with the NetworkService over
  // ownership of on-disk files.
  *network_context_params = network::mojom::NetworkContextParams::New();
}

void ProfileNetworkContextService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kQuicAllowed, true);
}

void ProfileNetworkContextService::DisableQuicIfNotAllowed() {
  if (!quic_allowed_.IsManaged())
    return;

  // If QUIC is allowed, do nothing (re-enabling QUIC is not supported).
  if (quic_allowed_.GetValue())
    return;

  g_browser_process->system_network_context_manager()->DisableQuic();
}

void ProfileNetworkContextService::UpdateAcceptLanguage() {
  content::BrowserContext::GetDefaultStoragePartition(profile_)
      ->GetNetworkContext()
      ->SetAcceptLanguage(ComputeAcceptLanguage());
}

std::string ProfileNetworkContextService::ComputeAcceptLanguage() const {
  return chrome_accept_language_settings::ComputeAcceptLanguageFromPref(
      pref_accept_language_.GetValue());
}

void ProfileNetworkContextService::FlushProxyConfigMonitorForTesting() {
  proxy_config_monitor_.FlushForTesting();
}

network::mojom::NetworkContextParamsPtr
ProfileNetworkContextService::CreateMainNetworkContextParams() {
  return CreateNetworkContextParams(profile_->IsOffTheRecord(),
                                    base::FilePath());
}

network::mojom::NetworkContextParamsPtr
ProfileNetworkContextService::CreateNetworkContextParams(
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  // TODO(mmenke): Set up parameters here.
  network::mojom::NetworkContextParamsPtr network_context_params =
      CreateDefaultNetworkContextParams();

  network_context_params->context_name = std::string("main");

  network_context_params->accept_language = ComputeAcceptLanguage();

  // Always enable the HTTP cache.
  network_context_params->http_cache_enabled = true;

  base::FilePath path = profile_->GetPath();
  if (!relative_partition_path.empty())
    path = path.Append(relative_partition_path);

  // Configure on-disk storage for non-OTR profiles. OTR profiles just use
  // default behavior (in memory storage, default sizes).
  PrefService* prefs = profile_->GetPrefs();
  if (!in_memory) {
    // Configure the HTTP cache path and size.
    base::FilePath base_cache_path;
    chrome::GetUserCacheDirectory(path, &base_cache_path);
    base::FilePath disk_cache_dir = prefs->GetFilePath(prefs::kDiskCacheDir);
    if (!disk_cache_dir.empty())
      base_cache_path = disk_cache_dir.Append(base_cache_path.BaseName());
    network_context_params->http_cache_path =
        base_cache_path.Append(chrome::kCacheDirname);
    network_context_params->http_cache_max_size =
        prefs->GetInteger(prefs::kDiskCacheSize);

    // Currently this just contains HttpServerProperties, but that will likely
    // change.
    network_context_params->http_server_properties_path =
        path.Append(chrome::kNetworkPersistentStateFilename);

    base::FilePath cookie_path = path;
    cookie_path = cookie_path.Append(chrome::kCookieFilename);
    network_context_params->cookie_path = cookie_path;

    base::FilePath channel_id_path = path;
    channel_id_path = channel_id_path.Append(chrome::kChannelIDFilename);
    network_context_params->channel_id_path = channel_id_path;

    if (relative_partition_path.empty()) {
      network_context_params->restore_old_session_cookies =
          profile_->ShouldRestoreOldSessionCookies();
      network_context_params->persist_session_cookies =
          profile_->ShouldPersistSessionCookies();
    } else {
      // Copy behavior of ProfileImplIOData::InitializeAppRequestContext.
      network_context_params->restore_old_session_cookies = false;
      network_context_params->persist_session_cookies = false;
    }

    network_context_params->transport_security_persister_path = path;
  }

  // NOTE(mmenke): Keep these protocol handlers and
  // ProfileIOData::SetUpJobFactoryDefaultsForBuilder in sync with
  // ProfileIOData::IsHandledProtocol().
  // TODO(mmenke): Find a better way of handling tracking supported schemes.
  network_context_params->enable_data_url_support = true;
  network_context_params->enable_file_url_support = true;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params.get());

  network_context_params->enable_certificate_reporting = true;

#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  if (prefs->FindPreference(prefs::kGSSAPILibraryName)) {
    network_context_params->gssapi_library_name =
        prefs->GetString(prefs::kGSSAPILibraryName);
  }
#endif

#if defined(OS_CHROMEOS)
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  network_context_params->allow_gssapi_library_load =
      connector->IsActiveDirectoryManaged();
#endif

  return network_context_params;
}
