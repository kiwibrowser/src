// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBRARIES_NACL_IO_STREAM_STREAM_EVENT_EMITTER_H_
#define LIBRARIES_NACL_IO_STREAM_STREAM_EVENT_EMITTER_H_

#include "nacl_io/event_emitter.h"

#include "sdk_util/macros.h"
#include "sdk_util/scoped_ref.h"

namespace nacl_io {

class FIFOInterface;
class StreamEventEmitter;
class StreamNode;

typedef sdk_util::ScopedRef<StreamEventEmitter> ScopedStreamEventEmitter;

class StreamEventEmitter : public EventEmitter {
 public:
  StreamEventEmitter();

  void AttachStream(StreamNode* stream);
  void DetachStream();

  StreamNode* stream() { return stream_; }

 protected:
  virtual FIFOInterface* in_fifo() = 0;
  virtual FIFOInterface* out_fifo() = 0;
  void UpdateStatus_Locked();

 protected:
  StreamNode* stream_;
  DISALLOW_COPY_AND_ASSIGN(StreamEventEmitter);
};

}  // namespace nacl_io

#endif  // LIBRARIES_NACL_IO_STREAM_STREAM_EVENT_EMITTER_H_
