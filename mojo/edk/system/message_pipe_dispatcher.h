// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_MESSAGE_PIPE_DISPATCHER_H_
#define MOJO_EDK_SYSTEM_MESSAGE_PIPE_DISPATCHER_H_

#include <stdint.h>

#include <memory>
#include <queue>

#include "base/macros.h"
#include "mojo/edk/system/atomic_flag.h"
#include "mojo/edk/system/dispatcher.h"
#include "mojo/edk/system/ports/port_ref.h"
#include "mojo/edk/system/watcher_set.h"

namespace mojo {
namespace edk {

class NodeController;

class MessagePipeDispatcher : public Dispatcher {
 public:
  // Constructs a MessagePipeDispatcher permanently tied to a specific port.
  // |connected| must indicate the state of the port at construction time; if
  // the port is initialized with a peer, |connected| must be true. Otherwise it
  // must be false.
  //
  // A MessagePipeDispatcher may not be transferred while in a disconnected
  // state, and one can never return to a disconnected once connected.
  //
  // |pipe_id| is a unique identifier which can be used to track pipe endpoints
  // as they're passed around. |endpoint| is either 0 or 1 and again is only
  // used for tracking pipes (one side is always 0, the other is always 1.)
  MessagePipeDispatcher(NodeController* node_controller,
                        const ports::PortRef& port,
                        uint64_t pipe_id,
                        int endpoint);

  // Fuses this pipe with |other|. Returns |true| on success or |false| on
  // failure. Regardless of the return value, both dispatchers are closed by
  // this call.
  bool Fuse(MessagePipeDispatcher* other);

  // Dispatcher:
  Type GetType() const override;
  MojoResult Close() override;
  MojoResult WriteMessage(
      std::unique_ptr<ports::UserMessageEvent> message) override;
  MojoResult ReadMessage(
      std::unique_ptr<ports::UserMessageEvent>* message) override;
  HandleSignalsState GetHandleSignalsState() const override;
  MojoResult AddWatcherRef(const scoped_refptr<WatcherDispatcher>& watcher,
                           uintptr_t context) override;
  MojoResult RemoveWatcherRef(WatcherDispatcher* watcher,
                              uintptr_t context) override;
  void StartSerialize(uint32_t* num_bytes,
                      uint32_t* num_ports,
                      uint32_t* num_handles) override;
  bool EndSerialize(void* destination,
                    ports::PortName* ports,
                    ScopedInternalPlatformHandle* handles) override;
  bool BeginTransit() override;
  void CompleteTransitAndClose() override;
  void CancelTransit() override;

  static scoped_refptr<Dispatcher> Deserialize(
      const void* data,
      size_t num_bytes,
      const ports::PortName* ports,
      size_t num_ports,
      ScopedInternalPlatformHandle* handles,
      size_t num_handles);

 private:
  class PortObserverThunk;
  friend class PortObserverThunk;

  ~MessagePipeDispatcher() override;

  MojoResult CloseNoLock();
  HandleSignalsState GetHandleSignalsStateNoLock() const;
  void OnPortStatusChanged();

  // These are safe to access from any thread without locking.
  NodeController* const node_controller_;
  const ports::PortRef port_;
  const uint64_t pipe_id_;
  const int endpoint_;

  // Guards access to all the fields below.
  mutable base::Lock signal_lock_;

  // This is not the same is |port_transferred_|. It's only held true between
  // BeginTransit() and Complete/CancelTransit().
  AtomicFlag in_transit_;

  bool port_transferred_ = false;
  AtomicFlag port_closed_;
  WatcherSet watchers_;

  DISALLOW_COPY_AND_ASSIGN(MessagePipeDispatcher);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_MESSAGE_PIPE_DISPATCHER_H_
