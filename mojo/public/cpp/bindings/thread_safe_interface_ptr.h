// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_THREAD_SAFE_INTERFACE_PTR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_THREAD_SAFE_INTERFACE_PTR_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/stl_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "mojo/public/cpp/bindings/associated_group.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"
#include "mojo/public/cpp/bindings/sync_event_watcher.h"

// ThreadSafeInterfacePtr wraps a non-thread-safe InterfacePtr and proxies
// messages to it. Async calls are posted to the sequence that the InteracePtr
// is bound to, and the responses are posted back. Sync calls are dispatched
// directly if the call is made on the sequence that the wrapped InterfacePtr is
// bound to, or posted otherwise. It's important to be aware that sync calls
// block both the calling sequence and the InterfacePtr sequence. That means
// that you cannot make sync calls through a ThreadSafeInterfacePtr if the
// underlying InterfacePtr is bound to a sequence that cannot block, like the IO
// thread.

namespace mojo {

// Instances of this class may be used from any sequence to serialize
// |Interface| messages and forward them elsewhere. In general you should use
// one of the ThreadSafeInterfacePtrBase helper aliases defined below, but this
// type may be useful if you need/want to manually manage the lifetime of the
// underlying proxy object which will be used to ultimately send messages.
template <typename Interface>
class ThreadSafeForwarder : public MessageReceiverWithResponder {
 public:
  using ProxyType = typename Interface::Proxy_;
  using ForwardMessageCallback = base::Callback<void(Message)>;
  using ForwardMessageWithResponderCallback =
      base::Callback<void(Message, std::unique_ptr<MessageReceiver>)>;

  // Constructs a ThreadSafeForwarder through which Messages are forwarded to
  // |forward| or |forward_with_responder| by posting to |task_runner|.
  //
  // Any message sent through this forwarding interface will dispatch its reply,
  // if any, back to the sequence which called the corresponding interface
  // method.
  ThreadSafeForwarder(
      const scoped_refptr<base::SequencedTaskRunner>& task_runner,
      const ForwardMessageCallback& forward,
      const ForwardMessageWithResponderCallback& forward_with_responder,
      const AssociatedGroup& associated_group)
      : proxy_(this),
        task_runner_(task_runner),
        forward_(forward),
        forward_with_responder_(forward_with_responder),
        associated_group_(associated_group),
        sync_calls_(new InProgressSyncCalls()) {}

  ~ThreadSafeForwarder() override {
    // If there are ongoing sync calls signal their completion now.
    base::AutoLock l(sync_calls_->lock);
    for (const auto& pending_response : sync_calls_->pending_responses)
      pending_response->event.Signal();
  }

  ProxyType& proxy() { return proxy_; }

 private:
  // MessageReceiverWithResponder implementation:
  bool PrefersSerializedMessages() override {
    // TSIP is primarily used because it emulates legacy IPC threading behavior.
    // In practice this means it's only for cross-process messaging and we can
    // just always assume messages should be serialized.
    return true;
  }

  bool Accept(Message* message) override {
    if (!message->associated_endpoint_handles()->empty()) {
      // If this DCHECK fails, it is likely because:
      // - This is a non-associated interface pointer setup using
      //     PtrWrapper::BindOnTaskRunner(
      //         InterfacePtrInfo<InterfaceType> ptr_info);
      //   Please see the TODO in that method.
      // - This is an associated interface which hasn't been associated with a
      //   message pipe. In other words, the corresponding
      //   AssociatedInterfaceRequest hasn't been sent.
      DCHECK(associated_group_.GetController());
      message->SerializeAssociatedEndpointHandles(
          associated_group_.GetController());
    }
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(forward_, base::Passed(message)));
    return true;
  }

  bool AcceptWithResponder(
      Message* message,
      std::unique_ptr<MessageReceiver> responder) override {
    if (!message->associated_endpoint_handles()->empty()) {
      // Please see comment for the DCHECK in the previous method.
      DCHECK(associated_group_.GetController());
      message->SerializeAssociatedEndpointHandles(
          associated_group_.GetController());
    }

    // Async messages are always posted (even if |task_runner_| runs tasks on
    // this sequence) to guarantee that two async calls can't be reordered.
    if (!message->has_flag(Message::kFlagIsSync)) {
      auto reply_forwarder =
          std::make_unique<ForwardToCallingThread>(std::move(responder));
      task_runner_->PostTask(
          FROM_HERE, base::Bind(forward_with_responder_, base::Passed(message),
                                base::Passed(&reply_forwarder)));
      return true;
    }

    SyncCallRestrictions::AssertSyncCallAllowed();

    // If the InterfacePtr is bound to this sequence, dispatch it directly.
    if (task_runner_->RunsTasksInCurrentSequence()) {
      forward_with_responder_.Run(std::move(*message), std::move(responder));
      return true;
    }

    // If the InterfacePtr is bound on another sequence, post the call.
    // TODO(yzshen, watk): We block both this sequence and the InterfacePtr
    // sequence. Ideally only this sequence would block.
    auto response = base::MakeRefCounted<SyncResponseInfo>();
    auto response_signaler = std::make_unique<SyncResponseSignaler>(response);
    task_runner_->PostTask(
        FROM_HERE, base::Bind(forward_with_responder_, base::Passed(message),
                              base::Passed(&response_signaler)));

    // Save the pending SyncResponseInfo so that if the sync call deletes
    // |this|, we can signal the completion of the call to return from
    // SyncWatch().
    auto sync_calls = sync_calls_;
    {
      base::AutoLock l(sync_calls->lock);
      sync_calls->pending_responses.push_back(response.get());
    }

    auto assign_true = [](bool* b) { *b = true; };
    bool event_signaled = false;
    SyncEventWatcher watcher(&response->event,
                             base::Bind(assign_true, &event_signaled));
    const bool* stop_flags[] = {&event_signaled};
    watcher.SyncWatch(stop_flags, 1);

    {
      base::AutoLock l(sync_calls->lock);
      base::Erase(sync_calls->pending_responses, response.get());
    }

    if (response->received)
      ignore_result(responder->Accept(&response->message));

    return true;
  }

