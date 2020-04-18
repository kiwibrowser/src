// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_SERIAL_SERIAL_IO_HANDLER_IMPL_H_
#define SERVICES_DEVICE_SERIAL_SERIAL_IO_HANDLER_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "device/serial/serial_io_handler.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace device {

// TODO(leonhsl): Merge this class with SerialIoHandler if/once
// SerialIoHandler is exposed only via the Device Service.
// crbug.com/748505
// This class must be constructed and run on IO thread.
class SerialIoHandlerImpl : public mojom::SerialIoHandler {
 public:
  static void Create(
      mojom::SerialIoHandlerRequest request,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);

  explicit SerialIoHandlerImpl(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);
  ~SerialIoHandlerImpl() override;

 private:
  // mojom::SerialIoHandler methods:
  void Open(const std::string& port,
            mojom::SerialConnectionOptionsPtr options,
            OpenCallback callback) override;
  void Read(uint32_t bytes, ReadCallback callback) override;
  void Write(const std::vector<uint8_t>& data, WriteCallback callback) override;
  void CancelRead(mojom::SerialReceiveError reason) override;
  void CancelWrite(mojom::SerialSendError reason) override;
  void Flush(FlushCallback callback) override;
  void GetControlSignals(GetControlSignalsCallback callback) override;
  void SetControlSignals(mojom::SerialHostControlSignalsPtr signals,
                         SetControlSignalsCallback callback) override;
  void ConfigurePort(mojom::SerialConnectionOptionsPtr options,
                     ConfigurePortCallback callback) override;
  void GetPortInfo(GetPortInfoCallback callback) override;
  void SetBreak(SetBreakCallback callback) override;
  void ClearBreak(ClearBreakCallback callback) override;

  scoped_refptr<device::SerialIoHandler> io_handler_;

  DISALLOW_COPY_AND_ASSIGN(SerialIoHandlerImpl);
};

}  // namespace device

#endif  // SERVICES_DEVICE_SERIAL_SERIAL_IO_HANDLER_IMPL_H_
