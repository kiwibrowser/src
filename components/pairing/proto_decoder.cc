// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/proto_decoder.h"

#include "components/pairing/pairing_api.pb.h"
#include "net/base/io_buffer.h"

namespace {
enum {
  MESSAGE_NONE,
  MESSAGE_HOST_STATUS,
  MESSAGE_CONFIGURE_HOST,
  MESSAGE_PAIR_DEVICES,
  MESSAGE_COMPLETE_SETUP,
  MESSAGE_ERROR,
  MESSAGE_ADD_NETWORK,
  MESSAGE_REBOOT,
  NUM_MESSAGES,
};
}

namespace pairing_chromeos {

ProtoDecoder::ProtoDecoder(Observer* observer)
    : observer_(observer),
      next_message_type_(MESSAGE_NONE),
      next_message_size_(0) {
  DCHECK(observer_);
}

ProtoDecoder::~ProtoDecoder() {}

bool ProtoDecoder::DecodeIOBuffer(int size,
                                  ProtoDecoder::IOBufferRefPtr io_buffer) {
  // Update the message buffer.
  message_buffer_.AddIOBuffer(io_buffer, size);

  // If there is no current message, the next byte is the message type.
  if (next_message_type_ == MESSAGE_NONE) {
    if (message_buffer_.AvailableBytes() < static_cast<int>(sizeof(uint8_t)))
      return true;

    uint8_t message_type = MESSAGE_NONE;
    message_buffer_.ReadBytes(reinterpret_cast<char*>(&message_type),
                               sizeof(message_type));

    if (message_type == MESSAGE_NONE || message_type >= NUM_MESSAGES) {
      LOG(ERROR) << "Unknown message type received: " << message_type;
      return false;
    }
    next_message_type_ = message_type;
  }

  // If the message size isn't set, the next two bytes are the message size.
  if (next_message_size_ == 0) {
    if (message_buffer_.AvailableBytes() < static_cast<int>(sizeof(uint16_t)))
      return true;

    // The size is sent in network byte order.
    uint8_t high_byte = 0;
    message_buffer_.ReadBytes(reinterpret_cast<char*>(&high_byte),
                               sizeof(high_byte));
    uint8_t low_byte = 0;
    message_buffer_.ReadBytes(reinterpret_cast<char*>(&low_byte),
                               sizeof(low_byte));

    next_message_size_ = (high_byte << 8) + low_byte;
  }

  // If the whole proto buffer is not yet available, return early.
  if (message_buffer_.AvailableBytes() < next_message_size_)
    return true;

  std::vector<char> buffer(next_message_size_);
  message_buffer_.ReadBytes(&buffer[0], next_message_size_);

  switch (next_message_type_) {
    case MESSAGE_HOST_STATUS: {
        pairing_api::HostStatus message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnHostStatusMessage(message);
      }
      break;
    case MESSAGE_CONFIGURE_HOST: {
        pairing_api::ConfigureHost message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnConfigureHostMessage(message);
      }
      break;
    case MESSAGE_PAIR_DEVICES: {
        pairing_api::PairDevices message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnPairDevicesMessage(message);
      }
      break;
    case MESSAGE_COMPLETE_SETUP: {
        pairing_api::CompleteSetup message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnCompleteSetupMessage(message);
      }
      break;
    case MESSAGE_ERROR: {
        pairing_api::Error message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnErrorMessage(message);
      }
      break;
    case MESSAGE_ADD_NETWORK: {
        pairing_api::AddNetwork message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnAddNetworkMessage(message);
      }
      break;
    case MESSAGE_REBOOT: {
        pairing_api::Reboot message;
        message.ParseFromArray(&buffer[0], buffer.size());
        observer_->OnRebootMessage(message);
      }
      break;

    default:
      LOG(WARNING) << "Skipping unknown message type: " << next_message_type_;
      break;
  }

  // Reset the message data.
  next_message_type_ = MESSAGE_NONE;
  next_message_size_ = 0;

  return true;
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendHostStatus(
    const pairing_api::HostStatus& message, int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_HOST_STATUS, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendHostNetwork(
    const pairing_api::AddNetwork& message,
    int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_ADD_NETWORK, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendConfigureHost(
    const pairing_api::ConfigureHost& message, int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_CONFIGURE_HOST, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendPairDevices(
    const pairing_api::PairDevices& message, int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_PAIR_DEVICES, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendCompleteSetup(
    const pairing_api::CompleteSetup& message, int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_COMPLETE_SETUP, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendError(
    const pairing_api::Error& message, int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_ERROR, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendRebootHost(
    const pairing_api::Reboot& message,
    int* size) {
  std::string serialized_proto;
  if (!message.SerializeToString(&serialized_proto))
    NOTREACHED();

  return SendMessage(MESSAGE_REBOOT, serialized_proto, size);
}

ProtoDecoder::IOBufferRefPtr ProtoDecoder::SendMessage(
    uint8_t message_type,
    const std::string& message,
    int* size) {
  uint16_t message_size = message.size();

  *size = sizeof(message_type) + sizeof(message_size) + message.size();
  IOBufferRefPtr io_buffer(new net::IOBuffer(*size));

  // Write the message type.
  int offset = 0;
  memcpy(&io_buffer->data()[offset], &message_type, sizeof(message_type));
  offset += sizeof(message_type);

  // Network byte order.
  // Write the high byte of the size.
  uint8_t data = (message_size >> 8) & 0xFF;
  memcpy(&io_buffer->data()[offset], &data, sizeof(data));
  offset += sizeof(data);
  // Write the low byte of the size.
  data = message_size & 0xFF;
  memcpy(&io_buffer->data()[offset], &data, sizeof(data));
  offset += sizeof(data);

  // Write the actual message.
  memcpy(&io_buffer->data()[offset], message.data(), message.size());

  return io_buffer;
}

}  // namespace pairing_chromeos
