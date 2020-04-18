// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_SHARED_MEMORY_DATA_CONSUMER_HANDLE_H_
#define CONTENT_RENDERER_LOADER_SHARED_MEMORY_DATA_CONSUMER_HANDLE_H_

#include <stddef.h>

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/renderer/request_peer.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"

namespace content {

// This class is a WebDataConsumerHandle that accepts RequestPeer::ReceivedData.
class CONTENT_EXPORT SharedMemoryDataConsumerHandle final
    : public blink::WebDataConsumerHandle {
 private:
  class Context;

 public:
  enum BackpressureMode {
    kApplyBackpressure,
    kDoNotApplyBackpressure,
  };

  class CONTENT_EXPORT Writer final {
   public:
    Writer(const scoped_refptr<Context>& context, BackpressureMode mode);
    ~Writer();
    // Note: Writer assumes |AddData| is not called in a client's didGetReadable
    // callback. There isn't such assumption for |Close| and |Fail|.
    void AddData(std::unique_ptr<RequestPeer::ReceivedData> data);
    void Close();
    // TODO(yhirano): Consider providing error code.
    void Fail();

   private:
    scoped_refptr<Context> context_;
    BackpressureMode mode_;

    DISALLOW_COPY_AND_ASSIGN(Writer);
  };

  class ReaderImpl final : public Reader {
   public:
    ReaderImpl(scoped_refptr<Context> context, Client* client);
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
    scoped_refptr<Context> context_;

    DISALLOW_COPY_AND_ASSIGN(ReaderImpl);
  };

  // Creates a handle and a writer associated with the handle. The created
  // writer should be used on the calling thread.
  SharedMemoryDataConsumerHandle(BackpressureMode mode,
                                 std::unique_ptr<Writer>* writer);
  // |on_reader_detached| will be called aynchronously on the calling thread
  // when the reader (including the handle) is detached (i.e. both the handle
  // and the reader are destructed). The callback will be reset in the internal
  // context when the writer is detached, i.e. |Close| or |Fail| is called,
  // and the callback will never be called.
  SharedMemoryDataConsumerHandle(BackpressureMode mode,
                                 const base::Closure& on_reader_detached,
                                 std::unique_ptr<Writer>* writer);
  ~SharedMemoryDataConsumerHandle() override;

  std::unique_ptr<Reader> ObtainReader(
      Client* client,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;

 private:
  const char* DebugName() const override;

  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryDataConsumerHandle);
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_SHARED_MEMORY_DATA_CONSUMER_HANDLE_H_
