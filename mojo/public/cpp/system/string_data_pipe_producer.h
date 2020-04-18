// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_STRING_DATA_PIPE_PRODUCER_H_
#define MOJO_PUBLIC_CPP_SYSTEM_STRING_DATA_PIPE_PRODUCER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_piece.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "mojo/public/cpp/system/system_export.h"

namespace mojo {

// Helper class which takes ownership of a ScopedDataPipeProducerHandle and
// assumes responsibility for feeding it the contents of a given string. This
// takes care of waiting for pipe capacity as needed, and can notify callers
// asynchronously when the operation is complete.
//
// Note that the StringDataPipeProducer must be kept alive until notified of
// completion to ensure that all of the string's data is written to the pipe.
// Premature destruction may result in partial or total truncation of data made
// available to the consumer.
class MOJO_CPP_SYSTEM_EXPORT StringDataPipeProducer {
 public:
  using CompletionCallback = base::OnceCallback<void(MojoResult result)>;

  // Constructs a new StringDataPipeProducer which will write data to
  // |producer|.
  explicit StringDataPipeProducer(ScopedDataPipeProducerHandle producer);
  ~StringDataPipeProducer();

  // Describes what happens to the data when an async writing situation occurs
  // (where the pipe cannot immediately accept all of the data).
  enum class AsyncWritingMode {
    // The |data| given to Write() may be invalidated before completion
    // |callback| is called. The pending |data| is copied and owned by this
    // class until all bytes are written.
    STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION,
    // The |data| given to Write() stays valid until the completion |callback|
    // is called.
    STRING_STAYS_VALID_UNTIL_COMPLETION
  };

  // Attempts to eventually write all of |data|. Invokes |callback|
  // asynchronously when done. Note that |callback| IS allowed to delete this
  // StringDataPipeProducer.
  //
  // If the data cannot be entirely written synchronously, then the |mode|
  // determines how this class holds the pending data.
  //
  // If the write is successful |result| will be |MOJO_RESULT_OK|. Otherwise
  // (e.g. if the producer detects the consumer is closed and the pipe has no
  // remaining capacity) |result| will be |MOJO_RESULT_ABORTED|.
  //
  // Note that if the StringDataPipeProducer is destroyed before |callback| can
  // be invoked, |callback| is *never* invoked, and the write will be
  // permanently interrupted (and the producer handle closed) after making
  // potentially only partial progress.
  //
  // Multiple writes may be performed in sequence (each one after the last
  // completes), but Write() must not be called before the |callback| for the
  // previous call to Write() (if any) has returned.
  void Write(const base::StringPiece& data,
             AsyncWritingMode mode,
             CompletionCallback callback);

 private:
  void InvokeCallback(MojoResult result);
  void OnProducerHandleReady(MojoResult ready_result,
                             const HandleSignalsState& state);

  ScopedDataPipeProducerHandle producer_;
  std::string data_;
  base::StringPiece data_view_;
  CompletionCallback callback_;
  SimpleWatcher watcher_;
  base::WeakPtrFactory<StringDataPipeProducer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StringDataPipeProducer);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_STRING_DATA_PIPE_PRODUCER_H_
