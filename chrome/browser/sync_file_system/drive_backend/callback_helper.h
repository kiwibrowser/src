// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_HELPER_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_HELPER_H_

#include <memory>
#include <type_traits>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

// TODO(tzik): Merge this file to media/base/bind_to_current_loop.h.

namespace sync_file_system {
namespace drive_backend {

namespace internal {

template <typename T>
base::internal::PassedWrapper<std::unique_ptr<T>> RebindForward(
    std::unique_ptr<T>& t) {
  return base::Passed(&t);
}

template <typename T>
T& RebindForward(T& t) {
  return t;
}

template <typename T>
class CallbackHolder {
 public:
  CallbackHolder(const scoped_refptr<base::SequencedTaskRunner>& task_runner,
                 const base::Location& from_here,
                 const base::Callback<T>& callback)
      : task_runner_(task_runner),
        from_here_(from_here),
        callback_(new base::Callback<T>(callback)) {
    DCHECK(task_runner_.get());
  }

  ~CallbackHolder() {
    base::Callback<T>* callback = callback_.release();
    if (!task_runner_->DeleteSoon(from_here_, callback))
      delete callback;
  }

  base::SequencedTaskRunner* task_runner() const { return task_runner_.get(); }
  const base::Location& from_here() const { return from_here_; }
  const base::Callback<T>& callback() const { return *callback_; }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  const base::Location from_here_;
  std::unique_ptr<base::Callback<T>> callback_;

  DISALLOW_COPY_AND_ASSIGN(CallbackHolder);
};

template <typename>
struct RelayToTaskRunnerHelper;

template <typename... Args>
struct RelayToTaskRunnerHelper<void(Args...)> {
  static void Run(CallbackHolder<void(Args...)>* holder, Args... args) {
    holder->task_runner()->PostTask(
        holder->from_here(),
        base::BindOnce(holder->callback(), RebindForward(args)...));
  }
};

}  // namespace internal

template <typename T>
base::Callback<T> RelayCallbackToTaskRunner(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const base::Location& from_here,
    const base::Callback<T>& callback) {
  DCHECK(task_runner->RunsTasksInCurrentSequence());

  if (callback.is_null())
    return base::Callback<T>();

  return base::Bind(&internal::RelayToTaskRunnerHelper<T>::Run,
                    base::Owned(new internal::CallbackHolder<T>(
                        task_runner, from_here, callback)));
}

template <typename T>
base::Callback<T> RelayCallbackToCurrentThread(
    const base::Location& from_here,
    const base::Callback<T>& callback) {
  return RelayCallbackToTaskRunner(
      base::ThreadTaskRunnerHandle::Get(),
      from_here, callback);
}

}  // namespace drive_backend
}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_HELPER_H_
