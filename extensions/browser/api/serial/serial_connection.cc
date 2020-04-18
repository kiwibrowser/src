// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/serial/serial_connection.h"

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/common/api/serial.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace extensions {

namespace {

const int kDefaultBufferSize = 4096;

api::serial::SendError ConvertSendErrorFromMojo(
    device::mojom::SerialSendError input) {
  switch (input) {
    case device::mojom::SerialSendError::NONE:
      return api::serial::SEND_ERROR_NONE;
    case device::mojom::SerialSendError::DISCONNECTED:
      return api::serial::SEND_ERROR_DISCONNECTED;
    case device::mojom::SerialSendError::PENDING:
      return api::serial::SEND_ERROR_PENDING;
    case device::mojom::SerialSendError::TIMEOUT:
      return api::serial::SEND_ERROR_TIMEOUT;
    case device::mojom::SerialSendError::SYSTEM_ERROR:
      return api::serial::SEND_ERROR_SYSTEM_ERROR;
  }
  return api::serial::SEND_ERROR_NONE;
}

api::serial::ReceiveError ConvertReceiveErrorFromMojo(
    device::mojom::SerialReceiveError input) {
  switch (input) {
    case device::mojom::SerialReceiveError::NONE:
      return api::serial::RECEIVE_ERROR_NONE;
    case device::mojom::SerialReceiveError::DISCONNECTED:
      return api::serial::RECEIVE_ERROR_DISCONNECTED;
    case device::mojom::SerialReceiveError::TIMEOUT:
      return api::serial::RECEIVE_ERROR_TIMEOUT;
    case device::mojom::SerialReceiveError::DEVICE_LOST:
      return api::serial::RECEIVE_ERROR_DEVICE_LOST;
    case device::mojom::SerialReceiveError::BREAK:
      return api::serial::RECEIVE_ERROR_BREAK;
    case device::mojom::SerialReceiveError::FRAME_ERROR:
      return api::serial::RECEIVE_ERROR_FRAME_ERROR;
    case device::mojom::SerialReceiveError::OVERRUN:
      return api::serial::RECEIVE_ERROR_OVERRUN;
    case device::mojom::SerialReceiveError::BUFFER_OVERFLOW:
      return api::serial::RECEIVE_ERROR_BUFFER_OVERFLOW;
    case device::mojom::SerialReceiveError::PARITY_ERROR:
      return api::serial::RECEIVE_ERROR_PARITY_ERROR;
    case device::mojom::SerialReceiveError::SYSTEM_ERROR:
      return api::serial::RECEIVE_ERROR_SYSTEM_ERROR;
  }
  return api::serial::RECEIVE_ERROR_NONE;
}

api::serial::DataBits ConvertDataBitsFromMojo(
    device::mojom::SerialDataBits input) {
  switch (input) {
    case device::mojom::SerialDataBits::NONE:
      return api::serial::DATA_BITS_NONE;
    case device::mojom::SerialDataBits::SEVEN:
      return api::serial::DATA_BITS_SEVEN;
    case device::mojom::SerialDataBits::EIGHT:
      return api::serial::DATA_BITS_EIGHT;
  }
  return api::serial::DATA_BITS_NONE;
}

device::mojom::SerialDataBits ConvertDataBitsToMojo(
    api::serial::DataBits input) {
  switch (input) {
    case api::serial::DATA_BITS_NONE:
      return device::mojom::SerialDataBits::NONE;
    case api::serial::DATA_BITS_SEVEN:
      return device::mojom::SerialDataBits::SEVEN;
    case api::serial::DATA_BITS_EIGHT:
      return device::mojom::SerialDataBits::EIGHT;
  }
  return device::mojom::SerialDataBits::NONE;
}

api::serial::ParityBit ConvertParityBitFromMojo(
    device::mojom::SerialParityBit input) {
  switch (input) {
    case device::mojom::SerialParityBit::NONE:
      return api::serial::PARITY_BIT_NONE;
    case device::mojom::SerialParityBit::ODD:
      return api::serial::PARITY_BIT_ODD;
    case device::mojom::SerialParityBit::NO_PARITY:
      return api::serial::PARITY_BIT_NO;
    case device::mojom::SerialParityBit::EVEN:
      return api::serial::PARITY_BIT_EVEN;
  }
  return api::serial::PARITY_BIT_NONE;
}

device::mojom::SerialParityBit ConvertParityBitToMojo(
    api::serial::ParityBit input) {
  switch (input) {
    case api::serial::PARITY_BIT_NONE:
      return device::mojom::SerialParityBit::NONE;
    case api::serial::PARITY_BIT_NO:
      return device::mojom::SerialParityBit::NO_PARITY;
    case api::serial::PARITY_BIT_ODD:
      return device::mojom::SerialParityBit::ODD;
    case api::serial::PARITY_BIT_EVEN:
      return device::mojom::SerialParityBit::EVEN;
  }
  return device::mojom::SerialParityBit::NONE;
}

api::serial::StopBits ConvertStopBitsFromMojo(
    device::mojom::SerialStopBits input) {
  switch (input) {
    case device::mojom::SerialStopBits::NONE:
      return api::serial::STOP_BITS_NONE;
    case device::mojom::SerialStopBits::ONE:
      return api::serial::STOP_BITS_ONE;
    case device::mojom::SerialStopBits::TWO:
      return api::serial::STOP_BITS_TWO;
  }
  return api::serial::STOP_BITS_NONE;
}

device::mojom::SerialStopBits ConvertStopBitsToMojo(
    api::serial::StopBits input) {
  switch (input) {
    case api::serial::STOP_BITS_NONE:
      return device::mojom::SerialStopBits::NONE;
    case api::serial::STOP_BITS_ONE:
      return device::mojom::SerialStopBits::ONE;
    case api::serial::STOP_BITS_TWO:
      return device::mojom::SerialStopBits::TWO;
  }
  return device::mojom::SerialStopBits::NONE;
}

}  // namespace

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<SerialConnection>>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<SerialConnection> >*
ApiResourceManager<SerialConnection>::GetFactoryInstance() {
  return g_factory.Pointer();
}

SerialConnection::SerialConnection(
    const std::string& port,
    const std::string& owner_extension_id,
    device::mojom::SerialIoHandlerPtrInfo io_handler_info)
    : ApiResource(owner_extension_id),
      port_(port),
      persistent_(false),
      buffer_size_(kDefaultBufferSize),
      receive_timeout_(0),
      send_timeout_(0),
      paused_(false),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_info.is_valid());
  io_handler_.Bind(std::move(io_handler_info));
  io_handler_.set_connection_error_handler(base::BindOnce(
      &SerialConnection::OnConnectionError, base::Unretained(this)));
}

