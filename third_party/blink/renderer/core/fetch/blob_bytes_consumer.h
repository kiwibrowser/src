// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BLOB_BYTES_CONSUMER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BLOB_BYTES_CONSUMER_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class BlobDataHandle;
class EncodedFormData;
class ExecutionContext;
class ThreadableLoader;
class WebDataConsumerHandle;

// A BlobBytesConsumer is created from a blob handle and it will
// return a valid handle from drainAsBlobDataHandle as much as possible.
class CORE_EXPORT BlobBytesConsumer final : public BytesConsumer,
                                            public ContextLifecycleObserver,
                                            public BytesConsumer::Client,
                                            public ThreadableLoaderClient {
  USING_GARBAGE_COLLECTED_MIXIN(BlobBytesConsumer);
  USING_PRE_FINALIZER(BlobBytesConsumer, Cancel);

 public:
  // |handle| can be null. In that case this consumer gets closed.
  BlobBytesConsumer(ExecutionContext*,
                    scoped_refptr<BlobDataHandle> /* handle */);
  ~BlobBytesConsumer() override;

  // BytesConsumer implementation
  Result BeginRead(const char** buffer, size_t* available) override;
  Result EndRead(size_t read_size) override;
  scoped_refptr<BlobDataHandle> DrainAsBlobDataHandle(BlobSizePolicy) override;
  scoped_refptr<EncodedFormData> DrainAsFormData() override;
  void SetClient(BytesConsumer::Client*) override;
  void ClearClient() override;
  void Cancel() override;
  PublicState GetPublicState() const override;
  Error GetError() const override;
  String DebugName() const override { return "BlobBytesConsumer"; }

  // ContextLifecycleObserver implementation
  void ContextDestroyed(ExecutionContext*) override;

  // BytesConsumer::Client implementation
  void OnStateChange() override;

  // ThreadableLoaderClient implementation
  void DidReceiveResponse(unsigned long identifier,
                          const ResourceResponse&,
                          std::unique_ptr<WebDataConsumerHandle>) override;
  void DidFinishLoading(unsigned long identifier) override;
  void DidFail(const ResourceError&) override;
  void DidFailRedirectCheck() override;

  void Trace(blink::Visitor*) override;

  static BlobBytesConsumer* CreateForTesting(ExecutionContext*,
                                             scoped_refptr<BlobDataHandle>,
                                             ThreadableLoader*);

 private:
  BlobBytesConsumer(ExecutionContext*,
                    scoped_refptr<BlobDataHandle>,
                    ThreadableLoader*);
  ThreadableLoader* CreateLoader();
  void DidFailInternal();
  bool IsClean() const { return blob_data_handle_.get(); }
  void Close();
  void GetError();
  void Clear();

  KURL blob_url_;
  scoped_refptr<BlobDataHandle> blob_data_handle_;
  Member<BytesConsumer> body_;
  Member<BytesConsumer::Client> client_;
  Member<ThreadableLoader> loader_;

  PublicState state_ = PublicState::kReadableOrWaiting;
  // These two booleans are meaningful only when m_state is ReadableOrWaiting.
  bool has_seen_end_of_data_ = false;
  bool has_finished_loading_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BLOB_BYTES_CONSUMER_H_
