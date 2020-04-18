// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_hid_device.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "crypto/random.h"
#include "device/fido/fido_hid_message.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace device {

namespace switches {
static constexpr char kEnableU2fHidTest[] = "enable-u2f-hid-tests";
}  // namespace switches

namespace {
// U2F devices only provide a single report so specify a report ID of 0 here.
static constexpr uint8_t kReportId = 0x00;
}  // namespace

FidoHidDevice::FidoHidDevice(device::mojom::HidDeviceInfoPtr device_info,
                             device::mojom::HidManager* hid_manager)
    : FidoDevice(),
      hid_manager_(hid_manager),
      device_info_(std::move(device_info)),
      weak_factory_(this) {}

FidoHidDevice::~FidoHidDevice() = default;

void FidoHidDevice::DeviceTransact(std::vector<uint8_t> command,
                                   DeviceCallback callback) {
  Transition(std::move(command), std::move(callback));
}

void FidoHidDevice::Cancel() {
  // If device has not been connected or is already in error state, do nothing.
  if (state_ != State::kBusy && state_ != State::kReady)
    return;

  Transition(std::vector<uint8_t>(), base::DoNothing());
}

void FidoHidDevice::Transition(std::vector<uint8_t> command,
                               DeviceCallback callback) {
  // This adapter is needed to support the calls to ArmTimeout(). However, it is
  // still guaranteed that |callback| will only be invoked once.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  switch (state_) {
    case State::kInit:
      state_ = State::kBusy;
      ArmTimeout(repeating_callback);
      Connect(base::BindOnce(&FidoHidDevice::OnConnect,
                             weak_factory_.GetWeakPtr(), std::move(command),
                             repeating_callback));
      break;
    case State::kConnected:
      state_ = State::kBusy;
      ArmTimeout(repeating_callback);
      AllocateChannel(std::move(command), repeating_callback);
      break;
    case State::kReady: {
      state_ = State::kBusy;
      ArmTimeout(repeating_callback);

      // If cancel command has been received, send HID_CANCEL with no-op
      // callback.
      // TODO(hongjunchoi): Re-factor cancel logic and consolidate it with
      // FidoBleDevice::Cancel().
      if (command.empty()) {
        WriteMessage(
            FidoHidMessage::Create(channel_id_, FidoHidDeviceCommand::kCancel,
                                   std::move(command)),
            false, base::DoNothing());
        return;
      }

      // Write message to the device.
      const auto command_type = supported_protocol() == ProtocolVersion::kCtap
                                    ? FidoHidDeviceCommand::kCbor
                                    : FidoHidDeviceCommand::kMsg;
      WriteMessage(
          FidoHidMessage::Create(channel_id_, command_type, std::move(command)),
          true,
          base::BindOnce(&FidoHidDevice::MessageReceived,
                         weak_factory_.GetWeakPtr(), repeating_callback));
      break;
    }
    case State::kBusy:
      pending_transactions_.emplace(std::move(command), repeating_callback);
      break;
    case State::kDeviceError:
    default:
      base::WeakPtr<FidoHidDevice> self = weak_factory_.GetWeakPtr();
      repeating_callback.Run(base::nullopt);
      // Executing callbacks may free |this|. Check |self| first.
      while (self && !pending_transactions_.empty()) {
        // Respond to any pending requests.
        DeviceCallback pending_cb =
            std::move(pending_transactions_.front().second);
        pending_transactions_.pop();
        std::move(pending_cb).Run(base::nullopt);
      }
      break;
  }
}

void FidoHidDevice::Connect(ConnectCallback callback) {
  DCHECK(hid_manager_);
  hid_manager_->Connect(device_info_->guid, std::move(callback));
}

void FidoHidDevice::OnConnect(std::vector<uint8_t> command,
                              DeviceCallback callback,
                              device::mojom::HidConnectionPtr connection) {
  if (state_ == State::kDeviceError)
    return;
  timeout_callback_.Cancel();

  if (connection) {
    connection_ = std::move(connection);
    state_ = State::kConnected;
  } else {
    state_ = State::kDeviceError;
  }
  Transition(std::move(command), std::move(callback));
}

