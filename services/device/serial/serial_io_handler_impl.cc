// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_io_handler_impl.h"

#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "device/serial/buffer.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

// static
void SerialIoHandlerImpl::Create(
    mojom::SerialIoHandlerRequest request,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  mojo::MakeStrongBinding(std::make_unique<SerialIoHandlerImpl>(ui_task_runner),
                          std::move(request));
}

SerialIoHandlerImpl::SerialIoHandlerImpl(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : io_handler_(device::SerialIoHandler::Create(ui_task_runner)) {}

SerialIoHandlerImpl::~SerialIoHandlerImpl() = default;

void SerialIoHandlerImpl::Open(const std::string& port,
                               mojom::SerialConnectionOptionsPtr options,
                               OpenCallback callback) {
  io_handler_->Open(port, *options, std::move(callback));
}

void SerialIoHandlerImpl::Read(uint32_t bytes, ReadCallback callback) {
  auto buffer = base::MakeRefCounted<net::IOBuffer>(static_cast<size_t>(bytes));
  io_handler_->Read(std::make_unique<ReceiveBuffer>(
      buffer, bytes,
      base::BindOnce(
          [](ReadCallback callback, scoped_refptr<net::IOBuffer> buffer,
             int bytes_read, mojom::SerialReceiveError error) {
            std::move(callback).Run(
                std::vector<uint8_t>(buffer->data(),
                                     buffer->data() + bytes_read),
                error);
          },
          std::move(callback), buffer)));
}

void SerialIoHandlerImpl::Write(const std::vector<uint8_t>& data,
                                WriteCallback callback) {
  io_handler_->Write(std::make_unique<SendBuffer>(
      data, base::BindOnce(
                [](WriteCallback callback, int bytes_sent,
                   mojom::SerialSendError error) {
                  std::move(callback).Run(bytes_sent, error);
                },
                std::move(callback))));
}

void SerialIoHandlerImpl::CancelRead(mojom::SerialReceiveError reason) {
  io_handler_->CancelRead(reason);
}

void SerialIoHandlerImpl::CancelWrite(mojom::SerialSendError reason) {
  io_handler_->CancelWrite(reason);
}

void SerialIoHandlerImpl::Flush(FlushCallback callback) {
  std::move(callback).Run(io_handler_->Flush());
}

void SerialIoHandlerImpl::GetControlSignals(
    GetControlSignalsCallback callback) {
  std::move(callback).Run(io_handler_->GetControlSignals());
}

void SerialIoHandlerImpl::SetControlSignals(
    mojom::SerialHostControlSignalsPtr signals,
    SetControlSignalsCallback callback) {
  std::move(callback).Run(io_handler_->SetControlSignals(*signals));
}

void SerialIoHandlerImpl::ConfigurePort(
    mojom::SerialConnectionOptionsPtr options,
    ConfigurePortCallback callback) {
  std::move(callback).Run(io_handler_->ConfigurePort(*options));
}

void SerialIoHandlerImpl::GetPortInfo(GetPortInfoCallback callback) {
  std::move(callback).Run(io_handler_->GetPortInfo());
}

void SerialIoHandlerImpl::SetBreak(SetBreakCallback callback) {
  std::move(callback).Run(io_handler_->SetBreak());
}

void SerialIoHandlerImpl::ClearBreak(ClearBreakCallback callback) {
  std::move(callback).Run(io_handler_->ClearBreak());
}

}  // namespace device
