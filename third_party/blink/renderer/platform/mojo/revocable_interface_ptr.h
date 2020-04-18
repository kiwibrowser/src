// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_INTERFACE_PTR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_INTERFACE_PTR_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/connection_error_callback.h"
#include "mojo/public/cpp/bindings/interface_ptr_info.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/lib/interface_ptr_state.h"
#include "third_party/blink/renderer/platform/mojo/interface_invalidator.h"

namespace blink {

// RevocableInterfacePtr is a wrapper around an InterfacePtr that has to be tied
// to an InterfaceInvalidator when bound to a message pipe. The underlying
// connection is closed once the InterfaceInvalidator is destroyed and the
// interface will behave as if its peer had closed the connection. This is
// useful for tying the lifetime of interface pointers to another object.
template <typename Interface>
class RevocableInterfacePtr : public InterfaceInvalidator::Observer {
 public:
  using PtrType = mojo::InterfacePtr<Interface>;
  using PtrInfoType = mojo::InterfacePtrInfo<Interface>;
  using Proxy = typename Interface::Proxy_;

  // Constructs an unbound RevocableInterfacePtr.
  RevocableInterfacePtr() {}
  RevocableInterfacePtr(std::nullptr_t) {}

  // Takes over the binding of another RevocableInterfacePtr.
  RevocableInterfacePtr(RevocableInterfacePtr&& other) {
    interface_ptr_ = std::move(other.interface_ptr_);
    SetInvalidator(other.invalidator_.get());
    // Reset the other interface ptr to remove it as an observer of the
    // invalidator.
    other.reset();
  }

  RevocableInterfacePtr(PtrInfoType info, InterfaceInvalidator* invalidator) {
    Bind(std::move(info), invalidator);
  }

  // Takes over the binding of another RevocableInterfacePtr, and closes any
  // message pipe already bound to this pointer.
  RevocableInterfacePtr& operator=(RevocableInterfacePtr&& other) {
    reset();
    interface_ptr_ = std::move(other.interface_ptr);
    SetInvalidator(other.invalidator_.get());
    // Reset the other interface ptr to remove it as an observer of the
    // invalidator.
    other.reset();
    return *this;
  }

  // Assigning nullptr to this class causes it to close the currently bound
  // message pipe (if any) and returns the pointer to the unbound state.
  RevocableInterfacePtr& operator=(decltype(nullptr)) {
    reset();
    return *this;
  }

  // Closes the bound message pipe (if any) on destruction.
  ~RevocableInterfacePtr() {
    if (invalidator_) {
      invalidator_->RemoveObserver(this);
    }
  }

  // Binds the RevocableInterfacePtr to a remote implementation of Interface.
  //
  // Calling with an invalid |info| (containing an invalid message pipe handle)
  // has the same effect as reset(). In this case, the InterfacePtr is not
  // considered as bound.
  //
  // |runner| must belong to the same thread. It will be used to dispatch all
  // callbacks and connection error notification. It is useful when you attach
  // multiple task runners to a single thread for the purposes of task
  // scheduling.
  void Bind(PtrInfoType info,
            InterfaceInvalidator* invalidator,
            scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
    DCHECK(invalidator);
    reset();
    if (info.is_valid()) {
      interface_ptr_.Bind(std::move(info), std::move(runner));
      invalidator_ = invalidator->GetWeakPtr();
      invalidator_->AddObserver(this);
    }
  }

  // Returns a raw pointer to the local proxy. Caller does not take ownership.
  // Note that the local proxy is thread hostile, as stated above.
  Proxy* get() const { return interface_ptr_.get(); }

  // Functions like a pointer to Interface. Must already be bound.
  Proxy* operator->() const { return get(); }
  Proxy& operator*() const { return *get(); }

  // Returns the version number of the interface that the remote side supports.
  uint32_t version() const { return interface_ptr_.version(); }