void FidoHidDevice::AllocateChannel(std::vector<uint8_t> command,
                                    DeviceCallback callback) {
  // Send random nonce to device to verify received message.
  std::vector<uint8_t> nonce(8);
  crypto::RandBytes(nonce.data(), nonce.size());
  WriteMessage(
      FidoHidMessage::Create(channel_id_, FidoHidDeviceCommand::kInit, nonce),
      true,
      base::BindOnce(&FidoHidDevice::OnAllocateChannel,
                     weak_factory_.GetWeakPtr(), nonce, std::move(command),
                     std::move(callback)));
}

void FidoHidDevice::OnAllocateChannel(std::vector<uint8_t> nonce,
                                      std::vector<uint8_t> command,
                                      DeviceCallback callback,
                                      base::Optional<FidoHidMessage> message) {
  if (state_ == State::kDeviceError)
    return;

  timeout_callback_.Cancel();

  if (!message) {
    state_ = State::kDeviceError;
    Transition(std::vector<uint8_t>(), std::move(callback));
    return;
  }

  // Channel allocation response is defined as:
  // 0: 8 byte nonce
  // 8: 4 byte channel id
  // 12: Protocol version id
  // 13: Major device version
  // 14: Minor device version
  // 15: Build device version
  // 16: Capabilities
  std::vector<uint8_t> payload = message->GetMessagePayload();
  if (payload.size() != 17) {
    state_ = State::kDeviceError;
    Transition(std::vector<uint8_t>(), std::move(callback));
    return;
  }

  auto received_nonce = base::make_span(payload).first(8);
  // Received a broadcast message for a different client. Disregard and continue
  // reading.
  if (base::make_span(nonce) != received_nonce) {
    auto repeating_callback =
        base::AdaptCallbackForRepeating(std::move(callback));
    ArmTimeout(repeating_callback);
    ReadMessage(base::BindOnce(&FidoHidDevice::OnAllocateChannel,
                               weak_factory_.GetWeakPtr(), nonce,
                               std::move(command), repeating_callback));
    return;
  }

  size_t index = 8;
  channel_id_ = payload[index++] << 24;
  channel_id_ |= payload[index++] << 16;
  channel_id_ |= payload[index++] << 8;
  channel_id_ |= payload[index++];
  capabilities_ = payload[16];
  state_ = State::kReady;
  Transition(std::move(command), std::move(callback));
}

void FidoHidDevice::WriteMessage(base::Optional<FidoHidMessage> message,
                                 bool response_expected,
                                 HidMessageCallback callback) {
  if (!connection_ || !message || message->NumPackets() == 0) {
    std::move(callback).Run(base::nullopt);
    return;
  }
  const auto& packet = message->PopNextPacket();
  connection_->Write(
      kReportId, packet,
      base::BindOnce(&FidoHidDevice::PacketWritten, weak_factory_.GetWeakPtr(),
                     std::move(message), response_expected,
                     std::move(callback)));
}

void FidoHidDevice::PacketWritten(base::Optional<FidoHidMessage> message,
                                  bool response_expected,
                                  HidMessageCallback callback,
                                  bool success) {
  if (success && message->NumPackets() > 0) {
    WriteMessage(std::move(message), response_expected, std::move(callback));
  } else if (success && response_expected) {
    ReadMessage(std::move(callback));
  } else {
    std::move(callback).Run(base::nullopt);
  }
}

void FidoHidDevice::ReadMessage(HidMessageCallback callback) {
  if (!connection_) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  connection_->Read(base::BindOnce(
      &FidoHidDevice::OnRead, weak_factory_.GetWeakPtr(), std::move(callback)));
}

void FidoHidDevice::OnRead(HidMessageCallback callback,
                           bool success,
                           uint8_t report_id,
                           const base::Optional<std::vector<uint8_t>>& buf) {
  if (!success) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  DCHECK(buf);
  auto read_message = FidoHidMessage::CreateFromSerializedData(*buf);
  if (!read_message) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  // Received a message from a different channel, so try again.
  if (channel_id_ != read_message->channel_id()) {
    connection_->Read(base::BindOnce(&FidoHidDevice::OnRead,
                                     weak_factory_.GetWeakPtr(),
                                     std::move(callback)));
    return;
  }

  if (read_message->MessageComplete()) {
    std::move(callback).Run(std::move(read_message));
    return;
  }

  // Continue reading additional packets.
  connection_->Read(base::BindOnce(
      &FidoHidDevice::OnReadContinuation, weak_factory_.GetWeakPtr(),
      std::move(read_message), std::move(callback)));
}

