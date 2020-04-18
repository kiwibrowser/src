// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_ARC_OEMCRYPTO_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_ARC_OEMCRYPTO_CLIENT_H_

#include "chromeos/dbus/arc_oemcrypto_client.h"

namespace chromeos {

// A fake implementation of ArcOemCryptoClient.
class CHROMEOS_EXPORT FakeArcOemCryptoClient : public ArcOemCryptoClient {
 public:
  FakeArcOemCryptoClient();
  ~FakeArcOemCryptoClient() override;

  // DBusClient override:
  void Init(dbus::Bus* bus) override;

  // ArcOemCryptoClient override:
  void BootstrapMojoConnection(base::ScopedFD fd,
                               VoidDBusMethodCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeArcOemCryptoClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_ARC_OEMCRYPTO_CLIENT_H_
