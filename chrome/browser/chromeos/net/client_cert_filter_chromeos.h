// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_FILTER_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_FILTER_CHROMEOS_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/net/client_cert_store_chromeos.h"
#include "crypto/scoped_nss_types.h"
#include "net/cert/nss_profile_filter_chromeos.h"

namespace chromeos {

// A client certificate filter that filters by applying a
// NSSProfileFilterChromeOS.
class ClientCertFilterChromeOS : public ClientCertStoreChromeOS::CertFilter {
 public:
  // The internal NSSProfileFilterChromeOS will be initialized with the public
  // and private slot of the user with |username_hash| and with the system slot
  // if |use_system_slot| is true.
  // If |username_hash| is empty, no public and no private slot will be used.
  ClientCertFilterChromeOS(bool use_system_slot,
                           const std::string& username_hash);
  ~ClientCertFilterChromeOS() override;

  // ClientCertStoreChromeOS::CertFilter:
  bool Init(const base::Closure& callback) override;
  bool IsCertAllowed(CERTCertificate* cert) const override;

 private:
  // Called back if the system slot was retrieved asynchronously. Continues the
  // initialization.
  void GotSystemSlot(crypto::ScopedPK11Slot system_slot);

  // Called back if the private slot was retrieved asynchronously. Continues the
  // initialization.
  void GotPrivateSlot(crypto::ScopedPK11Slot private_slot);

  // If the required slots (|private_slot_| and conditionally |system_slot_|)
  // are available, initializes |nss_profile_filter_| and returns true.
  // Otherwise returns false.
  bool InitIfSlotsAvailable();

  // True once Init() was called.
  bool init_called_;

  // The callback provided to Init(), which may be null. Reset after the filter
  // is initialized.
  base::Closure init_callback_;

  bool use_system_slot_;
  std::string username_hash_;

  // Used to store the system slot, if required, for initialization. Will be
  // null after the filter is initialized.
  crypto::ScopedPK11Slot system_slot_;

  // Used to store the private slot for initialization. Will be null after the
  // filter is initialized.
  crypto::ScopedPK11Slot private_slot_;

  // If a private slot is requested but the slot, maybe null, is not obtained
  // yet, this is equal true. As long as this is true, the NSSProfileFilter will
  // not be initialized.
  bool waiting_for_private_slot_;

  net::NSSProfileFilterChromeOS nss_profile_filter_;
  base::WeakPtrFactory<ClientCertFilterChromeOS> weak_ptr_factory_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_FILTER_CHROMEOS_H_