void FidoHidDevice::OnReadContinuation(
    base::Optional<FidoHidMessage> message,
    HidMessageCallback callback,
    bool success,
    uint8_t report_id,
    const base::Optional<std::vector<uint8_t>>& buf) {
  if (!success) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  DCHECK(buf);
  message->AddContinuationPacket(*buf);
  if (message->MessageComplete()) {
    std::move(callback).Run(std::move(message));
    return;
  }
  connection_->Read(base::BindOnce(&FidoHidDevice::OnReadContinuation,
                                   weak_factory_.GetWeakPtr(),
                                   std::move(message), std::move(callback)));
}

void FidoHidDevice::MessageReceived(DeviceCallback callback,
                                    base::Optional<FidoHidMessage> message) {
  if (state_ == State::kDeviceError)
    return;

  timeout_callback_.Cancel();
  if (!message) {
    state_ = State::kDeviceError;
    Transition(std::vector<uint8_t>(), std::move(callback));
    return;
  }

  const auto cmd = message->cmd();
  // If received HID packet has keep_alive as command type, re-read after delay.
  if (supported_protocol() == ProtocolVersion::kCtap &&
      cmd == FidoHidDeviceCommand::kKeepAlive) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&FidoHidDevice::OnKeepAlive, weak_factory_.GetWeakPtr(),
                       std::move(callback)),
        kHidKeepAliveDelay);
    return;
  }

  if (cmd != FidoHidDeviceCommand::kMsg && cmd != FidoHidDeviceCommand::kCbor) {
    DLOG(ERROR) << "Unexpected HID device command received.";
    state_ = State::kDeviceError;
    Transition(std::vector<uint8_t>(), std::move(callback));
    return;
  }

  auto response = message->GetMessagePayload();
  state_ = State::kReady;
  base::WeakPtr<FidoHidDevice> self = weak_factory_.GetWeakPtr();
  std::move(callback).Run(
      message ? base::make_optional(message->GetMessagePayload())
              : base::nullopt);

  // Executing |callback| may have freed |this|. Check |self| first.
  if (self && !pending_transactions_.empty()) {
    // If any transactions were queued, process the first one.
    auto pending_cmd = std::move(pending_transactions_.front().first);
    auto pending_cb = std::move(pending_transactions_.front().second);
    pending_transactions_.pop();
    Transition(std::move(pending_cmd), std::move(pending_cb));
  }
}

void FidoHidDevice::TryWink(WinkCallback callback) {
  // Only try to wink if device claims support.
  if (!(capabilities_ & kWinkCapability) || state_ != State::kReady) {
    std::move(callback).Run();
    return;
  }

  WriteMessage(FidoHidMessage::Create(channel_id_, FidoHidDeviceCommand::kWink,
                                      std::vector<uint8_t>()),
               true,
               base::BindOnce(&FidoHidDevice::OnWink,
                              weak_factory_.GetWeakPtr(), std::move(callback)));
}

void FidoHidDevice::OnKeepAlive(DeviceCallback callback) {
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  ArmTimeout(repeating_callback);
  ReadMessage(base::BindOnce(&FidoHidDevice::MessageReceived,
                             weak_factory_.GetWeakPtr(),
                             std::move(repeating_callback)));
}

void FidoHidDevice::OnWink(WinkCallback callback,
                           base::Optional<FidoHidMessage> response) {
  std::move(callback).Run();
}

void FidoHidDevice::ArmTimeout(DeviceCallback callback) {
  DCHECK(timeout_callback_.IsCancelled());
  timeout_callback_.Reset(base::BindOnce(&FidoHidDevice::OnTimeout,
                                         weak_factory_.GetWeakPtr(),
                                         std::move(callback)));
  // Setup timeout task for 3 seconds.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_callback_.callback(), kDeviceTimeout);
}

void FidoHidDevice::OnTimeout(DeviceCallback callback) {
  state_ = State::kDeviceError;
  Transition(std::vector<uint8_t>(), std::move(callback));
}

std::string FidoHidDevice::GetId() const {
  return GetIdForDevice(*device_info_);
}

// static
std::string FidoHidDevice::GetIdForDevice(
    const device::mojom::HidDeviceInfo& device_info) {
  return "hid:" + device_info.guid;
}

// static
bool FidoHidDevice::IsTestEnabled() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(switches::kEnableU2fHidTest);
}

base::WeakPtr<FidoDevice> FidoHidDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace device
