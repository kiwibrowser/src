// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/stream_parser.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace heap_profiling {

namespace {

void SendData(const scoped_refptr<StreamParser>& parser,
              const void* data,
              size_t size) {
  std::unique_ptr<char[]> heap(new char[size]);
  memcpy(heap.get(), data, size);
  parser->OnStreamData(std::move(heap), size);
}

void SendHeader(scoped_refptr<StreamParser>& parser) {
  StreamHeader header;
  SendData(parser, &header, sizeof(StreamHeader));
}

class TestReceiver : public Receiver {
 public:
  TestReceiver() {
    // Make our saved header invalid so we can't confuse the locally
    // initialized one with a valid one that's received.
    header_.signature = 0;
    last_barrier_.barrier_id =
        std::numeric_limits<decltype(last_barrier_.barrier_id)>::max();
  }

  bool got_header() const { return got_header_; }
  const StreamHeader& header() const { return header_; }
  void OnHeader(const StreamHeader& header) override {
    ASSERT_FALSE(got_header_);  // Don't expect more than one.
    got_header_ = true;
    header_ = header;
  }

  int alloc_count() const { return alloc_count_; }
  const AllocPacket& last_alloc() const { return last_alloc_; }
  const std::vector<Address>& last_alloc_stack() const {
    return last_alloc_stack_;
  }
  const std::string& last_alloc_context() const { return last_alloc_context_; }
  void OnAlloc(const AllocPacket& alloc_packet,
               std::vector<Address>&& stack,
               std::string&& context) override {
    alloc_count_++;
    last_alloc_ = alloc_packet;
    last_alloc_stack_ = std::move(stack);
    last_alloc_context_ = std::move(context);
  }

  int free_count() const { return free_count_; }
  const FreePacket& last_free() const { return last_free_; }
  void OnFree(const FreePacket& free_packet) override {
    free_count_++;
    last_free_ = free_packet;
  }

  int barrier_count() const { return barrier_count_; }
  const BarrierPacket& last_barrier() const { return last_barrier_; }
  void OnBarrier(const BarrierPacket& barrier) override {
    barrier_count_++;
    last_barrier_.barrier_id = barrier.barrier_id;
  }

  int string_mapping_count() const { return string_mapping_count_; }
  const StringMappingPacket& last_string_mapping() const {
    return last_string_mapping_;
  }
  const char* last_raw_string() const { return last_raw_string_.c_str(); }

  void OnStringMapping(const StringMappingPacket& string_mapping,
                       const std::string& str) override {
    string_mapping_count_++;
    memcpy(&last_string_mapping_, &string_mapping, sizeof(StringMappingPacket));
    last_raw_string_ = str;
  }

  bool got_complete() const { return got_complete_; }
  void OnComplete() override {
    ASSERT_FALSE(got_complete_);  // Don't expect more than one.
    got_complete_ = true;
  }

 private:
  bool got_header_ = false;
  StreamHeader header_;

  int alloc_count_ = 0;
  AllocPacket last_alloc_;
  std::vector<Address> last_alloc_stack_;
  std::string last_alloc_context_;

  int free_count_ = 0;
  FreePacket last_free_;

  int barrier_count_ = 0;
  BarrierPacket last_barrier_;

  int string_mapping_count_ = 0;
  StringMappingPacket last_string_mapping_;
  std::string last_raw_string_;

  bool got_complete_ = false;
};

}  // namespace

TEST(StreamParser, NormalHeader) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));

  // Should work to send in two packets.
  StreamHeader header;
  size_t first_size = sizeof(StreamHeader) / 2;
  SendData(parser, &header, first_size);
  EXPECT_FALSE(receiver.got_header());
  SendData(parser, &reinterpret_cast<char*>(&header)[first_size],
           sizeof(StreamHeader) - first_size);
  EXPECT_TRUE(receiver.got_header());
  EXPECT_FALSE(receiver.got_complete());

  parser->OnStreamComplete();
  EXPECT_TRUE(receiver.got_complete());
}

TEST(StreamParser, BadHeader) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));

  StreamHeader header;
  header.signature = 15;
  SendData(parser, &header, sizeof(StreamHeader));
  EXPECT_FALSE(receiver.got_header());
  EXPECT_TRUE(receiver.got_complete());
  EXPECT_TRUE(parser->has_error());
}