  // Data that we need to share between the sequences involved in a sync call.
  struct SyncResponseInfo
      : public base::RefCountedThreadSafe<SyncResponseInfo> {
    Message message;
    bool received = false;
    base::WaitableEvent event{base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED};

   private:
    friend class base::RefCountedThreadSafe<SyncResponseInfo>;
  };

  // A MessageReceiver that signals |response| when it either accepts the
  // response message, or is destructed.
  class SyncResponseSignaler : public MessageReceiver {
   public:
    explicit SyncResponseSignaler(scoped_refptr<SyncResponseInfo> response)
        : response_(response) {}

    ~SyncResponseSignaler() override {
      // If Accept() was not called we must still notify the waiter that the
      // sync call is finished.
      if (response_)
        response_->event.Signal();
    }

    bool Accept(Message* message) override {
      response_->message = std::move(*message);
      response_->received = true;
      response_->event.Signal();
      response_ = nullptr;
      return true;
    }

   private:
    scoped_refptr<SyncResponseInfo> response_;
  };

  // A record of the pending sync responses for canceling pending sync calls
  // when the owning ThreadSafeForwarder is destructed.
  struct InProgressSyncCalls
      : public base::RefCountedThreadSafe<InProgressSyncCalls> {
    // |lock| protects access to |pending_responses|.
    base::Lock lock;
    std::vector<SyncResponseInfo*> pending_responses;
  };

  class ForwardToCallingThread : public MessageReceiver {
   public:
    explicit ForwardToCallingThread(std::unique_ptr<MessageReceiver> responder)
        : responder_(std::move(responder)),
          caller_task_runner_(base::SequencedTaskRunnerHandle::Get()) {}
    ~ForwardToCallingThread() override {
      caller_task_runner_->DeleteSoon(FROM_HERE, std::move(responder_));
    }

   private:
    bool Accept(Message* message) override {
      // The current instance will be deleted when this method returns, so we
      // have to relinquish the responder's ownership so it does not get
      // deleted.
      caller_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&ForwardToCallingThread::CallAcceptAndDeleteResponder,
                     base::Passed(std::move(responder_)),
                     base::Passed(std::move(*message))));
      return true;
    }

    static void CallAcceptAndDeleteResponder(
        std::unique_ptr<MessageReceiver> responder,
        Message message) {
      ignore_result(responder->Accept(&message));
    }

    std::unique_ptr<MessageReceiver> responder_;
    scoped_refptr<base::SequencedTaskRunner> caller_task_runner_;
  };

  ProxyType proxy_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;
  const ForwardMessageCallback forward_;
  const ForwardMessageWithResponderCallback forward_with_responder_;
  AssociatedGroup associated_group_;
  scoped_refptr<InProgressSyncCalls> sync_calls_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeForwarder);
};

