// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/discovery/in_memory_host_locator.h"

#include <map>
#include <utility>

namespace chromeos {
namespace smb_client {

InMemoryHostLocator::InMemoryHostLocator() = default;
InMemoryHostLocator::~InMemoryHostLocator() = default;

void InMemoryHostLocator::AddHost(const Hostname& hostname,
                                  const Address& address) {
  host_map_[hostname] = address;
}

void InMemoryHostLocator::AddHosts(const HostMap& hosts) {
  for (const auto& host : hosts) {
    AddHost(host.first, host.second);
  }
}

void InMemoryHostLocator::RemoveHost(const Hostname& hostname) {
  host_map_.erase(hostname);
}

void InMemoryHostLocator::FindHosts(FindHostsCallback callback) {
  std::move(callback).Run(true /* success */, host_map_);
}

}  // namespace smb_client
}  // namespace chromeos
