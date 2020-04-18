// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/string_data_pipe_producer.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/task_scheduler/post_task.h"

namespace mojo {

namespace {

// Attempts to write data to a producer handle. Outputs the actual number of
// bytes written in |*size|, and returns a result indicating the status of the
// last attempted write operation.
MojoResult WriteDataToProducerHandle(DataPipeProducerHandle producer,
                                     const char* data,
                                     size_t* size) {
  void* dest;
  uint32_t bytes_left = static_cast<uint32_t>(*size);

  // We loop here since the pipe's available capacity may be larger than its
  // *contiguous* capacity, and hence two independent consecutive two-phase
  // writes may succeed. The goal here is to write as much of |data| as possible
  // until we either run out of data or run out of capacity.
  MojoResult result;
  do {
    uint32_t capacity = bytes_left;
    result =
        producer.BeginWriteData(&dest, &capacity, MOJO_WRITE_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      result = MOJO_RESULT_OK;
      break;
    } else if (result != MOJO_RESULT_OK) {
      break;
    }

    capacity = std::min(capacity, bytes_left);
    memcpy(dest, data, capacity);
    MojoResult end_result = producer.EndWriteData(capacity);
    DCHECK_EQ(MOJO_RESULT_OK, end_result);

    data += capacity;
    bytes_left -= capacity;
  } while (bytes_left);

  *size -= bytes_left;
  return result;
}

}  // namespace

StringDataPipeProducer::StringDataPipeProducer(
    ScopedDataPipeProducerHandle producer)
    : producer_(std::move(producer)),
      watcher_(FROM_HERE,
               SimpleWatcher::ArmingPolicy::AUTOMATIC,
               base::SequencedTaskRunnerHandle::Get()),
      weak_factory_(this) {}

StringDataPipeProducer::~StringDataPipeProducer() = default;

void StringDataPipeProducer::Write(const base::StringPiece& data,
                                   AsyncWritingMode mode,
                                   CompletionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);

  // Immediately attempt to write data without making an extra copy. If we can
  // get it all in one shot, we're done aleady.
  size_t size = data.size();
  MojoResult result =
      WriteDataToProducerHandle(producer_.get(), data.data(), &size);
  if (result == MOJO_RESULT_OK && size == data.size()) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&StringDataPipeProducer::InvokeCallback,
                                  weak_factory_.GetWeakPtr(), MOJO_RESULT_OK));
  } else {
    // Copy whatever data didn't make the cut and try again when the pipe has
    // some more capacity.
    if (mode == AsyncWritingMode::STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION) {
      data_ = std::string(data.data() + size, data.size() - size);
      data_view_ = data_;
    } else {
      data_view_ = base::StringPiece(data.data() + size, data.size() - size);
    }
    watcher_.Watch(producer_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                   MOJO_WATCH_CONDITION_SATISFIED,
                   base::Bind(&StringDataPipeProducer::OnProducerHandleReady,
                              base::Unretained(this)));
  }
}

void StringDataPipeProducer::InvokeCallback(MojoResult result) {
  // May delete |this|.
  std::move(callback_).Run(result);
}

void StringDataPipeProducer::OnProducerHandleReady(
    MojoResult ready_result,
    const HandleSignalsState& state) {
  bool failed = false;
  size_t size = data_view_.size();
  if (ready_result == MOJO_RESULT_OK) {
    MojoResult write_result =
        WriteDataToProducerHandle(producer_.get(), data_view_.data(), &size);
    if (write_result != MOJO_RESULT_OK)
      failed = true;
  } else {
    failed = true;
  }

  if (failed) {
    watcher_.Cancel();

    // May delete |this|.
    std::move(callback_).Run(MOJO_RESULT_ABORTED);
    return;
  }

  if (size == data_view_.size()) {
    watcher_.Cancel();

    // May delete |this|.
    std::move(callback_).Run(MOJO_RESULT_OK);
    return;
  }

  data_view_ =
      base::StringPiece(data_view_.data() + size, data_view_.size() - size);
}

}  // namespace mojo