template <typename InterfacePtrType>
class ThreadSafeInterfacePtrBase
    : public base::RefCountedThreadSafe<
          ThreadSafeInterfacePtrBase<InterfacePtrType>> {
 public:
  using InterfaceType = typename InterfacePtrType::InterfaceType;
  using PtrInfoType = typename InterfacePtrType::PtrInfoType;

  explicit ThreadSafeInterfacePtrBase(
      std::unique_ptr<ThreadSafeForwarder<InterfaceType>> forwarder)
      : forwarder_(std::move(forwarder)) {}

  // Creates a ThreadSafeInterfacePtrBase wrapping an underlying non-thread-safe
  // InterfacePtrType which is bound to the calling sequence. All messages sent
  // via this thread-safe proxy will internally be sent by first posting to this
  // (the calling) sequence's TaskRunner.
  static scoped_refptr<ThreadSafeInterfacePtrBase> Create(
      InterfacePtrType interface_ptr) {
    scoped_refptr<PtrWrapper> wrapper =
        new PtrWrapper(std::move(interface_ptr));
    return new ThreadSafeInterfacePtrBase(wrapper->CreateForwarder());
  }

  // Creates a ThreadSafeInterfacePtrBase which binds the underlying
  // non-thread-safe InterfacePtrType on the specified TaskRunner. All messages
  // sent via this thread-safe proxy will internally be sent by first posting to
  // that TaskRunner.
  static scoped_refptr<ThreadSafeInterfacePtrBase> Create(
      PtrInfoType ptr_info,
      const scoped_refptr<base::SequencedTaskRunner>& bind_task_runner) {
    scoped_refptr<PtrWrapper> wrapper = new PtrWrapper(bind_task_runner);
    wrapper->BindOnTaskRunner(std::move(ptr_info));
    return new ThreadSafeInterfacePtrBase(wrapper->CreateForwarder());
  }

  InterfaceType* get() { return &forwarder_->proxy(); }
  InterfaceType* operator->() { return get(); }
  InterfaceType& operator*() { return *get(); }

 private:
  friend class base::RefCountedThreadSafe<
      ThreadSafeInterfacePtrBase<InterfacePtrType>>;

  struct PtrWrapperDeleter;

  // Helper class which owns an |InterfacePtrType| instance on an appropriate
  // sequence. This is kept alive as long its bound within some
  // ThreadSafeForwarder's callbacks.
  class PtrWrapper
      : public base::RefCountedThreadSafe<PtrWrapper, PtrWrapperDeleter> {
   public:
    explicit PtrWrapper(InterfacePtrType ptr)
        : PtrWrapper(base::SequencedTaskRunnerHandle::Get()) {
      ptr_ = std::move(ptr);
      associated_group_ = *ptr_.internal_state()->associated_group();
    }

    explicit PtrWrapper(
        const scoped_refptr<base::SequencedTaskRunner>& task_runner)
        : task_runner_(task_runner) {}

    void BindOnTaskRunner(AssociatedInterfacePtrInfo<InterfaceType> ptr_info) {
      associated_group_ = AssociatedGroup(ptr_info.handle());
      task_runner_->PostTask(FROM_HERE, base::Bind(&PtrWrapper::Bind, this,
                                                   base::Passed(&ptr_info)));
    }

    void BindOnTaskRunner(InterfacePtrInfo<InterfaceType> ptr_info) {
      // TODO(yzhsen): At the momment we don't have a group controller
      // available. That means the user won't be able to pass associated
      // endpoints on this interface (at least not immediately). In order to fix
      // this, we need to create a MultiplexRouter immediately and bind it to
      // the interface pointer on the |task_runner_|. Therefore, MultiplexRouter
      // should be able to be created on a sequence different than the one that
      // it is supposed to listen on. crbug.com/682334
      task_runner_->PostTask(FROM_HERE, base::Bind(&PtrWrapper::Bind, this,
                                                   base::Passed(&ptr_info)));
    }

    std::unique_ptr<ThreadSafeForwarder<InterfaceType>> CreateForwarder() {
      return std::make_unique<ThreadSafeForwarder<InterfaceType>>(
          task_runner_, base::Bind(&PtrWrapper::Accept, this),
          base::Bind(&PtrWrapper::AcceptWithResponder, this),
          associated_group_);
    }

   private:
    friend struct PtrWrapperDeleter;

    ~PtrWrapper() {}

    void Bind(PtrInfoType ptr_info) {
      DCHECK(task_runner_->RunsTasksInCurrentSequence());
      ptr_.Bind(std::move(ptr_info));
    }

    void Accept(Message message) {
      ptr_.internal_state()->ForwardMessage(std::move(message));
    }

    void AcceptWithResponder(Message message,
                             std::unique_ptr<MessageReceiver> responder) {
      ptr_.internal_state()->ForwardMessageWithResponder(std::move(message),
                                                         std::move(responder));
    }

    void DeleteOnCorrectThread() const {
      if (!task_runner_->RunsTasksInCurrentSequence()) {
        // NOTE: This is only called when there are no more references to
        // |this|, so binding it unretained is both safe and necessary.
        task_runner_->PostTask(FROM_HERE,
                               base::Bind(&PtrWrapper::DeleteOnCorrectThread,
                                          base::Unretained(this)));
      } else {
        delete this;
      }
    }

    InterfacePtrType ptr_;
    const scoped_refptr<base::SequencedTaskRunner> task_runner_;
    AssociatedGroup associated_group_;

    DISALLOW_COPY_AND_ASSIGN(PtrWrapper);
  };

  struct PtrWrapperDeleter {
    static void Destruct(const PtrWrapper* interface_ptr) {
      interface_ptr->DeleteOnCorrectThread();
    }
  };

  ~ThreadSafeInterfacePtrBase() {}

  const std::unique_ptr<ThreadSafeForwarder<InterfaceType>> forwarder_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeInterfacePtrBase);
};

template <typename Interface>
using ThreadSafeAssociatedInterfacePtr =
    ThreadSafeInterfacePtrBase<AssociatedInterfacePtr<Interface>>;

template <typename Interface>
using ThreadSafeInterfacePtr =
    ThreadSafeInterfacePtrBase<InterfacePtr<Interface>>;

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_THREAD_SAFE_INTERFACE_PTR_H_
