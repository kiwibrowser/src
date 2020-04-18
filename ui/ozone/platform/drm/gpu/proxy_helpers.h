// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_PROXY_HELPERS_H_
#define UI_OZONE_PLATFORM_DRM_GPU_PROXY_HELPERS_H_

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_task_runner_handle.h"

namespace ui {

namespace internal {

template <typename... Args>
void PostAsyncTask(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const base::Callback<void(Args...)>& callback,
    Args... args) {
  task_runner->PostTask(FROM_HERE, base::BindOnce(callback, args...));
}

template <typename... Args>
void PostAsyncTaskOnce(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::OnceCallback<void(Args...)> callback,
    Args... args) {
  auto closure = base::BindOnce(std::move(callback), std::move(args)...);
  task_runner->PostTask(FROM_HERE, std::move(closure));
}

}  // namespace internal

// Posts a task to a different thread and blocks waiting for the task to finish
// executing.
void PostSyncTask(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::OnceClosure callback);

// Creates a callback that will run |callback| on the calling thread. Useful
// when posting a task on a different thread and expecting a callback when the
// task finished (and the callback needs to run on the original thread).
template <typename... Args>
base::Callback<void(Args...)> CreateSafeCallback(
    const base::Callback<void(Args...)>& callback) {
  return base::Bind(&internal::PostAsyncTask<Args...>,
                    base::ThreadTaskRunnerHandle::Get(), callback);
}

// Creates a OnceCallback that will run |callback| on the calling thread. Useful
// when posting a task on a different thread and expecting a callback when the
// task finished (and the callback needs to run on the original thread).
template <typename... Args>
base::OnceCallback<void(Args...)> CreateSafeOnceCallback(
    base::OnceCallback<void(Args...)> callback) {
  return base::BindOnce(&internal::PostAsyncTaskOnce<Args...>,
                        base::ThreadTaskRunnerHandle::Get(),
                        std::move(callback));
}

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_PROXY_HELPERS_H_
