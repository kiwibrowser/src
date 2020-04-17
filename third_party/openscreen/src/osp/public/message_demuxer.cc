// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/message_demuxer.h"

#include <memory>

#include "osp/impl/quic/quic_connection.h"
#include "osp_base/big_endian.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"

namespace openscreen {

// static
// Decodes a varUint, expecting it to follow the encoding format described here:
// https://tools.ietf.org/html/draft-ietf-quic-transport-16#section-16
ErrorOr<uint64_t> MessageTypeDecoder::DecodeVarUint(
    const std::vector<uint8_t>& buffer,
    size_t* num_bytes_decoded) {
  if (buffer.size() == 0) {
    return Error::Code::kCborIncompleteMessage;
  }

  uint8_t num_type_bytes = static_cast<uint8_t>(buffer[0] >> 6 & 0x03);
  *num_bytes_decoded = 0x1 << num_type_bytes;

  // Ensure that ReadBigEndian won't read beyond the end of the buffer. Also,
  // since we expect the id to be followed by the message, equality is not valid
  if (buffer.size() <= *num_bytes_decoded) {
    return Error::Code::kCborIncompleteMessage;
  }

  switch (num_type_bytes) {
    case 0:
      return ReadBigEndian<uint8_t>(&buffer[0]) & ~0xC0;
    case 1:
      return ReadBigEndian<uint16_t>(&buffer[0]) & ~(0xC0 << 8);
    case 2:
      return ReadBigEndian<uint32_t>(&buffer[0]) & ~(0xC0 << 24);
    case 3:
      return ReadBigEndian<uint64_t>(&buffer[0]) & ~(uint64_t{0xC0} << 56);
    default:
      OSP_NOTREACHED();
      return Error::Code::kCborParsing;
  }
}

// static
// Decodes the Type of message, expecting it to follow the encoding format
// described here:
// https://tools.ietf.org/html/draft-ietf-quic-transport-16#section-16
ErrorOr<msgs::Type> MessageTypeDecoder::DecodeType(
    const std::vector<uint8_t>& buffer,
    size_t* num_bytes_decoded) {
  ErrorOr<uint64_t> message_type =
      MessageTypeDecoder::DecodeVarUint(buffer, num_bytes_decoded);
  if (message_type.is_error()) {
    return message_type.error();
  }

  msgs::Type parsed_type =
      msgs::TypeEnumValidator::SafeCast(message_type.value());
  if (parsed_type == msgs::Type::kUnknown) {
    return Error::Code::kCborInvalidMessage;
  }

  return parsed_type;
}

// static
constexpr size_t MessageDemuxer::kDefaultBufferLimit;

MessageDemuxer::MessageWatch::MessageWatch() = default;

MessageDemuxer::MessageWatch::MessageWatch(MessageDemuxer* parent,
                                           bool is_default,
                                           uint64_t endpoint_id,
                                           msgs::Type message_type)
    : parent_(parent),
      is_default_(is_default),
      endpoint_id_(endpoint_id),
      message_type_(message_type) {}

MessageDemuxer::MessageWatch::MessageWatch(MessageDemuxer::MessageWatch&& other)
    : parent_(other.parent_),
      is_default_(other.is_default_),
      endpoint_id_(other.endpoint_id_),
      message_type_(other.message_type_) {
  other.parent_ = nullptr;
}

MessageDemuxer::MessageWatch::~MessageWatch() {
  if (parent_) {
    if (is_default_) {
      OSP_VLOG << "dropping default handler for type: "
               << static_cast<int>(message_type_);
      parent_->StopDefaultMessageTypeWatch(message_type_);
    } else {
      OSP_VLOG << "dropping handler for type: "
               << static_cast<int>(message_type_);
      parent_->StopWatchingMessageType(endpoint_id_, message_type_);
    }
  }
}

MessageDemuxer::MessageWatch& MessageDemuxer::MessageWatch::operator=(
    MessageWatch&& other) {
  using std::swap;
  swap(parent_, other.parent_);
  swap(is_default_, other.is_default_);
  swap(endpoint_id_, other.endpoint_id_);
  swap(message_type_, other.message_type_);
  return *this;
}

MessageDemuxer::MessageDemuxer(platform::ClockNowFunctionPtr now_function,
                               size_t buffer_limit = kDefaultBufferLimit)
    : now_function_(now_function), buffer_limit_(buffer_limit) {
  OSP_DCHECK(now_function_);
}

MessageDemuxer::~MessageDemuxer() = default;

MessageDemuxer::MessageWatch MessageDemuxer::WatchMessageType(
    uint64_t endpoint_id,
    msgs::Type message_type,
    MessageCallback* callback) {
  auto callbacks_entry = message_callbacks_.find(endpoint_id);
  if (callbacks_entry == message_callbacks_.end()) {
    callbacks_entry =
        message_callbacks_
            .emplace(endpoint_id, std::map<msgs::Type, MessageCallback*>{})
            .first;
  }
  auto emplace_result = callbacks_entry->second.emplace(message_type, callback);
  if (!emplace_result.second)
    return MessageWatch();
  auto endpoint_entry = buffers_.find(endpoint_id);
  if (endpoint_entry != buffers_.end()) {
    for (auto& buffer : endpoint_entry->second) {
      if (buffer.second.empty())
        continue;
      auto buffered_type = static_cast<msgs::Type>(buffer.second[0]);
      if (message_type == buffered_type) {
        HandleStreamBufferLoop(endpoint_id, buffer.first, callbacks_entry,
                               &buffer.second);
      }
    }
  }
  return MessageWatch(this, false, endpoint_id, message_type);
}

MessageDemuxer::MessageWatch MessageDemuxer::SetDefaultMessageTypeWatch(
    msgs::Type message_type,
    MessageCallback* callback) {
  auto emplace_result = default_callbacks_.emplace(message_type, callback);
  if (!emplace_result.second)
    return MessageWatch();
  for (auto& endpoint_buffers : buffers_) {
    for (auto& buffer : endpoint_buffers.second) {
      if (buffer.second.empty())
        continue;
      auto buffered_type = static_cast<msgs::Type>(buffer.second[0]);
      if (message_type == buffered_type) {
        auto callbacks_entry = message_callbacks_.find(endpoint_buffers.first);
        HandleStreamBufferLoop(endpoint_buffers.first, buffer.first,
                               callbacks_entry, &buffer.second);
      }
    }
  }
  return MessageWatch(this, true, 0, message_type);
}

void MessageDemuxer::OnStreamData(uint64_t endpoint_id,
                                  uint64_t connection_id,
                                  const uint8_t* data,
                                  size_t data_size) {
  OSP_VLOG << __func__ << ": [" << endpoint_id << ", " << connection_id
           << "] - (" << data_size << ")";
  auto& stream_map = buffers_[endpoint_id];
  if (!data_size) {
    stream_map.erase(connection_id);
    if (stream_map.empty())
      buffers_.erase(endpoint_id);
    return;
  }
  std::vector<uint8_t>& buffer = stream_map[connection_id];
  buffer.insert(buffer.end(), data, data + data_size);

  auto callbacks_entry = message_callbacks_.find(endpoint_id);
  HandleStreamBufferLoop(endpoint_id, connection_id, callbacks_entry, &buffer);

  if (buffer.size() > buffer_limit_)
    stream_map.erase(connection_id);
}

void MessageDemuxer::StopWatchingMessageType(uint64_t endpoint_id,
                                             msgs::Type message_type) {
  auto& message_map = message_callbacks_[endpoint_id];
  auto it = message_map.find(message_type);
  message_map.erase(it);
}

void MessageDemuxer::StopDefaultMessageTypeWatch(msgs::Type message_type) {
  default_callbacks_.erase(message_type);
}

MessageDemuxer::HandleStreamBufferResult MessageDemuxer::HandleStreamBufferLoop(
    uint64_t endpoint_id,
    uint64_t connection_id,
    std::map<uint64_t, std::map<msgs::Type, MessageCallback*>>::iterator
        callbacks_entry,
    std::vector<uint8_t>* buffer) {
  HandleStreamBufferResult result;
  do {
    result = {false, 0};
    if (callbacks_entry != message_callbacks_.end()) {
      OSP_VLOG << "attempting endpoint-specific handling";
      result = HandleStreamBuffer(endpoint_id, connection_id,
                                  &callbacks_entry->second, buffer);
    }
    if (!result.handled) {
      if (!default_callbacks_.empty()) {
        OSP_VLOG << "attempting generic message handling";
        result = HandleStreamBuffer(endpoint_id, connection_id,
                                    &default_callbacks_, buffer);
      }
    }
    OSP_VLOG_IF(!result.handled) << "no message handler matched";
  } while (result.consumed && !buffer->empty());
  return result;
}

// TODO(rwkeane) Use absl::Span for the buffer
MessageDemuxer::HandleStreamBufferResult MessageDemuxer::HandleStreamBuffer(
    uint64_t endpoint_id,
    uint64_t connection_id,
    std::map<msgs::Type, MessageCallback*>* message_callbacks,
    std::vector<uint8_t>* buffer) {
  size_t consumed = 0;
  size_t total_consumed = 0;
  bool handled = false;
  do {
    consumed = 0;
    size_t msg_type_byte_length;
    ErrorOr<msgs::Type> message_type =
        MessageTypeDecoder::DecodeType(*buffer, &msg_type_byte_length);
    if (message_type.is_error()) {
      buffer->clear();
      break;
    }
    auto callback_entry = message_callbacks->find(message_type.value());
    if (callback_entry == message_callbacks->end())
      break;
    handled = true;
    OSP_VLOG << "handling message type "
             << static_cast<int>(message_type.value());
    auto consumed_or_error = callback_entry->second->OnStreamMessage(
        endpoint_id, connection_id, message_type.value(),
        buffer->data() + msg_type_byte_length,
        buffer->size() - msg_type_byte_length, now_function_());
    if (!consumed_or_error) {
      if (consumed_or_error.error().code() !=
          Error::Code::kCborIncompleteMessage) {
        buffer->clear();
        break;
      }
    } else {
      consumed = consumed_or_error.value();
      buffer->erase(buffer->begin(),
                    buffer->begin() + consumed + msg_type_byte_length);
    }
    total_consumed += consumed;
  } while (consumed && !buffer->empty());
  return HandleStreamBufferResult{handled, total_consumed};
}

void StopWatching(MessageDemuxer::MessageWatch* watch) {
  *watch = MessageDemuxer::MessageWatch();
}

}  // namespace openscreen
