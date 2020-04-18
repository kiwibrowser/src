// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/connector.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_local.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/lib/may_auto_lock.h"
#include "mojo/public/cpp/bindings/mojo_buildflags.h"
#include "mojo/public/cpp/bindings/sync_handle_watcher.h"
#include "mojo/public/cpp/system/wait.h"

namespace mojo {

namespace {

// The NestingObserver for each thread. Note that this is always a
// Connector::RunLoopNestingObserver; we use the base type here because that
// subclass is private to Connector.
base::LazyInstance<base::ThreadLocalPointer<base::RunLoop::NestingObserver>>::
    Leaky g_tls_nesting_observer = LAZY_INSTANCE_INITIALIZER;

// The default outgoing serialization mode for new Connectors.
Connector::OutgoingSerializationMode g_default_outgoing_serialization_mode =
    Connector::OutgoingSerializationMode::kLazy;

// The default incoming serialization mode for new Connectors.
Connector::IncomingSerializationMode g_default_incoming_serialization_mode =
    Connector::IncomingSerializationMode::kDispatchAsIs;

}  // namespace

// Used to efficiently maintain a doubly-linked list of all Connectors
// currently dispatching on any given thread.
class Connector::ActiveDispatchTracker {
 public:
  explicit ActiveDispatchTracker(const base::WeakPtr<Connector>& connector);
  ~ActiveDispatchTracker();

  void NotifyBeginNesting();

 private:
  const base::WeakPtr<Connector> connector_;
  RunLoopNestingObserver* const nesting_observer_;
  ActiveDispatchTracker* outer_tracker_ = nullptr;
  ActiveDispatchTracker* inner_tracker_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ActiveDispatchTracker);
};

// Watches the MessageLoop on the current thread. Notifies the current chain of
// ActiveDispatchTrackers when a nested run loop is started.
class Connector::RunLoopNestingObserver
    : public base::RunLoop::NestingObserver,
      public base::MessageLoopCurrent::DestructionObserver {
 public:
  RunLoopNestingObserver() {
    base::RunLoop::AddNestingObserverOnCurrentThread(this);
    base::MessageLoopCurrent::Get()->AddDestructionObserver(this);
  }

  ~RunLoopNestingObserver() override {}

  // base::RunLoop::NestingObserver:
  void OnBeginNestedRunLoop() override {
    if (top_tracker_)
      top_tracker_->NotifyBeginNesting();
  }

  // base::MessageLoopCurrent::DestructionObserver:
  void WillDestroyCurrentMessageLoop() override {
    base::RunLoop::RemoveNestingObserverOnCurrentThread(this);
    base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);
    DCHECK_EQ(this, g_tls_nesting_observer.Get().Get());
    g_tls_nesting_observer.Get().Set(nullptr);
    delete this;
  }

  static RunLoopNestingObserver* GetForThread() {
    if (!base::MessageLoopCurrent::Get())
      return nullptr;
    auto* observer = static_cast<RunLoopNestingObserver*>(
        g_tls_nesting_observer.Get().Get());
    if (!observer) {
      observer = new RunLoopNestingObserver;
      g_tls_nesting_observer.Get().Set(observer);
    }
    return observer;
  }

 private:
  friend class ActiveDispatchTracker;

  ActiveDispatchTracker* top_tracker_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(RunLoopNestingObserver);
};

Connector::ActiveDispatchTracker::ActiveDispatchTracker(
    const base::WeakPtr<Connector>& connector)
    : connector_(connector), nesting_observer_(connector_->nesting_observer_) {
  DCHECK(nesting_observer_);
  if (nesting_observer_->top_tracker_) {
    outer_tracker_ = nesting_observer_->top_tracker_;
    outer_tracker_->inner_tracker_ = this;
  }
  nesting_observer_->top_tracker_ = this;
}

Connector::ActiveDispatchTracker::~ActiveDispatchTracker() {
  if (nesting_observer_->top_tracker_ == this)
    nesting_observer_->top_tracker_ = outer_tracker_;
  else if (inner_tracker_)
    inner_tracker_->outer_tracker_ = outer_tracker_;
  if (outer_tracker_)
    outer_tracker_->inner_tracker_ = inner_tracker_;
}

