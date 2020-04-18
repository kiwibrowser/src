// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PROFILE_NETWORK_CONTEXT_SERVICE_H_
#define CHROME_BROWSER_NET_PROFILE_NETWORK_CONTEXT_SERVICE_H_

#include "base/macros.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "services/network/public/mojom/network_service.mojom.h"

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

// KeyedService that initializes and provides access to the NetworkContexts for
// a Profile. This will eventually replace ProfileIOData.
class ProfileNetworkContextService : public KeyedService {
 public:
  explicit ProfileNetworkContextService(Profile* profile);
  ~ProfileNetworkContextService() override;

  // Creates the main NetworkContext for the BrowserContext.  Uses the network
  // service if enabled. Otherwise creates one that will use the IOThread's
  // NetworkService. This may be called either before or after
  // SetUpProfileIODataMainContext.
  network::mojom::NetworkContextPtr CreateMainNetworkContext();

  // Create a network context for the given |relative_parition_path|. This is
  // only used when the network service is enabled for now.
  network::mojom::NetworkContextPtr CreateNetworkContextForPartition(
      bool in_memory,
      const base::FilePath& relative_partition_path);

  // Initializes |*network_context_params| to set up the ProfileIOData's
  // main URLRequestContext and |*network_context_request| to be one end of a
  // Mojo pipe to be bound to the NetworkContext for that URLRequestContext.
  // The caller will need to send these parameters to the IOThread's in-process
  // NetworkService.
  //
  // If the network service is disabled, CreateMainNetworkContext(), which is
  // called first, will return the other end of the pipe.  In this case, all
  // requests associated with this Profile will use the associated
  // URLRequestContext (either accessed through the StoragePartition's
  // GetURLRequestContext() or directly).
  //
  // If the network service is enabled, CreateMainNetworkContext() will instead
  // return a NetworkContext vended by the network service's NetworkService
  // (Instead of the IOThread's in-process one).  In this case, the
  // ProfileIOData's URLRequest context will be configured not to use on-disk
  // storage (so as not to conflict with the network service vended context),
  // and will only be used for legacy requests that use it directly.
  void SetUpProfileIODataMainContext(
      network::mojom::NetworkContextRequest* network_context_request,
      network::mojom::NetworkContextParamsPtr* network_context_params);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Flushes all pending proxy configuration changes.
  void FlushProxyConfigMonitorForTesting();

 private:
  // Checks |quic_allowed_|, and disables QUIC if needed.
  void DisableQuicIfNotAllowed();

  // Forwards changes to |pref_accept_language_| to the NetworkContext, after
  // formatting them as appropriate.
  void UpdateAcceptLanguage();

  // Computes appropriate value of Accept-Language header based on
  // |pref_accept_language_|
  std::string ComputeAcceptLanguage() const;

  // Creates parameters for the NetworkContext. May only be called once, since
  // it initializes some class members.
  network::mojom::NetworkContextParamsPtr CreateMainNetworkContextParams();

  // Creates parameters for the NetworkContext. Use |in_memory| instead of
  // |profile_->IsOffTheRecord()| because sometimes normal profiles want off the
  // record partitions (e.g. for webview tag).
  network::mojom::NetworkContextParamsPtr CreateNetworkContextParams(
      bool in_memory,
      const base::FilePath& relative_partition_path);

  Profile* const profile_;

  ProxyConfigMonitor proxy_config_monitor_;

  // This is a NetworkContext interface that uses ProfileIOData's
  // NetworkContext. If the network service is disabled, ownership is passed to
  // StoragePartition when CreateMainNetworkContext is called.  Otherwise,
  // retains ownership, though nothing uses it after construction.
  network::mojom::NetworkContextPtr profile_io_data_main_network_context_;

  // Request corresponding to |profile_io_data_main_network_context_|. Ownership
  // is passed to ProfileIOData when SetUpProfileIODataMainContext() is called.
  network::mojom::NetworkContextRequest profile_io_data_context_request_;

  BooleanPrefMember quic_allowed_;
  StringPrefMember pref_accept_language_;

  DISALLOW_COPY_AND_ASSIGN(ProfileNetworkContextService);
};

#endif  // CHROME_BROWSER_NET_PROFILE_NETWORK_CONTEXT_SERVICE_H_
