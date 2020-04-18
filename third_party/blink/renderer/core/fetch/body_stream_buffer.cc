// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"

#include <memory>
#include "base/auto_reset.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body.h"
#include "third_party/blink/renderer/core/fetch/readable_stream_bytes_consumer.h"
#include "third_party/blink/renderer/core/streams/readable_stream_default_controller_wrapper.h"
#include "third_party/blink/renderer/core/streams/readable_stream_operations.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class BodyStreamBuffer::LoaderClient final
    : public GarbageCollectedFinalized<LoaderClient>,
      public ContextLifecycleObserver,
      public FetchDataLoader::Client {
  USING_GARBAGE_COLLECTED_MIXIN(LoaderClient);

 public:
  LoaderClient(ExecutionContext* execution_context,
               BodyStreamBuffer* buffer,
               FetchDataLoader::Client* client)
      : ContextLifecycleObserver(execution_context),
        buffer_(buffer),
        client_(client) {}

  void DidFetchDataLoadedBlobHandle(
      scoped_refptr<BlobDataHandle> blob_data_handle) override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedBlobHandle(std::move(blob_data_handle));
  }

  void DidFetchDataLoadedArrayBuffer(DOMArrayBuffer* array_buffer) override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedArrayBuffer(array_buffer);
  }

  void DidFetchDataLoadedFormData(FormData* form_data) override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedFormData(form_data);
  }

  void DidFetchDataLoadedString(const String& string) override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedString(string);
  }

  void DidFetchDataLoadedDataPipe() override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedDataPipe();
  }

  void DidFetchDataLoadedCustomFormat() override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadedCustomFormat();
  }

  void DidFetchDataLoadFailed() override {
    buffer_->EndLoading();
    client_->DidFetchDataLoadFailed();
  }

  void Abort() override { NOTREACHED(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(buffer_);
    visitor->Trace(client_);
    ContextLifecycleObserver::Trace(visitor);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  void ContextDestroyed(ExecutionContext*) override { buffer_->StopLoading(); }

  Member<BodyStreamBuffer> buffer_;
  Member<FetchDataLoader::Client> client_;
  DISALLOW_COPY_AND_ASSIGN(LoaderClient);
};

BodyStreamBuffer::BodyStreamBuffer(ScriptState* script_state,
                                   BytesConsumer* consumer,
                                   AbortSignal* signal)
    : UnderlyingSourceBase(script_state),
      script_state_(script_state),
      consumer_(consumer),
      signal_(signal),
      made_from_readable_stream_(false) {
  v8::Local<v8::Value> body_value = ToV8(this, script_state);
  DCHECK(!body_value.IsEmpty());
  DCHECK(body_value->IsObject());
  v8::Local<v8::Object> body = body_value.As<v8::Object>();

  ScriptValue readable_stream = ReadableStreamOperations::CreateReadableStream(
      script_state, this,
      ReadableStreamOperations::CreateCountQueuingStrategy(script_state, 0));
  DCHECK(!readable_stream.IsEmpty());
  V8PrivateProperty::GetInternalBodyStream(script_state->GetIsolate())
      .Set(body, readable_stream.V8Value());
  consumer_->SetClient(this);
  if (signal) {
    if (signal->aborted()) {
      Abort();
    } else {
      signal->AddAlgorithm(
          WTF::Bind(&BodyStreamBuffer::Abort, WrapWeakPersistent(this)));
    }
  }
  OnStateChange();
}

BodyStreamBuffer::BodyStreamBuffer(ScriptState* script_state,
                                   ScriptValue stream)
    : UnderlyingSourceBase(script_state),
      script_state_(script_state),
      signal_(nullptr),
      made_from_readable_stream_(true) {
  DCHECK(ReadableStreamOperations::IsReadableStream(script_state, stream));
  v8::Local<v8::Value> body_value = ToV8(this, script_state);
  DCHECK(!body_value.IsEmpty());
  DCHECK(body_value->IsObject());
  v8::Local<v8::Object> body = body_value.As<v8::Object>();

  V8PrivateProperty::GetInternalBodyStream(script_state->GetIsolate())
      .Set(body, stream.V8Value());
}

ScriptValue BodyStreamBuffer::Stream() {
  ScriptState::Scope scope(script_state_.get());
  v8::Local<v8::Value> body_value = ToV8(this, script_state_.get());
  DCHECK(!body_value.IsEmpty());
  DCHECK(body_value->IsObject());
  v8::Local<v8::Object> body = body_value.As<v8::Object>();
  return ScriptValue(
      script_state_.get(),
      V8PrivateProperty::GetInternalBodyStream(script_state_->GetIsolate())
          .GetOrUndefined(body));
}

scoped_refptr<BlobDataHandle> BodyStreamBuffer::DrainAsBlobDataHandle(
    BytesConsumer::BlobSizePolicy policy) {
  DCHECK(!IsStreamLocked());
  DCHECK(!IsStreamDisturbed());
  if (IsStreamClosed() || IsStreamErrored())
    return nullptr;

  if (made_from_readable_stream_)
    return nullptr;

  scoped_refptr<BlobDataHandle> blob_data_handle =
      consumer_->DrainAsBlobDataHandle(policy);
  if (blob_data_handle) {
    CloseAndLockAndDisturb();
    return blob_data_handle;
  }
  return nullptr;
}

scoped_refptr<EncodedFormData> BodyStreamBuffer::DrainAsFormData() {
  DCHECK(!IsStreamLocked());
  DCHECK(!IsStreamDisturbed());
  if (IsStreamClosed() || IsStreamErrored())
    return nullptr;

  if (made_from_readable_stream_)
    return nullptr;

  scoped_refptr<EncodedFormData> form_data = consumer_->DrainAsFormData();
  if (form_data) {
    CloseAndLockAndDisturb();
    return form_data;
  }
  return nullptr;
}

void BodyStreamBuffer::StartLoading(FetchDataLoader* loader,
                                    FetchDataLoader::Client* client) {
  DCHECK(!loader_);
  DCHECK(script_state_->ContextIsValid());
  loader_ = loader;
  if (signal_) {
    if (signal_->aborted()) {
      client->Abort();
      return;
    }
    signal_->AddAlgorithm(
        WTF::Bind(&FetchDataLoader::Client::Abort, WrapWeakPersistent(client)));
  }
  loader->Start(ReleaseHandle(),
                new LoaderClient(ExecutionContext::From(script_state_.get()),
                                 this, client));
}

void BodyStreamBuffer::Tee(BodyStreamBuffer** branch1,
                           BodyStreamBuffer** branch2) {
  DCHECK(!IsStreamLocked());
  DCHECK(!IsStreamDisturbed());
  *branch1 = nullptr;
  *branch2 = nullptr;

  if (made_from_readable_stream_) {
    ScriptValue stream1, stream2;
    ReadableStreamOperations::Tee(script_state_.get(), Stream(), &stream1,
                                  &stream2);
    *branch1 = new BodyStreamBuffer(script_state_.get(), stream1);
    *branch2 = new BodyStreamBuffer(script_state_.get(), stream2);
    return;
  }
  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(ExecutionContext::From(script_state_.get()),
                     ReleaseHandle(), &dest1, &dest2);
  *branch1 = new BodyStreamBuffer(script_state_.get(), dest1, signal_);
  *branch2 = new BodyStreamBuffer(script_state_.get(), dest2, signal_);
}

ScriptPromise BodyStreamBuffer::pull(ScriptState* script_state) {
  DCHECK_EQ(script_state, script_state_.get());
  if (!consumer_) {
    // This is a speculative workaround for a crash. See
    // https://crbug.com/773525.
    // TODO(yhirano): Remove this branch or have a better comment.
    return ScriptPromise::CastUndefined(script_state);
  }

  if (stream_needs_more_)
    return ScriptPromise::CastUndefined(script_state);
  stream_needs_more_ = true;
  if (!in_process_data_)
    ProcessData();
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise BodyStreamBuffer::Cancel(ScriptState* script_state,
                                       ScriptValue reason) {
  DCHECK_EQ(script_state, script_state_.get());
  if (Controller())
    Controller()->Close();
  CancelConsumer();
  return ScriptPromise::CastUndefined(script_state);
}

void BodyStreamBuffer::OnStateChange() {
  if (!consumer_ || !GetExecutionContext() ||
      GetExecutionContext()->IsContextDestroyed())
    return;

  switch (consumer_->GetPublicState()) {
    case BytesConsumer::PublicState::kReadableOrWaiting:
      break;
    case BytesConsumer::PublicState::kClosed:
      Close();
      return;
    case BytesConsumer::PublicState::kErrored:
      GetError();
      return;
  }
  ProcessData();
}

bool BodyStreamBuffer::HasPendingActivity() const {
  if (loader_)
    return true;
  return UnderlyingSourceBase::HasPendingActivity();
}

void BodyStreamBuffer::ContextDestroyed(ExecutionContext* destroyed_context) {
  CancelConsumer();
  UnderlyingSourceBase::ContextDestroyed(destroyed_context);
}

bool BodyStreamBuffer::IsStreamReadable() {
  ScriptState::Scope scope(script_state_.get());
  return ReadableStreamOperations::IsReadable(script_state_.get(), Stream());
}

bool BodyStreamBuffer::IsStreamClosed() {
  ScriptState::Scope scope(script_state_.get());
  return ReadableStreamOperations::IsClosed(script_state_.get(), Stream());
}

bool BodyStreamBuffer::IsStreamErrored() {
  ScriptState::Scope scope(script_state_.get());
  return ReadableStreamOperations::IsErrored(script_state_.get(), Stream());
}

bool BodyStreamBuffer::IsStreamLocked() {
  ScriptState::Scope scope(script_state_.get());
  return ReadableStreamOperations::IsLocked(script_state_.get(), Stream());
}

bool BodyStreamBuffer::IsStreamDisturbed() {
  ScriptState::Scope scope(script_state_.get());
  return ReadableStreamOperations::IsDisturbed(script_state_.get(), Stream());
}

void BodyStreamBuffer::CloseAndLockAndDisturb() {
  if (IsStreamReadable()) {
    // Note that the stream cannot be "draining", because it doesn't have
    // the internal buffer.
    Close();
  }

  ScriptState::Scope scope(script_state_.get());
  NonThrowableExceptionState exception_state;
  ScriptValue reader = ReadableStreamOperations::GetReader(
      script_state_.get(), Stream(), exception_state);
  ReadableStreamOperations::DefaultReaderRead(script_state_.get(), reader);
}

bool BodyStreamBuffer::IsAborted() {
  if (!signal_)
    return false;
  return signal_->aborted();
}

void BodyStreamBuffer::Abort() {
  Controller()->GetError(DOMException::Create(kAbortError));
  CancelConsumer();
}

void BodyStreamBuffer::Close() {
  Controller()->Close();
  CancelConsumer();
}

void BodyStreamBuffer::GetError() {
  {
    ScriptState::Scope scope(script_state_.get());
    Controller()->GetError(V8ThrowException::CreateTypeError(
        script_state_->GetIsolate(), "network error"));
  }
  CancelConsumer();
}

void BodyStreamBuffer::CancelConsumer() {
  if (consumer_) {
    consumer_->Cancel();
    consumer_ = nullptr;
  }
}

void BodyStreamBuffer::ProcessData() {
  DCHECK(consumer_);
  DCHECK(!in_process_data_);

  base::AutoReset<bool> auto_reset(&in_process_data_, true);
  while (stream_needs_more_) {
    const char* buffer = nullptr;
    size_t available = 0;
    auto result = consumer_->BeginRead(&buffer, &available);
    if (result == BytesConsumer::Result::kShouldWait)
      return;
    DOMUint8Array* array = nullptr;
    if (result == BytesConsumer::Result::kOk) {
      array = DOMUint8Array::Create(
          reinterpret_cast<const unsigned char*>(buffer), available);
      result = consumer_->EndRead(available);
    }
    switch (result) {
      case BytesConsumer::Result::kOk:
      case BytesConsumer::Result::kDone:
        if (array) {
          // Clear m_streamNeedsMore in order to detect a pull call.
          stream_needs_more_ = false;
          Controller()->Enqueue(array);
        }
        if (result == BytesConsumer::Result::kDone) {
          Close();
          return;
        }
        // If m_streamNeedsMore is true, it means that pull is called and
        // the stream needs more data even if the desired size is not
        // positive.
        if (!stream_needs_more_)
          stream_needs_more_ = Controller()->DesiredSize() > 0;
        break;
      case BytesConsumer::Result::kShouldWait:
        NOTREACHED();
        return;
      case BytesConsumer::Result::kError:
        GetError();
        return;
    }
  }
}

void BodyStreamBuffer::EndLoading() {
  DCHECK(loader_);
  loader_ = nullptr;
}

void BodyStreamBuffer::StopLoading() {
  if (!loader_)
    return;
  loader_->Cancel();
  loader_ = nullptr;
}

BytesConsumer* BodyStreamBuffer::ReleaseHandle() {
  DCHECK(!IsStreamLocked());
  DCHECK(!IsStreamDisturbed());

  if (made_from_readable_stream_) {
    ScriptState::Scope scope(script_state_.get());
    // We need to have |reader| alive by some means (as written in
    // ReadableStreamDataConsumerHandle). Based on the following facts
    //  - This function is used only from tee and startLoading.
    //  - This branch cannot be taken when called from tee.
    //  - startLoading makes hasPendingActivity return true while loading.
    // , we don't need to keep the reader explicitly.
    NonThrowableExceptionState exception_state;
    ScriptValue reader = ReadableStreamOperations::GetReader(
        script_state_.get(), Stream(), exception_state);
    return new ReadableStreamBytesConsumer(script_state_.get(), reader);
  }
  // We need to call these before calling closeAndLockAndDisturb.
  const bool is_closed = IsStreamClosed();
  const bool is_errored = IsStreamErrored();
  BytesConsumer* consumer = consumer_.Release();

  CloseAndLockAndDisturb();

  if (is_closed) {
    // Note that the stream cannot be "draining", because it doesn't have
    // the internal buffer.
    return BytesConsumer::CreateClosed();
  }
  if (is_errored)
    return BytesConsumer::CreateErrored(BytesConsumer::Error("error"));

  DCHECK(consumer);
  consumer->ClearClient();
  return consumer;
}

}  // namespace blink