  // Queries the max version that the remote side supports. On completion, the
  // result will be returned as the input of |callback|. The version number of
  // this interface pointer will also be updated.
  void QueryVersion(const base::RepeatingCallback<void(uint32_t)>& callback) {
    interface_ptr_.QueryVersion(callback);
  }

  // If the remote side doesn't support the specified version, it will close its
  // end of the message pipe asynchronously. This does nothing if it's already
  // known that the remote side supports the specified version, i.e., if
  // |version <= this->version()|.
  //
  // After calling RequireVersion() with a version not supported by the remote
  // side, all subsequent calls to interface methods will be ignored.
  void RequireVersion(uint32_t version) {
    interface_ptr_.RequireVersion(version);
  }

  // Sends a no-op message on the underlying message pipe and runs the current
  // message loop until its response is received. This can be used in tests to
  // verify that no message was sent on a message pipe in response to some
  // stimulus.
  void FlushForTesting() { interface_ptr_.FlushForTesting(); }

  // Closes the bound message pipe, if any.
  void reset() {
    interface_ptr_.reset();
    SetInvalidator(nullptr);
  }

  // Similar to the method above, but also specifies a disconnect reason.
  void ResetWithReason(uint32_t custom_reason, const std::string& description) {
    interface_ptr_.ResetWithReason(custom_reason, description);
    SetInvalidator(nullptr);
  }

  // Whether there are any associated interfaces running on the pipe currently.
  bool HasAssociatedInterfaces() const {
    return interface_ptr_.HasAssociatedInterfaces();
  }

  // Indicates whether the message pipe has encountered an error. If true,
  // method calls made on this interface will be dropped (and may already have
  // been dropped).
  bool encountered_error() const { return interface_ptr_.encountered_error(); }

  // Registers a handler to receive error notifications. The handler will be
  // called from the sequence that owns this RevocableInterfacePtr.
  //
  // This method may only be called after the RevocableInterfacePtr has been
  // bound to a message pipe.
  void set_connection_error_handler(base::OnceClosure error_handler) {
    interface_ptr_.set_connection_error_handler(std::move(error_handler));
  }

  void set_connection_error_with_reason_handler(
      mojo::ConnectionErrorWithReasonCallback error_handler) {
    interface_ptr_.set_connection_error_with_reason_handler(
        std::move(error_handler));
  }

  // Unbinds the RevocableInterfacePtr and returns the information which could
  // be used to setup a RevocableInterfacePtr again. See comments on
  // InterfacePtr::PassInterface for details.
  PtrInfoType PassInterface() {
    SetInvalidator(nullptr);
    return interface_ptr_.PassInterface();
  }

  bool operator==(const RevocableInterfacePtr& other) const {
    if (this == &other)
      return true;

    // Now that the two refer to different objects, they are equivalent if
    // and only if they are both null.
    return !(*this) && !other;
  }

  // Allow RevocableInterfacePtr<> to be used in boolean expressions.
  explicit operator bool() const { return static_cast<bool>(interface_ptr_); }

 private:
  // InterfaceInvalidator::Observer
  void OnInvalidate() override {
    interface_ptr_.internal_state()->RaiseError();
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

  PtrType interface_ptr_;
  base::WeakPtr<InterfaceInvalidator> invalidator_;

  DISALLOW_COPY_AND_ASSIGN(RevocableInterfacePtr);
};

template <typename Interface>
mojo::InterfaceRequest<Interface> MakeRequest(
    RevocableInterfacePtr<Interface>* ptr,
    InterfaceInvalidator* invalidator,
    scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
  mojo::MessagePipe pipe;
  ptr->Bind(mojo::InterfacePtrInfo<Interface>(std::move(pipe.handle0), 0u),
            invalidator, std::move(runner));
  return mojo::InterfaceRequest<Interface>(std::move(pipe.handle1));
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_REVOCABLE_INTERFACE_PTR_H_
