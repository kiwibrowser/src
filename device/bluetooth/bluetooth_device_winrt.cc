// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_device_winrt.h"

#include "base/logging.h"
#include "device/bluetooth/bluetooth_adapter_winrt.h"

namespace device {

BluetoothDeviceWinrt::BluetoothDeviceWinrt(BluetoothAdapterWinrt* adapter)
    : BluetoothDevice(adapter) {}

BluetoothDeviceWinrt::~BluetoothDeviceWinrt() = default;

uint32_t BluetoothDeviceWinrt::GetBluetoothClass() const {
  NOTIMPLEMENTED();
  return 0;
}

std::string BluetoothDeviceWinrt::GetAddress() const {
  NOTIMPLEMENTED();
  return std::string();
}

BluetoothDevice::VendorIDSource BluetoothDeviceWinrt::GetVendorIDSource()
    const {
  NOTIMPLEMENTED();
  return VendorIDSource();
}

uint16_t BluetoothDeviceWinrt::GetVendorID() const {
  NOTIMPLEMENTED();
  return 0;
}

uint16_t BluetoothDeviceWinrt::GetProductID() const {
  NOTIMPLEMENTED();
  return 0;
}

uint16_t BluetoothDeviceWinrt::GetDeviceID() const {
  NOTIMPLEMENTED();
  return 0;
}

uint16_t BluetoothDeviceWinrt::GetAppearance() const {
  NOTIMPLEMENTED();
  return 0;
}

base::Optional<std::string> BluetoothDeviceWinrt::GetName() const {
  NOTIMPLEMENTED();
  return base::nullopt;
}

bool BluetoothDeviceWinrt::IsPaired() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::IsConnected() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::IsGattConnected() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::IsConnectable() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::IsConnecting() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::ExpectingPinCode() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::ExpectingPasskey() const {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothDeviceWinrt::ExpectingConfirmation() const {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothDeviceWinrt::GetConnectionInfo(
    const ConnectionInfoCallback& callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::SetConnectionLatency(
    ConnectionLatency connection_latency,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::Connect(PairingDelegate* pairing_delegate,
                                   const base::Closure& callback,
                                   const ConnectErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::SetPinCode(const std::string& pincode) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::SetPasskey(uint32_t passkey) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::ConfirmPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::RejectPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::CancelPairing() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::Disconnect(const base::Closure& callback,
                                      const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::Forget(const base::Closure& callback,
                                  const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::ConnectToService(
    const BluetoothUUID& uuid,
    const ConnectToServiceCallback& callback,
    const ConnectToServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::ConnectToServiceInsecurely(
    const device::BluetoothUUID& uuid,
    const ConnectToServiceCallback& callback,
    const ConnectToServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::CreateGattConnectionImpl() {
  NOTIMPLEMENTED();
}

void BluetoothDeviceWinrt::DisconnectGatt() {
  NOTIMPLEMENTED();
}

}  // namespace device
