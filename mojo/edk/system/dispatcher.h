// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_DISPATCHER_H_
#define MOJO_EDK_SYSTEM_DISPATCHER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <ostream>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/platform_handle.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/handle_signals_state.h"
#include "mojo/edk/system/ports/name.h"
#include "mojo/edk/system/ports/port_ref.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/edk/system/watch.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/trap.h"
#include "mojo/public/c/system/types.h"

namespace mojo {
namespace edk {

namespace ports {
class UserMessageEvent;
}

class Dispatcher;
class PlatformSharedMemoryMapping;

using DispatcherVector = std::vector<scoped_refptr<Dispatcher>>;

// A |Dispatcher| implements Mojo EDK calls that are associated with a
// particular MojoHandle.
class MOJO_SYSTEM_IMPL_EXPORT Dispatcher
    : public base::RefCountedThreadSafe<Dispatcher> {
 public:
  struct DispatcherInTransit {
    DispatcherInTransit();
    DispatcherInTransit(const DispatcherInTransit& other);
    ~DispatcherInTransit();

    scoped_refptr<Dispatcher> dispatcher;
    MojoHandle local_handle;
  };

  enum class Type {
    UNKNOWN = 0,
    MESSAGE_PIPE,
    DATA_PIPE_PRODUCER,
    DATA_PIPE_CONSUMER,
    SHARED_BUFFER,
    WATCHER,
    INVITATION,

    // "Private" types (not exposed via the public interface):
    PLATFORM_HANDLE = -1,
  };

  // All Dispatchers must minimally implement these methods.

  virtual Type GetType() const = 0;
  virtual MojoResult Close() = 0;

  ///////////// Watcher API ////////////////////

  virtual MojoResult WatchDispatcher(scoped_refptr<Dispatcher> dispatcher,
                                     MojoHandleSignals signals,
                                     MojoTriggerCondition condition,
                                     uintptr_t context);
  virtual MojoResult CancelWatch(uintptr_t context);
  virtual MojoResult Arm(uint32_t* num_ready_contexts,
                         uintptr_t* ready_contexts,
                         MojoResult* ready_results,
                         MojoHandleSignalsState* ready_signals_states);

  ///////////// Message pipe API /////////////

  virtual MojoResult WriteMessage(
      std::unique_ptr<ports::UserMessageEvent> message);

  virtual MojoResult ReadMessage(
      std::unique_ptr<ports::UserMessageEvent>* message);

  ///////////// Shared buffer API /////////////

  // |options| may be null. |new_dispatcher| must not be null, but
  // |*new_dispatcher| should be null (and will contain the dispatcher for the
  // new handle on success).
  virtual MojoResult DuplicateBufferHandle(
      const MojoDuplicateBufferHandleOptions* options,
      scoped_refptr<Dispatcher>* new_dispatcher);

  virtual MojoResult MapBuffer(
      uint64_t offset,
      uint64_t num_bytes,
      std::unique_ptr<PlatformSharedMemoryMapping>* mapping);

  virtual MojoResult GetBufferInfo(MojoSharedBufferInfo* info);

  ///////////// Data pipe consumer API /////////////

  virtual MojoResult ReadData(const MojoReadDataOptions& options,
                              void* elements,
                              uint32_t* num_bytes);

  virtual MojoResult BeginReadData(const void** buffer,
                                   uint32_t* buffer_num_bytes);

  virtual MojoResult EndReadData(uint32_t num_bytes_read);

  ///////////// Data pipe producer API /////////////

  virtual MojoResult WriteData(const void* elements,
                               uint32_t* num_bytes,
                               const MojoWriteDataOptions& options);

  virtual MojoResult BeginWriteData(void** buffer, uint32_t* buffer_num_bytes);

  virtual MojoResult EndWriteData(uint32_t num_bytes_written);

  // Invitation API.
  virtual MojoResult AttachMessagePipe(base::StringPiece name,
                                       ports::PortRef remote_peer_port);
  virtual MojoResult ExtractMessagePipe(base::StringPiece name,
                                        MojoHandle* message_pipe_handle);

  ///////////// General-purpose API for all handle types /////////

  // Gets the current handle signals state. (The default implementation simply
  // returns a default-constructed |HandleSignalsState|, i.e., no signals
  // satisfied or satisfiable.) Note: The state is subject to change from other
  // threads.
  virtual HandleSignalsState GetHandleSignalsState() const;

  // Adds a WatcherDispatcher reference to this dispatcher, to be notified of
  // all subsequent changes to handle state including signal changes or closure.
  // The reference is associated with a |context| for disambiguation of
  // removals.
  virtual MojoResult AddWatcherRef(
      const scoped_refptr<WatcherDispatcher>& watcher,
      uintptr_t context);

  // Removes a WatcherDispatcher reference from this dispatcher.
  virtual MojoResult RemoveWatcherRef(WatcherDispatcher* watcher,
                                      uintptr_t context);

  // Informs the caller of the total serialized size (in bytes) and the total
  // number of platform handles and ports needed to transfer this dispatcher
  // across a message pipe.
  //
  // Must eventually be followed by a call to EndSerializeAndClose(). Note that
  // StartSerialize() and EndSerialize() are always called in sequence, and
  // only between calls to BeginTransit() and either (but not both)
  // CompleteTransitAndClose() or CancelTransit().
  //
  // For this reason it is IMPERATIVE that the implementation ensure a
  // consistent serializable state between BeginTransit() and
  // CompleteTransitAndClose()/CancelTransit().
  virtual void StartSerialize(uint32_t* num_bytes,
                              uint32_t* num_ports,
                              uint32_t* num_platform_handles);

  // Serializes this dispatcher into |destination|, |ports|, and |handles|.
  // Returns true iff successful, false otherwise. In either case the dispatcher
  // will close.
  //
  // NOTE: Transit MAY still fail after this call returns. Implementations
  // should not assume InternalPlatformHandle ownership has transferred until
  // CompleteTransitAndClose() is called. In other words, if CancelTransit() is
  // called, the implementation should retain its InternalPlatformHandles in
  // working condition.
  virtual bool EndSerialize(void* destination,
                            ports::PortName* ports,
                            ScopedInternalPlatformHandle* handles);

  // Does whatever is necessary to begin transit of the dispatcher.  This
  // should return |true| if transit is OK, or false if the underlying resource
  // is deemed busy by the implementation.
  virtual bool BeginTransit();

  // Does whatever is necessary to complete transit of the dispatcher, including
  // closure. This is only called upon successfully transmitting an outgoing
  // message containing this serialized dispatcher.
  virtual void CompleteTransitAndClose();

  // Does whatever is necessary to cancel transit of the dispatcher. The
  // dispatcher should remain in a working state and resume normal operation.
  virtual void CancelTransit();

  // Deserializes a specific dispatcher type from an incoming message.
  static scoped_refptr<Dispatcher> Deserialize(
      Type type,
      const void* bytes,
      size_t num_bytes,
      const ports::PortName* ports,
      size_t num_ports,
      ScopedInternalPlatformHandle* platform_handles,
      size_t platform_handle_count);

 protected:
  friend class base::RefCountedThreadSafe<Dispatcher>;

  Dispatcher();
  virtual ~Dispatcher();

  DISALLOW_COPY_AND_ASSIGN(Dispatcher);
};

// So logging macros and |DCHECK_EQ()|, etc. work.
MOJO_SYSTEM_IMPL_EXPORT inline std::ostream& operator<<(std::ostream& out,
                                                        Dispatcher::Type type) {
  return out << static_cast<int>(type);
}

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_DISPATCHER_H_
