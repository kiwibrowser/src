// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/stream_parser.h"

#include <algorithm>

#include "base/containers/stack_container.h"
#include "base/strings/stringprintf.h"
#include "components/services/heap_profiling/address.h"
#include "components/services/heap_profiling/backtrace.h"
#include "components/services/heap_profiling/public/cpp/stream.h"

namespace heap_profiling {

StreamParser::Block::Block(std::unique_ptr<char[]> d, size_t s)
    : data(std::move(d)), size(s) {}

StreamParser::Block::Block(Block&& other) noexcept = default;

StreamParser::Block::~Block() = default;

StreamParser::StreamParser(Receiver* receiver) : receiver_(receiver) {}

StreamParser::~StreamParser() {}

void StreamParser::DisconnectReceivers() {
  base::AutoLock lock(lock_);
  receiver_ = nullptr;
}

bool StreamParser::OnStreamData(std::unique_ptr<char[]> data, size_t sz) {
  base::AutoLock l(lock_);
  if (!receiver_ || error_)
    return false;

  blocks_.emplace_back(std::move(data), sz);

  if (!received_header_) {
    ReadStatus status = ParseHeader();
    if (status == READ_NO_DATA)
      return true;  // Wait for more data.
    if (status == READ_ERROR) {
      SetErrorState();
      return false;
    }
    received_header_ = true;
  }

  while (true) {
    uint32_t msg_type;
    if (!PeekBytes(sizeof(msg_type), &msg_type))
      return true;  // Not enough data for a message type field.

    ReadStatus status;
    switch (msg_type) {
      case kAllocPacketType:
        status = ParseAlloc();
        break;
      case kFreePacketType:
        status = ParseFree();
        break;
      case kBarrierPacketType:
        status = ParseBarrier();
        break;
      case kStringMappingPacketType:
        status = ParseStringMapping();
        break;
      default:
        // Invalid message type.
        status = READ_ERROR;
        break;
    }

    if (status == READ_NO_DATA)
      return true;  // Wait for more data.
    if (status == READ_ERROR) {
      SetErrorState();
      return false;
    }
    // Success, loop around for more data.
  }
}

void StreamParser::OnStreamComplete() {
  base::AutoLock l(lock_);
  if (receiver_)
    receiver_->OnComplete();
}

bool StreamParser::AreBytesAvailable(size_t count) const {
  size_t used = 0;
  size_t current_block_offset = block_zero_offset_;
  for (auto it = blocks_.begin(); it != blocks_.end() && used < count; ++it) {
    used += it->size - current_block_offset;
    current_block_offset = 0;
  }
  return used >= count;
}

bool StreamParser::PeekBytes(size_t count, void* dest) const {
  char* dest_char = static_cast<char*>(dest);
  size_t used = 0;

  size_t current_block_offset = block_zero_offset_;
  for (const auto& block : blocks_) {
    size_t in_current_block = block.size - current_block_offset;
    size_t to_copy = std::min(count - used, in_current_block);

    memcpy(&dest_char[used], &block.data[current_block_offset], to_copy);
    used += to_copy;

    // All subsequent blocks start reading at offset 0.
    current_block_offset = 0;
  }
  return used == count;
}

bool StreamParser::ReadBytes(size_t count, void* dest) {
  if (!PeekBytes(count, dest))
    return false;
  ConsumeBytes(count);
  return true;
}

void StreamParser::ConsumeBytes(size_t count) {
  DCHECK(AreBytesAvailable(count));
  while (count > 0) {
    size_t bytes_left_in_block = blocks_.front().size - block_zero_offset_;
    if (bytes_left_in_block > count) {
      // Still data left in this block;
      block_zero_offset_ += count;
      return;
    }

    // Current block is consumed.
    blocks_.pop_front();
    block_zero_offset_ = 0;
    count -= bytes_left_in_block;
  }
}

StreamParser::ReadStatus StreamParser::ParseHeader() {
  StreamHeader header;
  if (!ReadBytes(sizeof(StreamHeader), &header))
    return READ_NO_DATA;

  if (header.signature != kStreamSignature) {
    return READ_ERROR;
  }

  receiver_->OnHeader(header);
  return READ_OK;
}

StreamParser::ReadStatus StreamParser::ParseAlloc() {
  // Read the packet. Can't commit the read until the stack is read and
  // that has to be done below.
  AllocPacket alloc_packet;
  if (!PeekBytes(sizeof(AllocPacket), &alloc_packet))
    return READ_NO_DATA;

  // Validate data.
  if (alloc_packet.stack_len > kMaxStackEntries ||
      alloc_packet.context_byte_len > kMaxContextLen ||
      alloc_packet.allocator >= AllocatorType::kCount) {
    return READ_ERROR;
  }

  std::vector<Address> stack;
  stack.resize(alloc_packet.stack_len);
  size_t stack_byte_size = sizeof(Address) * alloc_packet.stack_len;

  if (!AreBytesAvailable(sizeof(AllocPacket) + stack_byte_size +
                         alloc_packet.context_byte_len))
    return READ_NO_DATA;

  // Everything will fit, mark header consumed.
  ConsumeBytes(sizeof(AllocPacket));

  // Read stack.
  if (!stack.empty())
    ReadBytes(stack_byte_size, stack.data());

  // Read context.
  std::string context;
  context.resize(alloc_packet.context_byte_len);
  if (alloc_packet.context_byte_len)
    ReadBytes(alloc_packet.context_byte_len, &context[0]);

  receiver_->OnAlloc(alloc_packet, std::move(stack), std::move(context));
  return READ_OK;
}

StreamParser::ReadStatus StreamParser::ParseFree() {
  FreePacket free_packet;
  if (!ReadBytes(sizeof(FreePacket), &free_packet))
    return READ_NO_DATA;

  receiver_->OnFree(free_packet);
  return READ_OK;
}

StreamParser::ReadStatus StreamParser::ParseBarrier() {
  BarrierPacket barrier_packet;
  if (!ReadBytes(sizeof(BarrierPacket), &barrier_packet))
    return READ_NO_DATA;

  receiver_->OnBarrier(barrier_packet);
  return READ_OK;
}

StreamParser::ReadStatus StreamParser::ParseStringMapping() {
  StringMappingPacket string_mapping_packet;
  if (!PeekBytes(sizeof(StringMappingPacket), &string_mapping_packet))
    return READ_NO_DATA;

  if (!AreBytesAvailable(sizeof(StringMappingPacket) +
                         string_mapping_packet.string_len))
    return READ_NO_DATA;

  // Everything will fit, mark header consumed.
  ConsumeBytes(sizeof(StringMappingPacket));

  // Treat the incoming characters as an opaque blob. It should not contain null
  // characters but a malicious attacker could change that.
  std::string str;

  str.resize(string_mapping_packet.string_len);
  ReadBytes(string_mapping_packet.string_len, &str[0]);

  receiver_->OnStringMapping(string_mapping_packet, str);
  return READ_OK;
}

void StreamParser::SetErrorState() {
  LOG(ERROR) << "StreamParser parsing error";
  error_ = true;
  receiver_->OnComplete();
}

}  // namespace heap_profiling
