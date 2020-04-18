// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_cable_device.h"

#include <utility>

#include "base/command_line.h"
#include "base/strings/string_piece.h"
#include "device/fido/fido_ble_connection.h"
#include "device/fido/fido_ble_frames.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace switches {
constexpr char kEnableCableEncryption[] = "enable-cable-encryption";
}  // namespace switches

namespace {

// Maximum size of EncryptionData::read_sequence_num or
// EncryptionData::write_sequence_num allowed. If we encounter
// counter larger than |kMaxCounter| FidoCableDevice should error out.
constexpr size_t kMaxCounter = (1 << 24) - 1;

base::StringPiece ConvertToStringPiece(const std::vector<uint8_t>& data) {
  return base::StringPiece(reinterpret_cast<const char*>(data.data()),
                           data.size());
}

// static
bool IsEncryptionEnabled() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(switches::kEnableCableEncryption);
}

base::Optional<std::vector<uint8_t>> ConstructEncryptionNonce(
    base::span<const uint8_t> nonce,
    bool is_sender_client,
    uint32_t counter) {
  if (counter > kMaxCounter)
    return base::nullopt;

  auto constructed_nonce = fido_parsing_utils::Materialize(nonce);
  constructed_nonce.push_back(is_sender_client ? 0x00 : 0x01);
  constructed_nonce.push_back(counter >> 16 & 0xFF);
  constructed_nonce.push_back(counter >> 8 & 0xFF);
  constructed_nonce.push_back(counter & 0xFF);
  return constructed_nonce;
}

bool EncryptOutgoingMessage(
    const FidoCableDevice::EncryptionData& encryption_data,
    std::vector<uint8_t>* message_to_encrypt) {
  const auto nonce = ConstructEncryptionNonce(
      encryption_data.nonce, true /* is_sender_client */,
      encryption_data.write_sequence_num);
  if (!nonce)
    return false;

  DCHECK_EQ(nonce->size(), encryption_data.aes_key.NonceLength());
  std::string ciphertext;
  bool encryption_success = encryption_data.aes_key.Seal(
      ConvertToStringPiece(*message_to_encrypt), ConvertToStringPiece(*nonce),
      nullptr /* additional_data */, &ciphertext);
  if (!encryption_success)
    return false;

  message_to_encrypt->assign(ciphertext.begin(), ciphertext.end());
  return true;
}

bool DecryptIncomingMessage(
    const FidoCableDevice::EncryptionData& encryption_data,
    FidoBleFrame* incoming_frame) {
  const auto nonce = ConstructEncryptionNonce(
      encryption_data.nonce, false /* is_sender_client */,
      encryption_data.read_sequence_num);
  if (!nonce)
    return false;

  DCHECK_EQ(nonce->size(), encryption_data.aes_key.NonceLength());
  std::string ciphertext;

  bool decryption_success = encryption_data.aes_key.Open(
      ConvertToStringPiece(incoming_frame->data()),
      ConvertToStringPiece(*nonce), nullptr /* additional_data */, &ciphertext);
  if (!decryption_success)
    return false;

  incoming_frame->data().assign(ciphertext.begin(), ciphertext.end());
  return true;
}

}  // namespace

// FidoCableDevice::EncryptionData ----------------------------------------

FidoCableDevice::EncryptionData::EncryptionData(
    std::string session_key,
    const std::array<uint8_t, 8>& nonce)
    : encryption_key(std::move(session_key)), nonce(nonce) {
  DCHECK_EQ(encryption_key.size(), aes_key.KeyLength());
  aes_key.Init(&encryption_key);
}

FidoCableDevice::EncryptionData::EncryptionData(EncryptionData&& data) =
    default;

FidoCableDevice::EncryptionData& FidoCableDevice::EncryptionData::operator=(
    EncryptionData&& other) = default;

FidoCableDevice::EncryptionData::~EncryptionData() = default;

// FidoCableDevice::EncryptionData ----------------------------------------

FidoCableDevice::FidoCableDevice(std::string address,
                                 std::string session_key,
                                 const std::array<uint8_t, 8>& nonce)
    : FidoBleDevice(std::move(address)),
      encryption_data_(std::move(session_key), nonce),
      weak_factory_(this) {}

FidoCableDevice::FidoCableDevice(std::unique_ptr<FidoBleConnection> connection,
                                 std::string session_key,
                                 const std::array<uint8_t, 8>& nonce)
    : FidoBleDevice(std::move(connection)),
      encryption_data_(std::move(session_key), nonce),
      weak_factory_(this) {}

FidoCableDevice::~FidoCableDevice() = default;

void FidoCableDevice::DeviceTransact(std::vector<uint8_t> command,
                                     DeviceCallback callback) {
  if (IsEncryptionEnabled()) {
    if (!EncryptOutgoingMessage(encryption_data_, &command)) {
      state_ = State::kDeviceError;
      return;
    }

    ++encryption_data_.write_sequence_num;
  }

  AddToPendingFrames(FidoBleDeviceCommand::kMsg, std::move(command),
                     std::move(callback));
}

void FidoCableDevice::OnResponseFrame(FrameCallback callback,
                                      base::Optional<FidoBleFrame> frame) {
  // The request is done, time to reset |transaction_|.
  ResetTransaction();
  state_ = frame ? State::kReady : State::kDeviceError;

  if (frame && IsEncryptionEnabled()) {
    if (!DecryptIncomingMessage(encryption_data_, &frame.value())) {
      state_ = State::kDeviceError;
      frame = base::nullopt;
    }

    ++encryption_data_.read_sequence_num;
  }

  auto self = GetWeakPtr();
  std::move(callback).Run(std::move(frame));

  // Executing callbacks may free |this|. Check |self| first.
  if (self)
    Transition();
}

base::WeakPtr<FidoDevice> FidoCableDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace device
