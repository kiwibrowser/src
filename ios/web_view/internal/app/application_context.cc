// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/app/application_context.h"

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/net_log/chrome_net_log.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "ios/web/public/web_thread.h"
#include "ios/web_view/cwv_web_view_features.h"
#include "ios/web_view/internal/app/web_view_io_thread.h"
#include "net/socket/client_socket_pool_manager.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace ios_web_view {

ApplicationContext* ApplicationContext::GetInstance() {
  return base::Singleton<ApplicationContext>::get();
}

ApplicationContext::ApplicationContext() {
  net_log_ = std::make_unique<net_log::ChromeNetLog>();

  SetApplicationLocale(l10n_util::GetLocaleOverride());
}

ApplicationContext::~ApplicationContext() = default;

void ApplicationContext::PreCreateThreads() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_view_io_thread_ =
      std::make_unique<WebViewIOThread>(GetLocalState(), GetNetLog());
}

void ApplicationContext::SaveState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(crbug.com/723854): Commit prefs when entering background.
  if (local_state_) {
    local_state_->CommitPendingWrite();
  }
}

void ApplicationContext::PostDestroyThreads() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Resets associated state right after actual thread is stopped as
  // WebViewIOThread::Globals cleanup happens in CleanUp on the IO
  // thread, i.e. as the thread exits its message loop.
  //
  // This is important because in various places, the WebViewIOThread
  // object being null is considered synonymous with the IO thread
  // having stopped.
  web_view_io_thread_.reset();
}

PrefService* ApplicationContext::GetLocalState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_) {
    // Register local state preferences.
    scoped_refptr<PrefRegistrySimple> pref_registry(new PrefRegistrySimple);
    flags_ui::PrefServiceFlagsStorage::RegisterPrefs(pref_registry.get());
    PrefProxyConfigTrackerImpl::RegisterPrefs(pref_registry.get());
#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)
    SigninManagerBase::RegisterPrefs(pref_registry.get());
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)

    base::FilePath local_state_path;
    base::PathService::Get(base::DIR_APP_DATA, &local_state_path);
    local_state_path =
        local_state_path.Append(FILE_PATH_LITERAL("ChromeWebView"));
    local_state_path =
        local_state_path.Append(FILE_PATH_LITERAL("Local State"));

    scoped_refptr<PersistentPrefStore> user_pref_store =
        new JsonPrefStore(std::move(local_state_path));

    PrefServiceFactory factory;
    factory.set_user_prefs(user_pref_store);
    local_state_ = factory.Create(pref_registry.get());

    int max_normal_socket_pool_count =
        net::ClientSocketPoolManager::max_sockets_per_group(
            net::HttpNetworkSession::NORMAL_SOCKET_POOL);
    int socket_count = std::max<int>(net::kDefaultMaxSocketsPerProxyServer,
                                     max_normal_socket_pool_count);
    net::ClientSocketPoolManager::set_max_sockets_per_proxy_server(
        net::HttpNetworkSession::NORMAL_SOCKET_POOL, socket_count);
  }
  return local_state_.get();
}

net::URLRequestContextGetter* ApplicationContext::GetSystemURLRequestContext() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return web_view_io_thread_->system_url_request_context_getter();
}

const std::string& ApplicationContext::GetApplicationLocale() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!application_locale_.empty());
  return application_locale_;
}

net_log::ChromeNetLog* ApplicationContext::GetNetLog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return net_log_.get();
}

WebViewIOThread* ApplicationContext::GetWebViewIOThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_view_io_thread_.get());
  return web_view_io_thread_.get();
}

void ApplicationContext::SetApplicationLocale(const std::string& locale) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  application_locale_ = locale;
  translate::TranslateDownloadManager::GetInstance()->set_application_locale(
      application_locale_);
}

}  // namespace ios_web_view
