// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/buffering_data_pipe_writer.h"

#include "base/single_thread_task_runner.h"

namespace blink {

namespace {

const auto kNone = MOJO_WRITE_DATA_FLAG_NONE;

}  // namespace

BufferingDataPipeWriter::BufferingDataPipeWriter(
    mojo::ScopedDataPipeProducerHandle handle,
    base::SingleThreadTaskRunner* runner)
    : handle_(std::move(handle)),
      watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL, runner) {
  watcher_.Watch(handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                 MOJO_WATCH_CONDITION_SATISFIED,
                 base::BindRepeating(&BufferingDataPipeWriter::OnWritable,
                                     base::Unretained(this)));
}

bool BufferingDataPipeWriter::Write(const char* buffer, uint32_t num_bytes) {
  DCHECK(!finished_);
  if (!handle_.is_valid())
    return false;

  if (buffer_.empty()) {
    while (num_bytes > 0) {
      uint32_t size = num_bytes;
      MojoResult result = handle_->WriteData(buffer, &size, kNone);
      if (result == MOJO_RESULT_SHOULD_WAIT)
        break;
      if (result != MOJO_RESULT_OK) {
        Clear();
        return false;
      }
      num_bytes -= size;
      buffer += size;
    }
  }
  if (num_bytes == 0)
    return true;

  buffer_.push_back(Vector<char>());
  buffer_.back().Append(buffer, num_bytes);
  if (!waiting_) {
    waiting_ = true;
    watcher_.ArmOrNotify();
  }
  return true;
}

void BufferingDataPipeWriter::Finish() {
  finished_ = true;
  ClearIfNeeded();
}

void BufferingDataPipeWriter::OnWritable(MojoResult,
                                         const mojo::HandleSignalsState&) {
  if (!handle_.is_valid())
    return;
  waiting_ = false;
  while (!buffer_.empty()) {
    WTF::Vector<char>& front = buffer_.front();

    uint32_t size = front.size() - front_written_size_;

    MojoResult result =
        handle_->WriteData(front.data() + front_written_size_, &size, kNone);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      waiting_ = true;
      watcher_.ArmOrNotify();
      return;
    }
    if (result != MOJO_RESULT_OK) {
      Clear();
      return;
    }
    front_written_size_ += size;

    if (front_written_size_ == front.size()) {
      front_written_size_ = 0;
      buffer_.TakeFirst();
    }
  }
  ClearIfNeeded();
}

void BufferingDataPipeWriter::Clear() {
  handle_.reset();
  watcher_.Cancel();
  buffer_.clear();
}

void BufferingDataPipeWriter::ClearIfNeeded() {
  if (!finished_)
    return;

  if (buffer_.empty())
    Clear();
}

}  // namespace blink
