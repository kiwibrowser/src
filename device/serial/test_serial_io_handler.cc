// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/serial/test_serial_io_handler.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include "base/bind.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

TestSerialIoHandler::TestSerialIoHandler()
    : SerialIoHandler(NULL),
      opened_(false),
      dtr_(false),
      rts_(false),
      flushes_(0) {}

scoped_refptr<SerialIoHandler> TestSerialIoHandler::Create() {
  return scoped_refptr<SerialIoHandler>(new TestSerialIoHandler);
}

void TestSerialIoHandler::Open(const std::string& port,
                               const mojom::SerialConnectionOptions& options,
                               OpenCompleteCallback callback) {
  DCHECK(!opened_);
  opened_ = true;
  ConfigurePort(options);
  std::move(callback).Run(true);
}

void TestSerialIoHandler::ReadImpl() {
  if (!pending_read_buffer())
    return;
  if (buffer_.empty())
    return;

  size_t num_bytes =
      std::min(buffer_.size(), static_cast<size_t>(pending_read_buffer_len()));
  memcpy(pending_read_buffer(), buffer_.data(), num_bytes);
  buffer_.erase(buffer_.begin(), buffer_.begin() + num_bytes);
  ReadCompleted(static_cast<uint32_t>(num_bytes),
                mojom::SerialReceiveError::NONE);
}

void TestSerialIoHandler::CancelReadImpl() {
  ReadCompleted(0, read_cancel_reason());
}

void TestSerialIoHandler::WriteImpl() {
  if (send_callback_) {
    std::move(send_callback_).Run();
    return;
  }
  buffer_.insert(buffer_.end(), pending_write_buffer(),
                 pending_write_buffer() + pending_write_buffer_len());
  WriteCompleted(pending_write_buffer_len(), mojom::SerialSendError::NONE);
  if (pending_read_buffer())
    ReadImpl();
}

void TestSerialIoHandler::CancelWriteImpl() {
  WriteCompleted(0, write_cancel_reason());
}

bool TestSerialIoHandler::ConfigurePortImpl() {
  info_.bitrate = options().bitrate;
  info_.data_bits = options().data_bits;
  info_.parity_bit = options().parity_bit;
  info_.stop_bits = options().stop_bits;
  info_.cts_flow_control = options().cts_flow_control;
  return true;
}

mojom::SerialDeviceControlSignalsPtr TestSerialIoHandler::GetControlSignals()
    const {
  auto signals = mojom::SerialDeviceControlSignals::New();
  *signals = device_control_signals_;
  return signals;
}

mojom::SerialConnectionInfoPtr TestSerialIoHandler::GetPortInfo() const {
  auto info = mojom::SerialConnectionInfo::New();
  *info = info_;
  return info;
}

bool TestSerialIoHandler::Flush() const {
  flushes_++;
  return true;
}

bool TestSerialIoHandler::SetControlSignals(
    const mojom::SerialHostControlSignals& signals) {
  if (signals.has_dtr)
    dtr_ = signals.dtr;
  if (signals.has_rts)
    rts_ = signals.rts;
  return true;
}

bool TestSerialIoHandler::SetBreak() {
  return true;
}

bool TestSerialIoHandler::ClearBreak() {
  return true;
}

void TestSerialIoHandler::ForceReceiveError(
    device::mojom::SerialReceiveError error) {
  ReadCompleted(0, error);
}

void TestSerialIoHandler::ForceSendError(device::mojom::SerialSendError error) {
  WriteCompleted(0, error);
}

TestSerialIoHandler::~TestSerialIoHandler() = default;

}  // namespace device
