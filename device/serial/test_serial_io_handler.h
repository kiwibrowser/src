// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SERIAL_TEST_SERIAL_IO_HANDLER_H_
#define DEVICE_SERIAL_TEST_SERIAL_IO_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "device/serial/serial_io_handler.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

class TestSerialIoHandler : public SerialIoHandler {
 public:
  TestSerialIoHandler();

  static scoped_refptr<SerialIoHandler> Create();

  // SerialIoHandler overrides.
  void Open(const std::string& port,
            const mojom::SerialConnectionOptions& options,
            OpenCompleteCallback callback) override;
  void ReadImpl() override;
  void CancelReadImpl() override;
  void WriteImpl() override;
  void CancelWriteImpl() override;
  bool ConfigurePortImpl() override;
  mojom::SerialDeviceControlSignalsPtr GetControlSignals() const override;
  mojom::SerialConnectionInfoPtr GetPortInfo() const override;
  bool Flush() const override;
  bool SetControlSignals(
      const mojom::SerialHostControlSignals& signals) override;
  bool SetBreak() override;
  bool ClearBreak() override;
  void ForceReceiveError(device::mojom::SerialReceiveError error);
  void ForceSendError(device::mojom::SerialSendError error);

  mojom::SerialConnectionInfo* connection_info() { return &info_; }
  mojom::SerialDeviceControlSignals* device_control_signals() {
    return &device_control_signals_;
  }
  bool dtr() { return dtr_; }
  bool rts() { return rts_; }
  int flushes() { return flushes_; }
  // This callback will be called when this IoHandler processes its next write,
  // instead of the normal behavior of echoing the data to reads.
  void set_send_callback(base::OnceClosure callback) {
    send_callback_ = std::move(callback);
  }

 protected:
  ~TestSerialIoHandler() override;

 private:
  bool opened_;
  mojom::SerialConnectionInfo info_;
  mojom::SerialDeviceControlSignals device_control_signals_;
  bool dtr_;
  bool rts_;
  mutable int flushes_;
  std::vector<uint8_t> buffer_;
  base::OnceClosure send_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestSerialIoHandler);
};

}  // namespace device

#endif  // DEVICE_SERIAL_TEST_SERIAL_IO_HANDLER_H_
