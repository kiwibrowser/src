// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Defines callback types for the invalidation client library.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_

#include "base/callback.h"

#define INVALIDATION_CALLBACK1_TYPE(Arg1) ::Callback1<Arg1>

namespace invalidation {

using ::Closure;
using ::DoNothing;
using ::NewPermanentCallback;

template <class T>
bool IsCallbackRepeatable(const T* callback) {
  return callback->IsRepeatable();
}

// Encapsulates a callback and its argument.  Deletes the inner callback when it
// is itself deleted, regardless of whether it is ever run.
template<typename ArgumentType>
class CallbackWrapper : public Closure {
 public:
  // Constructs a new CallbackWrapper, which takes ownership of the inner
  // callback.
  CallbackWrapper(
      INVALIDATION_CALLBACK1_TYPE(ArgumentType)* callback, ArgumentType arg) :
      callback_(callback), arg_(arg) {}

  virtual ~CallbackWrapper() {
    delete callback_;
  }

  // Returns whether the inner callback is repeatable.
  virtual bool IsRepeatable() const {
    return callback_->IsRepeatable();
  }

  // Runs the inner callback on the argument.
  virtual void Run() {
    callback_->Run(arg_);
  }

 private:
  // The callback to run.
  INVALIDATION_CALLBACK1_TYPE(ArgumentType)* callback_;
  // The argument on which to run it.
  ArgumentType arg_;
};

// An override of NewPermanentCallback that wraps a callback and its argument,
// transferring ownership of the inner callback to the new one.  We define this
// here (in deps/callback.h), along with the class above, because the Google
// implementation of callbacks is much different from the one used in Chrome.  A
// Chrome Closure's destructor and Run() method are not virtual, so we can't
// define custom implementations (as above in CallbackWrapper) to get the
// semantics and memory management behavior we want.
template <typename ArgType>
Closure* NewPermanentCallback(
    INVALIDATION_CALLBACK1_TYPE(ArgType)* callback, ArgType arg) {
  return new CallbackWrapper<ArgType>(callback, arg);
}

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_CALLBACK_H_
