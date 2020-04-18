// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_arc_oemcrypto_client.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

FakeArcOemCryptoClient::FakeArcOemCryptoClient() = default;

FakeArcOemCryptoClient::~FakeArcOemCryptoClient() = default;

void FakeArcOemCryptoClient::Init(dbus::Bus* bus) {}

void FakeArcOemCryptoClient::BootstrapMojoConnection(
    base::ScopedFD fd,
    VoidDBusMethodCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), false));
}

}  // namespace chromeos