SerialConnection::~SerialConnection() {
  io_handler_->CancelRead(device::mojom::SerialReceiveError::DISCONNECTED);
  io_handler_->CancelWrite(device::mojom::SerialSendError::DISCONNECTED);
}

bool SerialConnection::IsPersistent() const {
  return persistent();
}

void SerialConnection::set_buffer_size(int buffer_size) {
  buffer_size_ = buffer_size;
}

void SerialConnection::set_receive_timeout(int receive_timeout) {
  receive_timeout_ = receive_timeout;
}

void SerialConnection::set_send_timeout(int send_timeout) {
  send_timeout_ = send_timeout;
}

void SerialConnection::set_paused(bool paused) {
  DCHECK(io_handler_);
  paused_ = paused;
  if (paused) {
    io_handler_->CancelRead(device::mojom::SerialReceiveError::NONE);
  }
}

void SerialConnection::set_connection_error_handler(
    base::OnceClosure connection_error_handler) {
  if (io_handler_.encountered_error()) {
    // Already being disconnected, run client's error handler immediatelly.
    std::move(connection_error_handler).Run();
    return;
  }
  connection_error_handler_ = std::move(connection_error_handler);
}

void SerialConnection::Open(const api::serial::ConnectionOptions& options,
                            OpenCompleteCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_);

  if (options.persistent.get())
    set_persistent(*options.persistent);
  if (options.name.get())
    set_name(*options.name);
  if (options.buffer_size.get())
    set_buffer_size(*options.buffer_size);
  if (options.receive_timeout.get())
    set_receive_timeout(*options.receive_timeout);
  if (options.send_timeout.get())
    set_send_timeout(*options.send_timeout);
  io_handler_->Open(
      port_, device::mojom::SerialConnectionOptions::From(options),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
}

