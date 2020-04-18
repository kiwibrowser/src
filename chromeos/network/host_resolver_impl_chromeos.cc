// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/host_resolver_impl_chromeos.h"

#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

// HostResolverImplChromeOS::NetworkStateHandlerObserver
//
// An instance of this class is created on the NetworkHandler (UI) thread and
// manages its own lifetime, destroying itself when NetworkStateHandlerObserver
// ::OnShuttingDown() gets called.

class HostResolverImplChromeOS::NetworkObserver
    : public chromeos::NetworkStateHandlerObserver {
 public:
  static void Create(
      const base::WeakPtr<HostResolverImplChromeOS>& resolver,
      scoped_refptr<base::SingleThreadTaskRunner> resolver_task_runner,
      NetworkStateHandler* network_state_handler) {
    new NetworkObserver(resolver, resolver_task_runner, network_state_handler);
  }

  NetworkObserver(
      const base::WeakPtr<HostResolverImplChromeOS>& resolver,
      scoped_refptr<base::SingleThreadTaskRunner> resolver_task_runner,
      NetworkStateHandler* network_state_handler)
      : resolver_(resolver),
        resolver_task_runner_(resolver_task_runner),
        network_state_handler_(network_state_handler),
        weak_ptr_factory_resolver_thread_(this) {
    network_state_handler_->AddObserver(this, FROM_HERE);
    DefaultNetworkChanged(network_state_handler_->DefaultNetwork());
  }

 private:
  ~NetworkObserver() override {
    network_state_handler_->RemoveObserver(this, FROM_HERE);
  }

  // NetworkStateHandlerObserver
  void DefaultNetworkChanged(const NetworkState* network) override {
    if (!network) {
      DVLOG(2) << "DefaultNetworkChanged: No Network.";
      CallResolverSetIpAddress("", "");
      return;
    }
    std::string ipv4_address, ipv6_address;
    const DeviceState* device_state =
        network_state_handler_->GetDeviceState(network->device_path());
    if (!device_state) {
      LOG_IF(ERROR, base::SysInfo::IsRunningOnChromeOS())
          << "DefaultNetworkChanged: Network missing device: "
          << network->path();
      CallResolverSetIpAddress("", "");
      return;
    }
    for (base::DictionaryValue::Iterator iter(device_state->ip_configs());
         !iter.IsAtEnd(); iter.Advance()) {
      const base::DictionaryValue* ip_config;
      if (!iter.value().GetAsDictionary(&ip_config)) {
        LOG(ERROR) << "Badly formatted IPConfigs: " << network->path();
        continue;
      }
      std::string method, address;
      if (ip_config->GetString(shill::kMethodProperty, &method) &&
          ip_config->GetString(shill::kAddressProperty, &address)) {
        if (method == shill::kTypeIPv4 || method == shill::kTypeDHCP)
          ipv4_address = address;
        else if (method == shill::kTypeIPv6 || method == shill::kTypeDHCP6)
          ipv6_address = address;
      } else {
        LOG(ERROR) << "DefaultNetworkChanged: IPConfigs missing properties: "
                   << network->path();
      }
    }
    DVLOG(2) << "DefaultNetworkChanged: " << network->name()
             << " IPv4: " << ipv4_address << " IPv6: " << ipv6_address;
    CallResolverSetIpAddress(ipv4_address, ipv6_address);
  }

  void OnShuttingDown() override { delete this; }

  void CallResolverSetIpAddress(const std::string& ipv4_address,
                                const std::string& ipv6_address) {
    resolver_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&NetworkObserver::SetIpAddressOnResolverThread,
                       weak_ptr_factory_resolver_thread_.GetWeakPtr(),
                       ipv4_address, ipv6_address));
  }

  void SetIpAddressOnResolverThread(const std::string& ipv4_address,
                                    const std::string& ipv6_address) {
    if (resolver_)
      resolver_->SetIPAddresses(ipv4_address, ipv6_address);
  }

  base::WeakPtr<HostResolverImplChromeOS> resolver_;
  scoped_refptr<base::SingleThreadTaskRunner> resolver_task_runner_;
  NetworkStateHandler* network_state_handler_;
  base::WeakPtrFactory<NetworkObserver> weak_ptr_factory_resolver_thread_;

  DISALLOW_COPY_AND_ASSIGN(NetworkObserver);
};

// HostResolverImplChromeOS

HostResolverImplChromeOS::HostResolverImplChromeOS(
    scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner,
    NetworkStateHandler* network_state_handler,
    const Options& options,
    net::NetLog* net_log)
    : HostResolverImpl(options, net_log),
      network_handler_task_runner_(network_handler_task_runner),
      weak_ptr_factory_(this) {
  network_handler_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&NetworkObserver::Create, weak_ptr_factory_.GetWeakPtr(),
                     base::ThreadTaskRunnerHandle::Get(),
                     network_state_handler));
}

HostResolverImplChromeOS::~HostResolverImplChromeOS() = default;

int HostResolverImplChromeOS::Resolve(
    const RequestInfo& info,
    net::RequestPriority priority,
    net::AddressList* addresses,
    const net::CompletionCallback& callback,
    std::unique_ptr<Request>* out_req,
    const net::NetLogWithSource& source_net_log) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (ResolveLocalIPAddress(info, addresses))
    return net::OK;
  return net::HostResolverImpl::Resolve(
      info, priority, addresses, callback, out_req, source_net_log);
}

void HostResolverImplChromeOS::SetIPAddresses(const std::string& ipv4_address,
                                              const std::string& ipv6_address) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ipv4_address_ = ipv4_address;
  ipv6_address_ = ipv6_address;
}

bool HostResolverImplChromeOS::ResolveLocalIPAddress(
    const RequestInfo& info,
    net::AddressList* addresses) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!info.is_my_ip_address() || ipv4_address_.empty())
    return false;

  // Use IPConfig data for localhost address lookup.
  addresses->clear();

  if (info.address_family() != net::ADDRESS_FAMILY_IPV4 &&
      !ipv6_address_.empty()) {
    net::IPAddress ipv6;
    if (ipv6.AssignFromIPLiteral(ipv6_address_))
      addresses->push_back(net::IPEndPoint(ipv6, 0));
  }

  net::IPAddress ipv4;
  if (ipv4.AssignFromIPLiteral(ipv4_address_))
    addresses->push_back(net::IPEndPoint(ipv4, 0));

  DVLOG(2) << "ResolveLocalIPAddress("
           << static_cast<int>(info.address_family()) << "): "
           << addresses->size()
           << " IPv4: " << ipv4_address_ << " IPv6: " << ipv6_address_;
  addresses->SetDefaultCanonicalName();
  return true;
}

// static
std::unique_ptr<net::HostResolver>
HostResolverImplChromeOS::CreateSystemResolver(const Options& options,
                                               net::NetLog* net_log) {
  return std::unique_ptr<net::HostResolver>(new HostResolverImplChromeOS(
      NetworkHandler::Get()->task_runner(),
      NetworkHandler::Get()->network_state_handler(), options, net_log));
}

// static
std::unique_ptr<net::HostResolver>
HostResolverImplChromeOS::CreateHostResolverForTest(
    scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner,
    NetworkStateHandler* network_state_handler) {
  Options options;
  return std::unique_ptr<net::HostResolver>(new HostResolverImplChromeOS(
      network_handler_task_runner, network_state_handler, options, NULL));
}

}  // namespace chromeos
