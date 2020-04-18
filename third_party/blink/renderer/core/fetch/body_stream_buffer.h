// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BODY_STREAM_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BODY_STREAM_BUFFER_H_

#include <memory>
#include "third_party/blink/public/platform/web_data_consumer_handle.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"
#include "third_party/blink/renderer/core/streams/underlying_source_base.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class EncodedFormData;
class ScriptState;

class CORE_EXPORT BodyStreamBuffer final : public UnderlyingSourceBase,
                                           public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(BodyStreamBuffer);

 public:
  // |consumer| must not have a client.
  // This function must be called with entering an appropriate V8 context.
  // |signal| should be non-null when this BodyStreamBuffer is associated with a
  // Response that was created by fetch().
  BodyStreamBuffer(ScriptState*,
                   BytesConsumer* /* consumer */,
                   AbortSignal* /* signal */);
  // |ReadableStreamOperations::isReadableStream(stream)| must hold.
  // This function must be called with entering an appropriate V8 context.
  BodyStreamBuffer(ScriptState*, ScriptValue stream);

  ScriptValue Stream();

  // Callable only when neither locked nor disturbed.
  scoped_refptr<BlobDataHandle> DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy);
  scoped_refptr<EncodedFormData> DrainAsFormData();
  void StartLoading(FetchDataLoader*, FetchDataLoader::Client* /* client */);
  void Tee(BodyStreamBuffer**, BodyStreamBuffer**);

  // UnderlyingSourceBase
  ScriptPromise pull(ScriptState*) override;
  ScriptPromise Cancel(ScriptState*, ScriptValue reason) override;
  bool HasPendingActivity() const override;
  void ContextDestroyed(ExecutionContext*) override;

  // BytesConsumer::Client
  void OnStateChange() override;
  String DebugName() const override { return "BodyStreamBuffer"; }

  bool IsStreamReadable();
  bool IsStreamClosed();
  bool IsStreamErrored();
  bool IsStreamLocked();
  bool IsStreamDisturbed();
  void CloseAndLockAndDisturb();
  ScriptState* GetScriptState() { return script_state_.get(); }

  bool IsAborted();

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(loader_);
    visitor->Trace(signal_);
    UnderlyingSourceBase::Trace(visitor);
  }

 private:
  class LoaderClient;

  BytesConsumer* ReleaseHandle();
  void Abort();
  void Close();
  void GetError();
  void CancelConsumer();
  void ProcessData();
  void EndLoading();
  void StopLoading();

  scoped_refptr<ScriptState> script_state_;
  Member<BytesConsumer> consumer_;
  // We need this member to keep it alive while loading.
  Member<FetchDataLoader> loader_;
  // We need this to ensure that we detect that abort has been signalled
  // correctly.
  Member<AbortSignal> signal_;
  bool stream_needs_more_ = false;
  bool made_from_readable_stream_;
  bool in_process_data_ = false;
  DISALLOW_COPY_AND_ASSIGN(BodyStreamBuffer);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BODY_STREAM_BUFFER_H_
