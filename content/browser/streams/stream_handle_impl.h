// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_HANDLE_IMPL_H_
#define CONTENT_BROWSER_STREAMS_STREAM_HANDLE_IMPL_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/stream_handle.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

class Stream;

class StreamHandleImpl : public StreamHandle {
 public:
  StreamHandleImpl(const base::WeakPtr<Stream>& stream);
  ~StreamHandleImpl() override;

 private:
  // StreamHandle overrides
  const GURL& GetURL() override;
  void AddCloseListener(const base::Closure& callback) override;

  base::WeakPtr<Stream> stream_;
  GURL url_;
  base::SingleThreadTaskRunner* stream_task_runner_;
  std::vector<base::Closure> close_listeners_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_HANDLE_IMPL_H_

