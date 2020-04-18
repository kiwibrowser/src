// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// We use a custom stream format for performance, since we're potentially
// sending a packet for every malloc and free.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_STREAM_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_STREAM_H_

#include <stdint.h>

#include "build/build_config.h"

namespace heap_profiling {

// These values should be kept in sync with
// chrome/profiling/stream_fuzzer.dict to ensure efficient fuzzer
// coverage of the stream parser.
constexpr uint32_t kStreamSignature = 0xF6103B71;

constexpr uint32_t kAllocPacketType = 0xF6103B72;
constexpr uint32_t kFreePacketType = 0xF6103B73;
constexpr uint32_t kBarrierPacketType = 0xF6103B74;
constexpr uint32_t kStringMappingPacketType = 0xF6103B75;

constexpr uint32_t kMaxStackEntries = 256;
constexpr uint32_t kMaxContextLen = 256;

// This should count up from 0 so it can be used to index into an array.
enum class AllocatorType : uint32_t {
  kMalloc = 0,
  kPartitionAlloc = 1,
  kOilpan = 2,
  kCount  // Number of allocator types.
};

#pragma pack(push, 1)
struct StreamHeader {
  uint32_t signature = kStreamSignature;
};

struct AllocPacket {
  uint32_t op = kAllocPacketType;

  AllocatorType allocator;

  uint64_t address;
  uint64_t size;

  // Number of stack entries following this header.
  uint32_t stack_len;

  // Number of context bytes followint the stack;
  uint32_t context_byte_len;

  // Immediately followed by |stack_len| uint64_t addresses and
  // |context_byte_len| bytes of context (not null terminated).
};

struct FreePacket {
  uint32_t op = kFreePacketType;

  uint64_t address;
};

// A barrier packet is a way to synchronize with the sender to make sure all
// events are received up to a certain point. The barrier ID is just a number
// that can be used to uniquely identify these events.
struct BarrierPacket {
  const uint32_t op = kBarrierPacketType;

  uint32_t barrier_id;
};

// Clients will sometimes use pointers to const strings in place of instruction
// addresses in AllocPackets. Prior to using such a pointer, the client should
// send a StringMappingPacket to inform the profiling service.
struct StringMappingPacket {
  const uint32_t op = kStringMappingPacketType;
  uint64_t address;
  uint32_t string_len;

  // Immediately followed by |string_len| bytes of string (not null
  // terminated).
};

#pragma pack(pop)

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_STREAM_H_
