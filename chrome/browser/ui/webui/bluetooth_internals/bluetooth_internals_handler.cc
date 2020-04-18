// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals_handler.h"

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "device/bluetooth/adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"

BluetoothInternalsHandler::BluetoothInternalsHandler(
    mojom::BluetoothInternalsHandlerRequest request)
    : binding_(this, std::move(request)), weak_ptr_factory_(this) {}

BluetoothInternalsHandler::~BluetoothInternalsHandler() {}

void BluetoothInternalsHandler::GetAdapter(GetAdapterCallback callback) {
  if (device::BluetoothAdapterFactory::IsBluetoothSupported()) {
    device::BluetoothAdapterFactory::GetAdapter(
        base::Bind(&BluetoothInternalsHandler::OnGetAdapter,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
  } else {
    std::move(callback).Run(nullptr /* AdapterPtr */);
  }
}

void BluetoothInternalsHandler::OnGetAdapter(
    GetAdapterCallback callback,
    scoped_refptr<device::BluetoothAdapter> adapter) {
  bluetooth::mojom::AdapterPtr adapter_ptr;
  mojo::MakeStrongBinding(std::make_unique<bluetooth::Adapter>(adapter),
                          mojo::MakeRequest(&adapter_ptr));
  std::move(callback).Run(std::move(adapter_ptr));
}
