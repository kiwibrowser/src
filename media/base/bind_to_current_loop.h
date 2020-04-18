// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_BIND_TO_CURRENT_LOOP_H_
#define MEDIA_BASE_BIND_TO_CURRENT_LOOP_H_

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

// This is a helper utility for base::Bind()ing callbacks to the current
// MessageLoop. The typical use is when |a| (of class |A|) wants to hand a
// callback such as base::Bind(&A::AMethod, a) to |b|, but needs to ensure that
// when |b| executes the callback, it does so on |a|'s current MessageLoop.
//
// Typical usage: request to be called back on the current thread:
// other->StartAsyncProcessAndCallMeBack(
//    media::BindToCurrentLoop(base::BindOnce(&MyClass::MyMethod, this)));
//
// media::BindToCurrentLoop returns the same type of callback to the given
// callback. I.e. it returns a RepeatingCallback for a given RepeatingCallback,
// and returns OnceCallback for a given OnceCallback.

namespace media {
namespace internal {

template <typename Signature, typename... Args>
base::OnceClosure MakeClosure(base::RepeatingCallback<Signature>* callback,
                              Args&&... args) {
  return base::BindOnce(*callback, std::forward<Args>(args)...);
}

template <typename Signature, typename... Args>
base::OnceClosure MakeClosure(base::OnceCallback<Signature>* callback,
                              Args&&... args) {
  return base::BindOnce(std::move(*callback), std::forward<Args>(args)...);
}

template <typename CallbackType>
class TrampolineHelper {
 public:
  TrampolineHelper(const base::Location& posted_from,
                   scoped_refptr<base::SequencedTaskRunner> task_runner,
                   CallbackType callback)
      : posted_from_(posted_from),
        task_runner_(std::move(task_runner)),
        callback_(std::move(callback)) {
    DCHECK(task_runner_);
    DCHECK(callback_);
  }

  template <typename... Args>
  void Run(Args... args) {
    // MakeClosure consumes |callback_| if it's OnceCallback.
    task_runner_->PostTask(
        posted_from_, MakeClosure(&callback_, std::forward<Args>(args)...));
  }

  ~TrampolineHelper() {
    if (callback_) {
      task_runner_->PostTask(
          posted_from_,
          base::BindOnce(&TrampolineHelper::ClearCallbackOnTargetTaskRunner,
                         std::move(callback_)));
    }
  }

 private:
  static void ClearCallbackOnTargetTaskRunner(CallbackType) {}

  base::Location posted_from_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  CallbackType callback_;
};

}  // namespace internal

template <typename... Args>
inline base::RepeatingCallback<void(Args...)> BindToCurrentLoop(
    base::RepeatingCallback<void(Args...)> cb) {
  using CallbackType = base::RepeatingCallback<void(Args...)>;
  using Helper = internal::TrampolineHelper<CallbackType>;
  using RunnerType = void (Helper::*)(Args...);
  RunnerType run = &Helper::Run;
  // TODO(tzik): Propagate FROM_HERE from the caller.
  return base::BindRepeating(
      run, std::make_unique<Helper>(
               FROM_HERE, base::ThreadTaskRunnerHandle::Get(), std::move(cb)));
}

template <typename... Args>
inline base::OnceCallback<void(Args...)> BindToCurrentLoop(
    base::OnceCallback<void(Args...)> cb) {
  using CallbackType = base::OnceCallback<void(Args...)>;
  using Helper = internal::TrampolineHelper<CallbackType>;
  using RunnerType = void (Helper::*)(Args...);
  RunnerType run = &Helper::Run;
  // TODO(tzik): Propagate FROM_HERE from the caller.
  return base::BindOnce(
      run, std::make_unique<Helper>(
               FROM_HERE, base::ThreadTaskRunnerHandle::Get(), std::move(cb)));
}

}  // namespace media

#endif  // MEDIA_BASE_BIND_TO_CURRENT_LOOP_H_
