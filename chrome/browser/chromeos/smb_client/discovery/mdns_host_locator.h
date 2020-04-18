// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_MDNS_HOST_LOCATOR_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_MDNS_HOST_LOCATOR_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/smb_client/discovery/host_locator.h"
#include "net/dns/mdns_client.h"

namespace chromeos {
namespace smb_client {

// Removes .local from |raw_hostname| if located at the end of the string and
// returns the new hostname.
Hostname RemoveLocal(const std::string& raw_hostname);

// HostLocator implementation that uses mDns to locate hosts.
class MDnsHostLocator : public HostLocator,
                        public base::SupportsWeakPtr<MDnsHostLocator> {
 public:
  MDnsHostLocator();
  ~MDnsHostLocator() override;

  // HostLocator override.
  void FindHosts(FindHostsCallback callback) override;

 private:
  // Makes the MDnsClient start listening on port 5353 on each network
  // interface.
  bool StartListening();

  // Creates a PTR transaction and finds all SMB services in the network.
  bool CreatePtrTransaction();

  // Creates an SRV transaction, which returns the hostname of |service|.
  void CreateSrvTransaction(const std::string& service);

  // Creates an A transaction, which returns the address of |raw_hostname|.
  void CreateATransaction(const std::string& raw_hostname);

  // Handler for the PTR transaction request. Returns true if the transaction
  // successfully starts.
  void OnPtrTransactionResponse(net::MDnsTransaction::Result result,
                                const net::RecordParsed* record);

  // Handler for the SRV transaction request.
  void OnSrvTransactionResponse(net::MDnsTransaction::Result result,
                                const net::RecordParsed* record);

  // Handler for the A transaction request.
  void OnATransactionResponse(const std::string& raw_hostname,
                              net::MDnsTransaction::Result result,
                              const net::RecordParsed* record);

  // Resolves services that were found through a PTR transaction request. If
  // there are no more services to be processed, this will call the
  // FindHostsCallback with the hosts found.
  void ResolveServicesFound();

  // Fires the callback if there are no more transactions left.
  void FireCallbackIfFinished();

  // Fires the callback immediately. If |success| is true, return with the hosts
  // that were found.
  void FireCallback(bool success);

  // Resets the state of the MDnsClient and resets all members to default.
  void Reset();

  // Returns the handler for the PTR transaction response.
  net::MDnsTransaction::ResultCallback GetPtrTransactionHandler();

  // Returns the handler for the SRV transaction response.
  net::MDnsTransaction::ResultCallback GetSrvTransactionHandler();

  // Returns the handler for the A transaction response.
  net::MDnsTransaction::ResultCallback GetATransactionHandler(
      const std::string& raw_hostname);

  bool running_ = false;
  uint32_t remaining_transactions_ = 0;
  FindHostsCallback callback_;
  std::vector<std::string> services_;
  HostMap results_;

  std::vector<std::unique_ptr<net::MDnsTransaction>> transactions_;
  std::unique_ptr<net::MDnsClient> mdns_client_;
  std::unique_ptr<net::MDnsSocketFactory> socket_factory_;

  DISALLOW_COPY_AND_ASSIGN(MDnsHostLocator);
};

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_DISCOVERY_MDNS_HOST_LOCATOR_H_
