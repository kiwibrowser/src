// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/client_cert_filter_chromeos.h"

#include <utility>

#include "base/bind.h"
#include "crypto/nss_util_internal.h"

namespace chromeos {

ClientCertFilterChromeOS::ClientCertFilterChromeOS(
    bool use_system_slot,
    const std::string& username_hash)
    : init_called_(false),
      use_system_slot_(use_system_slot),
      username_hash_(username_hash),
      waiting_for_private_slot_(false),
      weak_ptr_factory_(this) {
}

ClientCertFilterChromeOS::~ClientCertFilterChromeOS() {
}

bool ClientCertFilterChromeOS::Init(const base::Closure& callback) {
  DCHECK(!init_called_);
  init_called_ = true;

  waiting_for_private_slot_ = true;

  if (use_system_slot_) {
    system_slot_ = crypto::GetSystemNSSKeySlot(
        base::Bind(&ClientCertFilterChromeOS::GotSystemSlot,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  private_slot_ = crypto::GetPrivateSlotForChromeOSUser(
      username_hash_, base::Bind(&ClientCertFilterChromeOS::GotPrivateSlot,
                                 weak_ptr_factory_.GetWeakPtr()));

  // If the returned slot is null, GotPrivateSlot will be called back
  // eventually. If it is not null, the private slot was available synchronously
  // and the callback will not be called.
  if (private_slot_)
    waiting_for_private_slot_ = false;

  // Do not call back if we initialized synchronously.
  if (InitIfSlotsAvailable())
    return true;

  init_callback_ = callback;
  return false;
}

bool ClientCertFilterChromeOS::IsCertAllowed(CERTCertificate* cert) const {
  return nss_profile_filter_.IsCertAllowed(cert);
}

void ClientCertFilterChromeOS::GotSystemSlot(
    crypto::ScopedPK11Slot system_slot) {
  system_slot_ = std::move(system_slot);
  if (InitIfSlotsAvailable() && !init_callback_.is_null()) {
    init_callback_.Run();
    init_callback_.Reset();
  }
}

void ClientCertFilterChromeOS::GotPrivateSlot(
    crypto::ScopedPK11Slot private_slot) {
  waiting_for_private_slot_ = false;
  private_slot_ = std::move(private_slot);
  if (InitIfSlotsAvailable() && !init_callback_.is_null()) {
    init_callback_.Run();
    init_callback_.Reset();
  }
}

bool ClientCertFilterChromeOS::InitIfSlotsAvailable() {
  if ((use_system_slot_ && !system_slot_) || waiting_for_private_slot_)
    return false;
  nss_profile_filter_.Init(crypto::GetPublicSlotForChromeOSUser(username_hash_),
                           std::move(private_slot_), std::move(system_slot_));
  return true;
}

}  // namespace chromeos
