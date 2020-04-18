// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_discovery_base.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_discovery_session.h"

namespace device {

FidoBleDiscoveryBase::FidoBleDiscoveryBase()
    : FidoDiscovery(FidoTransportProtocol::kBluetoothLowEnergy),
      weak_factory_(this) {}

FidoBleDiscoveryBase::~FidoBleDiscoveryBase() {
  if (adapter_)
    adapter_->RemoveObserver(this);

  // Destroying |discovery_session_| will best-effort-stop discovering.
}

void FidoBleDiscoveryBase::OnStartDiscoverySessionWithFilter(
    std::unique_ptr<BluetoothDiscoverySession> session) {
  SetDiscoverySession(std::move(session));
  DVLOG(2) << "Discovery session started.";
  NotifyDiscoveryStarted(true);
}

void FidoBleDiscoveryBase::OnSetPoweredError() {
  DLOG(ERROR) << "Failed to power on the adapter.";
  NotifyDiscoveryStarted(false);
}

void FidoBleDiscoveryBase::OnStartDiscoverySessionError() {
  DLOG(ERROR) << "Discovery session not started.";
  NotifyDiscoveryStarted(false);
}

void FidoBleDiscoveryBase::SetDiscoverySession(
    std::unique_ptr<BluetoothDiscoverySession> discovery_session) {
  discovery_session_ = std::move(discovery_session);
}

void FidoBleDiscoveryBase::OnGetAdapter(
    scoped_refptr<BluetoothAdapter> adapter) {
  DCHECK(!adapter_);
  adapter_ = std::move(adapter);
  DCHECK(adapter_);
  VLOG(2) << "Got adapter " << adapter_->GetAddress();

  adapter_->AddObserver(this);
  if (adapter_->IsPowered()) {
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true,
        base::AdaptCallbackForRepeating(base::BindOnce(
            &FidoBleDiscoveryBase::OnSetPowered, weak_factory_.GetWeakPtr())),
        base::AdaptCallbackForRepeating(
            base::BindOnce(&FidoBleDiscoveryBase::OnSetPoweredError,
                           weak_factory_.GetWeakPtr())));
  }
}

void FidoBleDiscoveryBase::StartInternal() {
  auto& factory = BluetoothAdapterFactory::Get();
  auto callback = base::BindOnce(&FidoBleDiscoveryBase::OnGetAdapter,
                                 weak_factory_.GetWeakPtr());
#if defined(OS_MACOSX)
  // BluetoothAdapter may invoke the callback synchronously on Mac, but
  // StartInternal() never wants to invoke to NotifyDiscoveryStarted()
  // immediately, so ensure there is at least post-task at this bottleneck.
  // See: https://crbug.com/823686.
  callback = base::BindOnce(
      [](BluetoothAdapterFactory::AdapterCallback callback,
         scoped_refptr<BluetoothAdapter> adapter) {
        base::ThreadTaskRunnerHandle::Get()->PostTask(
            FROM_HERE, base::BindOnce(std::move(callback), adapter));
      },
      base::AdaptCallbackForRepeating(std::move(callback)));
#endif  // defined(OS_MACOSX)
  factory.GetAdapter(base::AdaptCallbackForRepeating(std::move(callback)));
}

}  // namespace device
