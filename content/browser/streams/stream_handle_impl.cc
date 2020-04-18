// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/streams/stream_handle_impl.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/streams/stream.h"

namespace content {

namespace {

void RunCloseListeners(const std::vector<base::Closure>& close_listeners) {
  for (size_t i = 0; i < close_listeners.size(); ++i)
    close_listeners[i].Run();
}

}  // namespace

StreamHandleImpl::StreamHandleImpl(const base::WeakPtr<Stream>& stream)
    : stream_(stream),
      url_(stream->url()),
      stream_task_runner_(base::ThreadTaskRunnerHandle::Get().get()) {
}

StreamHandleImpl::~StreamHandleImpl() {
  stream_task_runner_->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&Stream::CloseHandle, stream_),
      base::BindOnce(&RunCloseListeners, close_listeners_));
}

const GURL& StreamHandleImpl::GetURL() {
  return url_;
}

void StreamHandleImpl::AddCloseListener(const base::Closure& callback) {
  close_listeners_.push_back(callback);
}

}  // namespace content