void Connector::ActiveDispatchTracker::NotifyBeginNesting() {
  if (connector_ && connector_->handle_watcher_)
    connector_->handle_watcher_->ArmOrNotify();
  if (outer_tracker_)
    outer_tracker_->NotifyBeginNesting();
}

Connector::Connector(ScopedMessagePipeHandle message_pipe,
                     ConnectorConfig config,
                     scoped_refptr<base::SequencedTaskRunner> runner)
    : message_pipe_(std::move(message_pipe)),
      task_runner_(std::move(runner)),
      outgoing_serialization_mode_(g_default_outgoing_serialization_mode),
      incoming_serialization_mode_(g_default_incoming_serialization_mode),
      nesting_observer_(RunLoopNestingObserver::GetForThread()),
      weak_factory_(this) {
  if (config == MULTI_THREADED_SEND)
    lock_.emplace();

  weak_self_ = weak_factory_.GetWeakPtr();
  // Even though we don't have an incoming receiver, we still want to monitor
  // the message pipe to know if is closed or encounters an error.
  WaitToReadMore();
}

Connector::~Connector() {
  {
    // Allow for quick destruction on any sequence if the pipe is already
    // closed.
    base::AutoLock lock(connected_lock_);
    if (!connected_)
      return;
  }

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CancelWait();
}

void Connector::SetOutgoingSerializationMode(OutgoingSerializationMode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  outgoing_serialization_mode_ = mode;
}

void Connector::SetIncomingSerializationMode(IncomingSerializationMode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  incoming_serialization_mode_ = mode;
}

void Connector::CloseMessagePipe() {
  // Throw away the returned message pipe.
  PassMessagePipe();
}

ScopedMessagePipeHandle Connector::PassMessagePipe() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CancelWait();
  internal::MayAutoLock locker(&lock_);
  ScopedMessagePipeHandle message_pipe = std::move(message_pipe_);
  weak_factory_.InvalidateWeakPtrs();
  sync_handle_watcher_callback_count_ = 0;

  base::AutoLock lock(connected_lock_);
  connected_ = false;
  return message_pipe;
}

void Connector::RaiseError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  HandleError(true, true);
}

bool Connector::WaitForIncomingMessage(MojoDeadline deadline) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (error_)
    return false;

  ResumeIncomingMethodCallProcessing();

  // TODO(rockot): Use a timed Wait here. Nobody uses anything but 0 or
  // INDEFINITE deadlines at present, so we only support those.
  DCHECK(deadline == 0 || deadline == MOJO_DEADLINE_INDEFINITE);

  MojoResult rv = MOJO_RESULT_UNKNOWN;
  if (deadline == 0 && !message_pipe_->QuerySignalsState().readable())
    return false;

  if (deadline == MOJO_DEADLINE_INDEFINITE) {
    rv = Wait(message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE);
    if (rv != MOJO_RESULT_OK) {
      // Users that call WaitForIncomingMessage() should expect their code to be
      // re-entered, so we call the error handler synchronously.
      HandleError(rv != MOJO_RESULT_FAILED_PRECONDITION, false);
      return false;
    }
  }

  ignore_result(ReadSingleMessage(&rv));
  return (rv == MOJO_RESULT_OK);
}

void Connector::PauseIncomingMethodCallProcessing() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (paused_)
    return;

  paused_ = true;
  CancelWait();
}

void Connector::ResumeIncomingMethodCallProcessing() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!paused_)
    return;

  paused_ = false;
  WaitToReadMore();
}

bool Connector::PrefersSerializedMessages() {
  if (outgoing_serialization_mode_ == OutgoingSerializationMode::kEager)
    return true;
  DCHECK_EQ(OutgoingSerializationMode::kLazy, outgoing_serialization_mode_);
  return peer_remoteness_tracker_ &&
         peer_remoteness_tracker_->last_known_state().peer_remote();
}