TEST(StreamParser, GoodAlloc) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  constexpr size_t kStackSize = 4;
  uint64_t stack[kStackSize] = {0x1, 0x2, 0x3, 0x4};

  std::string context("hello");

  AllocPacket alloc;
  alloc.allocator = AllocatorType::kMalloc;
  alloc.address = 0x87654321;
  alloc.size = 128;
  alloc.stack_len = kStackSize;
  alloc.context_byte_len = static_cast<uint32_t>(context.size());

  SendData(parser, &alloc, sizeof(AllocPacket));
  EXPECT_EQ(0, receiver.alloc_count());
  SendData(parser, stack, sizeof(uint64_t) * kStackSize);
  ASSERT_EQ(0, receiver.alloc_count());
  SendData(parser, context.data(), context.size());
  ASSERT_EQ(1, receiver.alloc_count());

  EXPECT_EQ(alloc.allocator, receiver.last_alloc().allocator);
  EXPECT_EQ(alloc.address, receiver.last_alloc().address);
  EXPECT_EQ(alloc.size, receiver.last_alloc().size);
  EXPECT_EQ(alloc.stack_len, receiver.last_alloc().stack_len);
  EXPECT_EQ(context, receiver.last_alloc_context());

  ASSERT_EQ(4u, receiver.last_alloc_stack().size());
  EXPECT_EQ(stack[0], receiver.last_alloc_stack()[0].value);
  EXPECT_EQ(stack[1], receiver.last_alloc_stack()[1].value);
  EXPECT_EQ(stack[2], receiver.last_alloc_stack()[2].value);
  EXPECT_EQ(stack[3], receiver.last_alloc_stack()[3].value);
}

TEST(StreamParser, AllocBigStack) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  AllocPacket alloc;
  alloc.allocator = AllocatorType::kMalloc;
  alloc.address = 0x87654321;
  alloc.size = 128;
  alloc.stack_len = 10000;  // Too large a stack.
  alloc.context_byte_len = 0;

  SendData(parser, &alloc, sizeof(AllocPacket));

  // Even though no stack was sent, the alloc header with the too-large stack
  // should have triggered an error.
  EXPECT_EQ(0, receiver.alloc_count());
  EXPECT_TRUE(receiver.got_complete());
}

TEST(StreamParser, AllocBigContext) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  AllocPacket alloc;
  alloc.allocator = AllocatorType::kMalloc;
  alloc.address = 0x87654321;
  alloc.size = 128;
  alloc.stack_len = 0;  // Too large a stack.
  alloc.context_byte_len = 10000;

  SendData(parser, &alloc, sizeof(AllocPacket));

  // Even though no stack or context was sent, the alloc header with the
  // too-large stack should have triggered an error.
  EXPECT_EQ(0, receiver.alloc_count());
  EXPECT_TRUE(receiver.got_complete());
}

TEST(StreamParser, GoodFree) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  FreePacket fr;
  fr.address = 0x87654321;

  SendData(parser, &fr, sizeof(FreePacket));
  EXPECT_EQ(1, receiver.free_count());

  EXPECT_EQ(fr.address, receiver.last_free().address);
}

TEST(StreamParser, Barrier) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  constexpr uint32_t barrier_id = 0x12345678;

  BarrierPacket b;
  b.barrier_id = barrier_id;

  SendData(parser, &b, sizeof(BarrierPacket));
  EXPECT_EQ(1, receiver.barrier_count());

  EXPECT_EQ(barrier_id, receiver.last_barrier().barrier_id);
}

TEST(StreamParser, StringMapping) {
  TestReceiver receiver;
  scoped_refptr<StreamParser> parser(new StreamParser(&receiver));
  SendHeader(parser);

  const std::string kDummyText = "kDummyText";

  StringMappingPacket p;
  p.address = 0x1234;
  p.string_len = static_cast<uint32_t>(kDummyText.size());
  SendData(parser, &p, sizeof(StringMappingPacket));
  SendData(parser, kDummyText.data(), kDummyText.size());

  EXPECT_EQ(1, receiver.string_mapping_count());
  EXPECT_EQ(p.address, receiver.last_string_mapping().address);
  EXPECT_EQ(p.string_len, receiver.last_string_mapping().string_len);
  EXPECT_EQ(kDummyText, receiver.last_raw_string());
}

}  // namespace heap_profiling
