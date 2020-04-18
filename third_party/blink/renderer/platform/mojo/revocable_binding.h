// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_BINDING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_BINDING_H_

#include <utility>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/connection_error_callback.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/lib/binding_state.h"
#include "mojo/public/cpp/bindings/raw_ptr_impl_ref_traits.h"
#include "mojo/public/cpp/system/core.h"
#include "third_party/blink/renderer/platform/mojo/interface_invalidator.h"

namespace blink {

class MessageReceiver;

// RevocableBinding is a wrapper around a Binding that has to be tied to an
// InterfaceInvalidator when bound to a message pipe. The underlying connection
// is automatically closed once the InterfaceInvalidator is destroyed, and the
// binding will behave as if its peer had closed the connection. This is useful
// for tying the lifetime of mojo interfaces to another object.
//
// TODO(austinct): Add set_connection_error_with_reason_handler(),
// CloseWithReason(), ReportBadMessage() and GetBadMessageCallback() methods if
// needed. Undesirable for now because of the std::string parameter.
template <typename Interface,
          typename ImplRefTraits = mojo::RawPtrImplRefTraits<Interface>>
class RevocableBinding : public InterfaceInvalidator::Observer {
 public:
  using ImplPointerType = typename ImplRefTraits::PointerType;

  // Constructs an incomplete binding that will use the implementation |impl|.
  // The binding may be completed with a subsequent call to the |Bind| method.
  // Does not take ownership of |impl|, which must outlive the binding.
  explicit RevocableBinding(ImplPointerType impl) : binding_(std::move(impl)) {}

  // Constructs a completed binding of |impl| to the message pipe endpoint in
  // |request|, taking ownership of the endpoint. Does not take ownership of
  // |impl|, which must outlive the binding. Ties the lifetime of the binding to
  // |invalidator|.
  RevocableBinding(ImplPointerType impl,
                   mojo::InterfaceRequest<Interface> request,
                   InterfaceInvalidator* invalidator,
                   scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr)
      : RevocableBinding(std::move(impl)) {
    Bind(std::move(request), invalidator, std::move(runner));
  }

  // Tears down the binding, closing the message pipe and leaving the interface
  // implementation unbound.
  ~RevocableBinding() { SetInvalidator(nullptr); }

  // Completes a binding that was constructed with only an interface
  // implementation by removing the message pipe endpoint from |request| and
  // binding it to the previously specified implementation. Ties the lifetime of
  // the binding to |invalidator|.
  void Bind(mojo::InterfaceRequest<Interface> request,
            InterfaceInvalidator* invalidator,
            scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
    DCHECK(invalidator);
    binding_.Bind(std::move(request), std::move(runner));
    SetInvalidator(invalidator);
  }

  // Adds a message filter to be notified of each incoming message before
  // dispatch. If a filter returns |false| from Accept(), the message is not
  // dispatched and the pipe is closed. Filters cannot be removed.
  void AddFilter(std::unique_ptr<MessageReceiver> filter) {
    binding_.AddFilter(std::move(filter));
  }

  // Whether there are any associated interfaces running on the pipe currently.
  bool HasAssociatedInterfaces() const {
    return binding_.HasAssociatedInterfaces();
  }

  // Stops processing incoming messages until
  // ResumeIncomingMethodCallProcessing().
  // Outgoing messages are still sent.
  //
  // No errors are detected on the message pipe while paused.
  //
  // This method may only be called if the object has been bound to a message
  // pipe and there are no associated interfaces running.
  void PauseIncomingMethodCallProcessing() {
    binding_.PauseIncomingMethodCallProcessing();
  }
  void ResumeIncomingMethodCallProcessing() {
    binding_.ResumeIncomingMethodCallProcessing();
  }

  // Closes the message pipe that was previously bound. Put this object into a
  // state where it can be rebound to a new pipe.
  void Close() {
    SetInvalidator(nullptr);
    binding_.Close();
  }

  // Unbinds the underlying pipe from this binding and returns it so it can be
  // used in another context, such as on another sequence or with a different
  // implementation. Put this object into a state where it can be rebound to a
  // new pipe.
  //
  // This method may only be called if the object has been bound to a message
  // pipe and there are no associated interfaces running.
  //
  // TODO(yzshen): For now, users need to make sure there is no one holding
  // on to associated interface endpoint handles at both sides of the
  // message pipe in order to call this method. We need a way to forcefully
  // invalidate associated interface endpoint handles.
  mojo::InterfaceRequest<Interface> Unbind() {
    SetInvalidator(nullptr);
    return binding_.Unbind();
  }

  // Sets an error handler that will be called if a connection error occurs on
  // the bound message pipe.
  //
  // This method may only be called after this RevocableBinding has been bound
  // to a message pipe. The error handler will be reset when this
  // RevocableBinding is unbound, closed or invalidated.
  void set_connection_error_handler(base::OnceClosure error_handler) {
    binding_.set_connection_error_handler(std::move(error_handler));
  }

  // Returns the interface implementation that was previously specified. Caller
  // does not take ownership.
  Interface* impl() { return binding_.impl(); }

  // Indicates whether the binding has been completed (i.e., whether a message
  // pipe has been bound to the implementation).
  explicit operator bool() const { return static_cast<bool>(binding_); }

  // Sends a no-op message on the underlying message pipe and runs the current
  // message loop until its response is received. This can be used in tests to
  // verify that no message was sent on a message pipe in response to some
  // stimulus.
  void FlushForTesting() { binding_.FlushForTesting(); }

 private:
  // InterfaceInvalidator::Observer
  void OnInvalidate() override {
    if (binding_) {
      binding_.internal_state()->RaiseError();
    }
    if (invalidator_) {
      invalidator_->RemoveObserver(this);
    }
    invalidator_.reset();
  }

  // Replaces the existing invalidator with a new invalidator and changes the
  // invalidator being observed.
  void SetInvalidator(InterfaceInvalidator* invalidator) {
    if (invalidator_)
      invalidator_->RemoveObserver(this);

    invalidator_.reset();
    if (invalidator) {
      invalidator_ = invalidator->GetWeakPtr();
      invalidator_->AddObserver(this);
    }
  }

  mojo::Binding<Interface, ImplRefTraits> binding_;
  base::WeakPtr<InterfaceInvalidator> invalidator_;

  DISALLOW_COPY_AND_ASSIGN(RevocableBinding);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_BINDING_H_
