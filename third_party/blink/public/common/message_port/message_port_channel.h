// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_MESSAGE_PORT_CHANNEL_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_MESSAGE_PORT_CHANNEL_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "third_party/blink/common/common_export.h"

namespace blink {

// MessagePortChannel corresponds to a HTML MessagePort. It is a thin wrapper
// around a Mojo MessagePipeHandle and used to provide methods for reading and
// writing messages. Currently all reading and writing is handled separately
// by other code, so MessagePortChannel is nothing other than a ref-counted
// holder of a mojo MessagePipeHandle, and is in the process of being removed.
class BLINK_COMMON_EXPORT MessagePortChannel {
 public:
  ~MessagePortChannel();
  MessagePortChannel();

  // Shallow copy, resulting in multiple references to the same port.
  MessagePortChannel(const MessagePortChannel& other);
  MessagePortChannel& operator=(const MessagePortChannel& other);

  explicit MessagePortChannel(mojo::ScopedMessagePipeHandle handle);

  const mojo::ScopedMessagePipeHandle& GetHandle() const;
  mojo::ScopedMessagePipeHandle ReleaseHandle() const;

  static std::vector<mojo::ScopedMessagePipeHandle> ReleaseHandles(
      const std::vector<MessagePortChannel>& ports);
  static std::vector<MessagePortChannel> CreateFromHandles(
      std::vector<mojo::ScopedMessagePipeHandle> handles);

 private:
  class State : public base::RefCountedThreadSafe<State> {
   public:
    State();
    explicit State(mojo::ScopedMessagePipeHandle handle);

    mojo::ScopedMessagePipeHandle TakeHandle();

    const mojo::ScopedMessagePipeHandle& handle() const { return handle_; }

   private:
    friend class base::RefCountedThreadSafe<State>;

    ~State();

    // Guards access to the fields below.
    base::Lock lock_;

    mojo::ScopedMessagePipeHandle handle_;
  };
  mutable scoped_refptr<State> state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_MESSAGE_PORT_CHANNEL_H_
