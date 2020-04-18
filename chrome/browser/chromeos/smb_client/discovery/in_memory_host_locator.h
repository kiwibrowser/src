// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_IN_MEMORY_HOST_LOCATOR_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_IN_MEMORY_HOST_LOCATOR_H_

#include "chrome/browser/chromeos/smb_client/discovery/host_locator.h"

namespace chromeos {
namespace smb_client {

// HostLocator implementation that uses a map as the source for hosts. New hosts
// can be registered through AddHost().
class InMemoryHostLocator : public HostLocator {
 public:
  InMemoryHostLocator();
  ~InMemoryHostLocator() override;

  // Adds host with |hostname| and |address| to host_map_.
  void AddHost(const Hostname& hostname, const Address& address);

  // Adds |hosts| to host_map_;
  void AddHosts(const HostMap& hosts);

  // Removes host with |hostname| from host_map_.
  void RemoveHost(const Hostname& hostname);

  // HostLocator override.
  void FindHosts(FindHostsCallback callback) override;

 private:
  HostMap host_map_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryHostLocator);
};

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_IN_MEMORY_HOST_LOCATOR_H_
