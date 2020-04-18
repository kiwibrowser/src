// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_IMPL_H_
#define CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_IMPL_H_

#include <stdint.h>

#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/renderer/fileapi/webfilewriter_base.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

// An implementation of WebFileWriter for use in chrome renderers and workers.
class WebFileWriterImpl : public WebFileWriterBase,
                          public base::SupportsWeakPtr<WebFileWriterImpl> {
 public:
  enum Type {
    TYPE_SYNC,
    TYPE_ASYNC,
  };

  WebFileWriterImpl(const GURL& path,
                    blink::WebFileWriterClient* client,
                    Type type,
                    const scoped_refptr<base::SingleThreadTaskRunner>&
                        main_thread_task_runner);
  ~WebFileWriterImpl() override;

 protected:
  // WebFileWriterBase overrides
  void DoTruncate(const GURL& path, int64_t offset) override;
  void DoWrite(const GURL& path,
               const std::string& blob_id,
               int64_t offset) override;
  void DoCancel() override;

 private:
  class WriterBridge;

  void RunOnMainThread(const base::Closure& closure);

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<WriterBridge> bridge_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_IMPL_H_
