// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_PARSER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_PARSER_H_

#include "base/callback.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "components/services/heap_profiling/receiver.h"
#include "components/services/heap_profiling/stream_receiver.h"

namespace heap_profiling {

// Parses a memory stream. Refcounted via StreamReceiver.
class StreamParser : public StreamReceiver {
 public:
  // Both receivers must either outlive this class or live until
  // DisconnectReceivers is called.
  explicit StreamParser(Receiver* receiver);

  // For tear-down, resets both receivers so they will not be called.
  void DisconnectReceivers();

  // StreamReceiver implementation.
  bool OnStreamData(std::unique_ptr<char[]> data, size_t sz) override;
  void OnStreamComplete() override;

  base::Lock* GetLock() { return &lock_; }

  // Returns true if this stream has encountered a fatal parse error.
  bool has_error() const { return error_; }

 private:
  struct Block {
    Block(std::unique_ptr<char[]> d, size_t s);
    Block(Block&& other) noexcept;
    ~Block();

    std::unique_ptr<char[]> data;
    size_t size;
  };

  enum ReadStatus {
    READ_OK,      // Read OK.
    READ_ERROR,   // Fatal error, don't send more data.
    READ_NO_DATA  // Not enough data, try again when we get more
  };

  ~StreamParser() override;

  // Returns true if the given number of bytes are available now.
  bool AreBytesAvailable(size_t count) const;

  // Returns false if not enough bytes are available. On failure, the dest
  // buffer will be in an undefined state (it may be written partially).
  bool PeekBytes(size_t count, void* dest) const;
  bool ReadBytes(size_t count, void* dest);
  void ConsumeBytes(size_t count);  // Bytes must be available.

  ReadStatus ParseHeader();
  ReadStatus ParseAlloc();
  ReadStatus ParseFree();
  ReadStatus ParseBarrier();
  ReadStatus ParseStringMapping();

  void SetErrorState();

  // Not owned by this class.
  Receiver* receiver_;

  base::circular_deque<Block> blocks_;

  bool received_header_ = false;
  bool error_ = false;

  // Current offset into blocks_[0] of the next packet to process.
  size_t block_zero_offset_ = 0;

  // This lock must be acquired anytime the stream is being parsed. This
  // prevents concurrent access to data structures used by both the parser and
  // the memory dumper.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(StreamParser);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_STREAM_PARSER_H_
