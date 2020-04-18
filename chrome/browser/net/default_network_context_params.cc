// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/default_network_context_params.h"

#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/certificate_transparency/ct_known_logs.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/user_agent.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

network::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams() {
  network::mojom::NetworkContextParamsPtr network_context_params =
      network::mojom::NetworkContextParams::New();

  network_context_params->enable_brotli =
      base::FeatureList::IsEnabled(features::kBrotliEncoding);

  network_context_params->user_agent = GetUserAgent();

  std::string quic_user_agent_id = chrome::GetChannelName();
  if (!quic_user_agent_id.empty())
    quic_user_agent_id.push_back(' ');
  quic_user_agent_id.append(
      version_info::GetProductNameAndVersionForUserAgent());
  quic_user_agent_id.push_back(' ');
  quic_user_agent_id.append(content::BuildOSCpuInfo());
  network_context_params->quic_user_agent_id = quic_user_agent_id;

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // TODO(eroman): Figure out why this doesn't work in single-process mode,
  // or if it does work, now.
  // Should be possible now that a private isolate is used.
  // http://crbug.com/474654
  if (!command_line.HasSwitch(switches::kWinHttpProxyResolver)) {
    if (command_line.HasSwitch(switches::kSingleProcess)) {
      LOG(ERROR) << "Cannot use V8 Proxy resolver in single process mode.";
    } else {
      network_context_params->proxy_resolver_factory =
          ChromeMojoProxyResolverFactory::CreateWithStrongBinding()
              .PassInterface();
    }
  }

  PrefService* local_state = g_browser_process->local_state();
  network_context_params->pac_quick_check_enabled =
      local_state->GetBoolean(prefs::kQuickCheckEnabled);
  network_context_params->dangerously_allow_pac_access_to_secure_urls =
      !local_state->GetBoolean(prefs::kPacHttpsUrlStrippingEnabled);

  // Use the SystemNetworkContextManager to populate and update SSL
  // configuration. The SystemNetworkContextManager is owned by the
  // BrowserProcess itself, so will only be destroyed on shutdown, at which
  // point, all NetworkContexts will be destroyed as well.
  g_browser_process->system_network_context_manager()
      ->AddSSLConfigToNetworkContextParams(network_context_params.get());

#if !defined(OS_ANDROID)
  // CT is only enabled on Desktop platforms for now.
  network_context_params->enforce_chrome_ct_policy = true;
  for (const auto& ct_log : certificate_transparency::GetKnownLogs()) {
    // TODO(rsleevi): https://crbug.com/702062 - Remove this duplication.
    network::mojom::CTLogInfoPtr log_info = network::mojom::CTLogInfo::New();
    log_info->public_key = std::string(ct_log.log_key, ct_log.log_key_length);
    log_info->name = ct_log.log_name;
    log_info->dns_api_endpoint = ct_log.log_dns_domain;
    network_context_params->ct_logs.push_back(std::move(log_info));
  }
#endif

  bool http_09_on_non_default_ports_enabled = false;
  const base::Value* value =
      g_browser_process->policy_service()
          ->GetPolicies(policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME,
                                                std::string()))
          .GetValue(policy::key::kHttp09OnNonDefaultPortsEnabled);
  if (value)
    value->GetAsBoolean(&http_09_on_non_default_ports_enabled);
  network_context_params->http_09_on_non_default_ports_enabled =
      http_09_on_non_default_ports_enabled;

  return network_context_params;
}

void RegisterNetworkContextCreationPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kQuickCheckEnabled, true);
  registry->RegisterBooleanPref(prefs::kPacHttpsUrlStrippingEnabled, true);
}