bool Connector::Accept(Message* message) {
  if (!lock_)
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // It shouldn't hurt even if |error_| may be changed by a different sequence
  // at the same time. The outcome is that we may write into |message_pipe_|
  // after encountering an error, which should be fine.
  if (error_)
    return false;

  internal::MayAutoLock locker(&lock_);

  if (!message_pipe_.is_valid() || drop_writes_)
    return true;

  MojoResult rv =
      WriteMessageNew(message_pipe_.get(), message->TakeMojoMessage(),
                      MOJO_WRITE_MESSAGE_FLAG_NONE);

  switch (rv) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // There's no point in continuing to write to this pipe since the other
      // end is gone. Avoid writing any future messages. Hide write failures
      // from the caller since we'd like them to continue consuming any backlog
      // of incoming messages before regarding the message pipe as closed.
      drop_writes_ = true;
      break;
    case MOJO_RESULT_BUSY:
      // We'd get a "busy" result if one of the message's handles is:
      //   - |message_pipe_|'s own handle;
      //   - simultaneously being used on another sequence; or
      //   - in a "busy" state that prohibits it from being transferred (e.g.,
      //     a data pipe handle in the middle of a two-phase read/write,
      //     regardless of which sequence that two-phase read/write is happening
      //     on).
      // TODO(vtl): I wonder if this should be a |DCHECK()|. (But, until
      // crbug.com/389666, etc. are resolved, this will make tests fail quickly
      // rather than hanging.)
      CHECK(false) << "Race condition or other bug detected";
      return false;
    default:
      // This particular write was rejected, presumably because of bad input.
      // The pipe is not necessarily in a bad state.
      return false;
  }
  return true;
}

void Connector::AllowWokenUpBySyncWatchOnSameThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  allow_woken_up_by_others_ = true;

  EnsureSyncWatcherExists();
  sync_watcher_->AllowWokenUpBySyncWatchOnSameThread();
}

bool Connector::SyncWatch(const bool* should_stop) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (error_)
    return false;

  ResumeIncomingMethodCallProcessing();

  EnsureSyncWatcherExists();
  return sync_watcher_->SyncWatch(should_stop);
}

void Connector::SetWatcherHeapProfilerTag(const char* tag) {
  if (tag) {
    heap_profiler_tag_ = tag;
    if (handle_watcher_)
      handle_watcher_->set_heap_profiler_tag(tag);
  }
}

// static
void Connector::OverrideDefaultSerializationBehaviorForTesting(
    OutgoingSerializationMode outgoing_mode,
    IncomingSerializationMode incoming_mode) {
  g_default_outgoing_serialization_mode = outgoing_mode;
  g_default_incoming_serialization_mode = incoming_mode;
}

void Connector::OnWatcherHandleReady(MojoResult result) {
  OnHandleReadyInternal(result);
}

void Connector::OnSyncHandleWatcherHandleReady(MojoResult result) {
  base::WeakPtr<Connector> weak_self(weak_self_);

  sync_handle_watcher_callback_count_++;
  OnHandleReadyInternal(result);
  // At this point, this object might have been deleted.
  if (weak_self) {
    DCHECK_LT(0u, sync_handle_watcher_callback_count_);
    sync_handle_watcher_callback_count_--;
  }
}

void Connector::OnHandleReadyInternal(MojoResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (result != MOJO_RESULT_OK) {
    HandleError(result != MOJO_RESULT_FAILED_PRECONDITION, false);
    return;
  }

  ReadAllAvailableMessages();
  // At this point, this object might have been deleted. Return.
}

void Connector::WaitToReadMore() {
  CHECK(!paused_);
  DCHECK(!handle_watcher_);

  handle_watcher_.reset(new SimpleWatcher(
      FROM_HERE, SimpleWatcher::ArmingPolicy::MANUAL, task_runner_));
  handle_watcher_->set_heap_profiler_tag(heap_profiler_tag_);
  MojoResult rv = handle_watcher_->Watch(
      message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&Connector::OnWatcherHandleReady, base::Unretained(this)));

  if (message_pipe_.is_valid()) {
    peer_remoteness_tracker_.emplace(message_pipe_.get(),
                                     MOJO_HANDLE_SIGNAL_PEER_REMOTE);
  }

  if (rv != MOJO_RESULT_OK) {
    // If the watch failed because the handle is invalid or its conditions can
    // no longer be met, we signal the error asynchronously to avoid reentry.
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Connector::OnWatcherHandleReady, weak_self_, rv));
  } else {
    handle_watcher_->ArmOrNotify();
  }

  if (allow_woken_up_by_others_) {
    EnsureSyncWatcherExists();
    sync_watcher_->AllowWokenUpBySyncWatchOnSameThread();
  }
}