bool SerialConnection::Receive(ReceiveCompleteCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (receive_complete_)
    return false;
  DCHECK(io_handler_);
  receive_complete_ = std::move(callback);
  io_handler_->Read(buffer_size_,
                    mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                        base::BindOnce(&SerialConnection::OnAsyncReadComplete,
                                       weak_factory_.GetWeakPtr()),
                        std::vector<uint8_t>(),
                        device::mojom::SerialReceiveError::DISCONNECTED));
  receive_timeout_task_.Cancel();
  if (receive_timeout_ > 0) {
    receive_timeout_task_.Reset(base::Bind(&SerialConnection::OnReceiveTimeout,
                                           weak_factory_.GetWeakPtr()));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, receive_timeout_task_.callback(),
        base::TimeDelta::FromMilliseconds(receive_timeout_));
  }
  return true;
}

bool SerialConnection::Send(const std::vector<char>& data,
                            SendCompleteCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (send_complete_)
    return false;
  DCHECK(io_handler_);
  send_complete_ = std::move(callback);
  io_handler_->Write(
      std::vector<uint8_t>(data.data(), data.data() + data.size()),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&SerialConnection::OnAsyncWriteComplete,
                         weak_factory_.GetWeakPtr()),
          static_cast<uint32_t>(0),
          device::mojom::SerialSendError::DISCONNECTED));
  send_timeout_task_.Cancel();
  if (send_timeout_ > 0) {
    send_timeout_task_.Reset(base::Bind(&SerialConnection::OnSendTimeout,
                                        weak_factory_.GetWeakPtr()));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, send_timeout_task_.callback(),
        base::TimeDelta::FromMilliseconds(send_timeout_));
  }
  return true;
}

void SerialConnection::Configure(const api::serial::ConnectionOptions& options,
                                 ConfigureCompleteCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_);
  if (options.persistent.get())
    set_persistent(*options.persistent);
  if (options.name.get())
    set_name(*options.name);
  if (options.buffer_size.get())
    set_buffer_size(*options.buffer_size);
  if (options.receive_timeout.get())
    set_receive_timeout(*options.receive_timeout);
  if (options.send_timeout.get())
    set_send_timeout(*options.send_timeout);
  io_handler_->ConfigurePort(
      device::mojom::SerialConnectionOptions::From(options),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
  io_handler_->CancelRead(device::mojom::SerialReceiveError::NONE);
}

void SerialConnection::GetInfo(GetInfoCompleteCallback callback) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_);

  auto info = std::make_unique<api::serial::ConnectionInfo>();
  info->paused = paused_;
  info->persistent = persistent_;
  info->name = name_;
  info->buffer_size = buffer_size_;
  info->receive_timeout = receive_timeout_;
  info->send_timeout = send_timeout_;

  auto resp_callback = base::BindOnce(
      [](GetInfoCompleteCallback callback,
         std::unique_ptr<api::serial::ConnectionInfo> info,
         device::mojom::SerialConnectionInfoPtr port_info) {
        if (!port_info) {
          // Even without remote port info, return partial info and indicate
          // that it's not complete info.
          std::move(callback).Run(false, std::move(info));
          return;
        }
        info->bitrate.reset(new int(port_info->bitrate));
        info->data_bits = ConvertDataBitsFromMojo(port_info->data_bits);
        info->parity_bit = ConvertParityBitFromMojo(port_info->parity_bit);
        info->stop_bits = ConvertStopBitsFromMojo(port_info->stop_bits);
        info->cts_flow_control.reset(new bool(port_info->cts_flow_control));
        std::move(callback).Run(true, std::move(info));
      },
      std::move(callback), std::move(info));
  io_handler_->GetPortInfo(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(resp_callback), nullptr));
}

