// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_WEB_DATA_CONSUMER_HANDLE_IMPL_H_
#define CONTENT_RENDERER_LOADER_WEB_DATA_CONSUMER_HANDLE_IMPL_H_

#include <stddef.h>

#include <memory>

#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"

namespace content {

class CONTENT_EXPORT WebDataConsumerHandleImpl final
    : public blink::WebDataConsumerHandle {
  typedef mojo::ScopedDataPipeConsumerHandle Handle;
  class Context;

 public:
  class CONTENT_EXPORT ReaderImpl final : public Reader {
   public:
    ReaderImpl(scoped_refptr<Context> context,
               Client* client,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner);
    ~ReaderImpl() override;
    Result Read(void* data,
                size_t size,
                Flags flags,
                size_t* readSize) override;
    Result BeginRead(const void** buffer,
                     Flags flags,
                     size_t* available) override;
    Result EndRead(size_t readSize) override;

   private:
    Result HandleReadResult(MojoResult);
    void StartWatching();
    void OnHandleGotReadable(MojoResult);

    scoped_refptr<Context> context_;
    mojo::SimpleWatcher handle_watcher_;
    Client* client_;

    DISALLOW_COPY_AND_ASSIGN(ReaderImpl);
  };
  std::unique_ptr<Reader> ObtainReader(
      Client* client,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;

  explicit WebDataConsumerHandleImpl(Handle handle);
  ~WebDataConsumerHandleImpl() override;

 private:
  const char* DebugName() const override;

  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(WebDataConsumerHandleImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_WEB_DATA_CONSUMER_HANDLE_IMPL_H_
