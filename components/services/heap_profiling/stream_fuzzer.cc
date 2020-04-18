// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/receiver.h"
#include "components/services/heap_profiling/stream_parser.h"

#include <utility>

namespace heap_profiling {
namespace {

class DummyReceiver : public Receiver {
  void OnHeader(const StreamHeader& header) override {}
  void OnAlloc(const AllocPacket& alloc_packet,
               std::vector<Address>&& stack,
               std::string&& context) override {}
  void OnFree(const FreePacket& free_packet) override {}
  void OnBarrier(const BarrierPacket& barrier_packet) override {}
  void OnComplete() override {}
  void OnStringMapping(const StringMappingPacket& string_mapping_packet,
                       const std::string& str) override {}
};

}  // namespace
}  // namespace heap_profiling

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  heap_profiling::DummyReceiver receiver;
  scoped_refptr<heap_profiling::StreamParser> parser(
      new heap_profiling::StreamParser(&receiver));
  std::unique_ptr<char[]> stream_data(new char[size]);
  memcpy(stream_data.get(), data, size);
  parser->OnStreamData(std::move(stream_data), size);
  return 0;
}
