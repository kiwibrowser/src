// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_ARC_MIDIS_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_ARC_MIDIS_CLIENT_H_

#include "chromeos/dbus/arc_midis_client.h"

namespace chromeos {

// A fake implementation of ArcMidisClient.
class CHROMEOS_EXPORT FakeArcMidisClient : public ArcMidisClient {
 public:
  FakeArcMidisClient() = default;
  ~FakeArcMidisClient() override = default;

  // DBusClient override:
  void Init(dbus::Bus* bus) override;

  // ArcMidisClient override:
  void BootstrapMojoConnection(base::ScopedFD fd,
                               VoidDBusMethodCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeArcMidisClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_ARC_MIDIS_CLIENT_H_