void SerialConnection::Flush(FlushCompleteCallback callback) const {
  DCHECK(io_handler_);
  return io_handler_->Flush(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
}

void SerialConnection::GetControlSignals(
    GetControlSignalsCompleteCallback callback) const {
  DCHECK(io_handler_);
  auto resp_callback = base::BindOnce(
      [](GetControlSignalsCompleteCallback callback,
         device::mojom::SerialDeviceControlSignalsPtr signals) {
        if (!signals) {
          std::move(callback).Run(nullptr);
          return;
        }
        auto control_signals =
            std::make_unique<api::serial::DeviceControlSignals>();
        control_signals->dcd = signals->dcd;
        control_signals->cts = signals->cts;
        control_signals->ri = signals->ri;
        control_signals->dsr = signals->dsr;
        std::move(callback).Run(std::move(control_signals));
      },
      std::move(callback));
  io_handler_->GetControlSignals(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(resp_callback), nullptr));
}

void SerialConnection::SetControlSignals(
    const api::serial::HostControlSignals& control_signals,
    SetControlSignalsCompleteCallback callback) {
  DCHECK(io_handler_);
  io_handler_->SetControlSignals(
      device::mojom::SerialHostControlSignals::From(control_signals),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
}

void SerialConnection::SetBreak(SetBreakCompleteCallback callback) {
  DCHECK(io_handler_);
  io_handler_->SetBreak(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
}

void SerialConnection::ClearBreak(ClearBreakCompleteCallback callback) {
  DCHECK(io_handler_);
  io_handler_->ClearBreak(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), false));
}

void SerialConnection::OnReceiveTimeout() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_);
  io_handler_->CancelRead(device::mojom::SerialReceiveError::TIMEOUT);
}

void SerialConnection::OnSendTimeout() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_handler_);
  io_handler_->CancelWrite(device::mojom::SerialSendError::TIMEOUT);
}

void SerialConnection::OnAsyncReadComplete(
    const std::vector<uint8_t>& data_read,
    device::mojom::SerialReceiveError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(receive_complete_);
  receive_timeout_task_.Cancel();
  std::move(receive_complete_)
      .Run(std::vector<char>(data_read.data(),
                             data_read.data() + data_read.size()),
           ConvertReceiveErrorFromMojo(error));
}

void SerialConnection::OnAsyncWriteComplete(
    uint32_t bytes_sent,
    device::mojom::SerialSendError error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(send_complete_);
  send_timeout_task_.Cancel();
  std::move(send_complete_).Run(bytes_sent, ConvertSendErrorFromMojo(error));
}

void SerialConnection::OnConnectionError() {
  // Run client's error handler if existing.
  if (connection_error_handler_) {
    std::move(connection_error_handler_).Run();
  }
}

}  // namespace extensions

namespace mojo {

// static
device::mojom::SerialHostControlSignalsPtr
TypeConverter<device::mojom::SerialHostControlSignalsPtr,
              extensions::api::serial::HostControlSignals>::
    Convert(const extensions::api::serial::HostControlSignals& input) {
  device::mojom::SerialHostControlSignalsPtr output(
      device::mojom::SerialHostControlSignals::New());
  if (input.dtr.get()) {
    output->has_dtr = true;
    output->dtr = *input.dtr;
  }
  if (input.rts.get()) {
    output->has_rts = true;
    output->rts = *input.rts;
  }
  return output;
}

// static
device::mojom::SerialConnectionOptionsPtr
TypeConverter<device::mojom::SerialConnectionOptionsPtr,
              extensions::api::serial::ConnectionOptions>::
    Convert(const extensions::api::serial::ConnectionOptions& input) {
  device::mojom::SerialConnectionOptionsPtr output(
      device::mojom::SerialConnectionOptions::New());
  if (input.bitrate.get() && *input.bitrate > 0)
    output->bitrate = *input.bitrate;
  output->data_bits = extensions::ConvertDataBitsToMojo(input.data_bits);
  output->parity_bit = extensions::ConvertParityBitToMojo(input.parity_bit);
  output->stop_bits = extensions::ConvertStopBitsToMojo(input.stop_bits);
  if (input.cts_flow_control.get()) {
    output->has_cts_flow_control = true;
    output->cts_flow_control = *input.cts_flow_control;
  }
  return output;
}

}  // namespace mojo