bool Connector::ReadSingleMessage(MojoResult* read_result) {
  CHECK(!paused_);

  bool receiver_result = false;

  // Detect if |this| was destroyed or the message pipe was closed/transferred
  // during message dispatch.
  base::WeakPtr<Connector> weak_self = weak_self_;

  Message message;
  const MojoResult rv = ReadMessage(message_pipe_.get(), &message);
  *read_result = rv;

  if (rv == MOJO_RESULT_OK) {
    base::Optional<ActiveDispatchTracker> dispatch_tracker;
    if (!is_dispatching_ && nesting_observer_) {
      is_dispatching_ = true;
      dispatch_tracker.emplace(weak_self);
    }

    if (incoming_serialization_mode_ ==
        IncomingSerializationMode::kSerializeBeforeDispatchForTesting) {
      message.SerializeIfNecessary();
    } else {
      DCHECK_EQ(IncomingSerializationMode::kDispatchAsIs,
                incoming_serialization_mode_);
    }

#if !BUILDFLAG(MOJO_TRACE_ENABLED)
    // This emits just full class name, and is inferior to mojo tracing.
    TRACE_EVENT0("mojom", heap_profiler_tag_);
#endif

    receiver_result =
        incoming_receiver_ && incoming_receiver_->Accept(&message);

    if (!weak_self)
      return false;

    if (dispatch_tracker) {
      is_dispatching_ = false;
      dispatch_tracker.reset();
    }
  } else if (rv == MOJO_RESULT_SHOULD_WAIT) {
    return true;
  } else {
    HandleError(rv != MOJO_RESULT_FAILED_PRECONDITION, false);
    return false;
  }

  if (enforce_errors_from_incoming_receiver_ && !receiver_result) {
    HandleError(true, false);
    return false;
  }
  return true;
}

void Connector::ReadAllAvailableMessages() {
  while (!error_) {
    base::WeakPtr<Connector> weak_self = weak_self_;
    MojoResult rv;

    // May delete |this.|
    if (!ReadSingleMessage(&rv))
      return;

    if (!weak_self || paused_)
      return;

    DCHECK(rv == MOJO_RESULT_OK || rv == MOJO_RESULT_SHOULD_WAIT);

    if (rv == MOJO_RESULT_SHOULD_WAIT) {
      // Attempt to re-arm the Watcher.
      MojoResult ready_result;
      MojoResult arm_result = handle_watcher_->Arm(&ready_result);
      if (arm_result == MOJO_RESULT_OK)
        return;

      // The watcher is already ready to notify again.
      DCHECK_EQ(MOJO_RESULT_FAILED_PRECONDITION, arm_result);

      if (ready_result == MOJO_RESULT_FAILED_PRECONDITION) {
        HandleError(false, false);
        return;
      }

      // There's more to read now, so we'll just keep looping.
      DCHECK_EQ(MOJO_RESULT_OK, ready_result);
    }
  }
}

void Connector::CancelWait() {
  peer_remoteness_tracker_.reset();
  handle_watcher_.reset();
  sync_watcher_.reset();
}

void Connector::HandleError(bool force_pipe_reset, bool force_async_handler) {
  if (error_ || !message_pipe_.is_valid())
    return;

  if (paused_) {
    // Enforce calling the error handler asynchronously if the user has paused
    // receiving messages. We need to wait until the user starts receiving
    // messages again.
    force_async_handler = true;
  }

  if (!force_pipe_reset && force_async_handler)
    force_pipe_reset = true;

  if (force_pipe_reset) {
    CancelWait();
    internal::MayAutoLock locker(&lock_);
    message_pipe_.reset();
    MessagePipe dummy_pipe;
    message_pipe_ = std::move(dummy_pipe.handle0);
  } else {
    CancelWait();
  }

  if (force_async_handler) {
    if (!paused_)
      WaitToReadMore();
  } else {
    error_ = true;
    if (connection_error_handler_)
      std::move(connection_error_handler_).Run();
  }
}

void Connector::EnsureSyncWatcherExists() {
  if (sync_watcher_)
    return;
  sync_watcher_.reset(new SyncHandleWatcher(
      message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&Connector::OnSyncHandleWatcherHandleReady,
                 base::Unretained(this))));
}

}  // namespace mojo
