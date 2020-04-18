// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_config_monitor.h"

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif  // defined(OS_CHROMEOS)

ProxyConfigMonitor::ProxyConfigMonitor(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(profile);

// If this is the ChromeOS sign-in profile, just create the tracker from global
// state.
#if defined(OS_CHROMEOS)
  if (chromeos::ProfileHelper::IsSigninProfile(profile)) {
    pref_proxy_config_tracker_.reset(
        ProxyServiceFactory::CreatePrefProxyConfigTrackerOfLocalState(
            g_browser_process->local_state()));
  }
#endif  // defined(OS_CHROMEOS)

  if (!pref_proxy_config_tracker_) {
    pref_proxy_config_tracker_.reset(
        ProxyServiceFactory::CreatePrefProxyConfigTrackerOfProfile(
            profile->GetPrefs(), g_browser_process->local_state()));
  }

  proxy_config_service_ = ProxyServiceFactory::CreateProxyConfigService(
      pref_proxy_config_tracker_.get());

  proxy_config_service_->AddObserver(this);
}

ProxyConfigMonitor::ProxyConfigMonitor() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  pref_proxy_config_tracker_.reset(
      ProxyServiceFactory::CreatePrefProxyConfigTrackerOfLocalState(
          g_browser_process->local_state()));

  proxy_config_service_ = ProxyServiceFactory::CreateProxyConfigService(
      pref_proxy_config_tracker_.get());

  proxy_config_service_->AddObserver(this);
}

ProxyConfigMonitor::~ProxyConfigMonitor() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  proxy_config_service_->RemoveObserver(this);
  pref_proxy_config_tracker_->DetachFromPrefService();
}

void ProxyConfigMonitor::AddToNetworkContextParams(
    network::mojom::NetworkContextParams* network_context_params) {
  network::mojom::ProxyConfigClientPtr proxy_config_client;
  network_context_params->proxy_config_client_request =
      mojo::MakeRequest(&proxy_config_client);
  proxy_config_client_set_.AddPtr(std::move(proxy_config_client));

  binding_set_.AddBinding(
      this,
      mojo::MakeRequest(&network_context_params->proxy_config_poller_client));
  net::ProxyConfigWithAnnotation proxy_config;
  net::ProxyConfigService::ConfigAvailability availability =
      proxy_config_service_->GetLatestProxyConfig(&proxy_config);
  if (availability != net::ProxyConfigService::CONFIG_PENDING)
    network_context_params->initial_proxy_config = proxy_config;
}

void ProxyConfigMonitor::FlushForTesting() {
  proxy_config_client_set_.FlushForTesting();
}

void ProxyConfigMonitor::OnProxyConfigChanged(
    const net::ProxyConfigWithAnnotation& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  proxy_config_client_set_.ForAllPtrs(
      [config,
       availability](network::mojom::ProxyConfigClient* proxy_config_client) {
        switch (availability) {
          case net::ProxyConfigService::CONFIG_VALID:
            proxy_config_client->OnProxyConfigUpdated(config);
            break;
          case net::ProxyConfigService::CONFIG_UNSET:
            proxy_config_client->OnProxyConfigUpdated(
                net::ProxyConfigWithAnnotation::CreateDirect());
            break;
          case net::ProxyConfigService::CONFIG_PENDING:
            NOTREACHED();
            break;
        }
      });
}

void ProxyConfigMonitor::OnLazyProxyConfigPoll() {
  proxy_config_service_->OnLazyPoll();
}
