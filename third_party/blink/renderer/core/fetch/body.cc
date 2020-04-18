// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/body.h"

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_array_buffer.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/core/url/url_search_params.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/network/parsed_content_type.h"

namespace blink {

namespace {

class BodyConsumerBase : public GarbageCollectedFinalized<BodyConsumerBase>,
                         public FetchDataLoader::Client {
  USING_GARBAGE_COLLECTED_MIXIN(BodyConsumerBase);

 public:
  explicit BodyConsumerBase(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}
  ScriptPromiseResolver* Resolver() { return resolver_; }
  void DidFetchDataLoadFailed() override {
    ScriptState::Scope scope(Resolver()->GetScriptState());
    resolver_->Reject(V8ThrowException::CreateTypeError(
        Resolver()->GetScriptState()->GetIsolate(), "Failed to fetch"));
  }

  void Abort() override {
    resolver_->Reject(DOMException::Create(kAbortError));
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(resolver_);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  Member<ScriptPromiseResolver> resolver_;
  DISALLOW_COPY_AND_ASSIGN(BodyConsumerBase);
};

class BodyBlobConsumer final : public BodyConsumerBase {
 public:
  explicit BodyBlobConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedBlobHandle(
      scoped_refptr<BlobDataHandle> blob_data_handle) override {
    Resolver()->Resolve(Blob::Create(std::move(blob_data_handle)));
  }
  DISALLOW_COPY_AND_ASSIGN(BodyBlobConsumer);
};

class BodyArrayBufferConsumer final : public BodyConsumerBase {
 public:
  explicit BodyArrayBufferConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedArrayBuffer(DOMArrayBuffer* array_buffer) override {
    Resolver()->Resolve(array_buffer);
  }
  DISALLOW_COPY_AND_ASSIGN(BodyArrayBufferConsumer);
};

class BodyFormDataConsumer final : public BodyConsumerBase {
 public:
  explicit BodyFormDataConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedFormData(FormData* formData) override {
    Resolver()->Resolve(formData);
  }

  void DidFetchDataLoadedString(const String& string) override {
    FormData* formData = FormData::Create();
    for (const auto& pair : URLSearchParams::Create(string)->Params())
      formData->append(pair.first, pair.second);
    DidFetchDataLoadedFormData(formData);
  }
  DISALLOW_COPY_AND_ASSIGN(BodyFormDataConsumer);
};

class BodyTextConsumer final : public BodyConsumerBase {
 public:
  explicit BodyTextConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedString(const String& string) override {
    Resolver()->Resolve(string);
  }
  DISALLOW_COPY_AND_ASSIGN(BodyTextConsumer);
};

class BodyJsonConsumer final : public BodyConsumerBase {
 public:
  explicit BodyJsonConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedString(const String& string) override {
    if (!Resolver()->GetExecutionContext() ||
        Resolver()->GetExecutionContext()->IsContextDestroyed())
      return;
    ScriptState::Scope scope(Resolver()->GetScriptState());
    v8::Isolate* isolate = Resolver()->GetScriptState()->GetIsolate();
    v8::Local<v8::String> input_string = V8String(isolate, string);
    v8::TryCatch trycatch(isolate);
    v8::Local<v8::Value> parsed;
    if (v8::JSON::Parse(Resolver()->GetScriptState()->GetContext(),
                        input_string)
            .ToLocal(&parsed))
      Resolver()->Resolve(parsed);
    else
      Resolver()->Reject(trycatch.Exception());
  }
  DISALLOW_COPY_AND_ASSIGN(BodyJsonConsumer);
};

}  // namespace

ScriptPromise Body::arrayBuffer(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // When the main thread sends a V8::TerminateExecution() signal to a worker
  // thread, any V8 API on the worker thread starts returning an empty
  // handle. This can happen in this function. To avoid the situation, we
  // first check the ExecutionContext and return immediately if it's already
  // gone (which means that the V8::TerminateExecution() signal has been sent
  // to this worker thread).
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();
  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsArrayBuffer(),
                               new BodyArrayBufferConsumer(resolver));
  } else {
    resolver->Resolve(DOMArrayBuffer::Create(0u, 1));
  }
  return promise;
}

ScriptPromise Body::blob(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();
  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(
        FetchDataLoader::CreateLoaderAsBlobHandle(MimeType()),
        new BodyBlobConsumer(resolver));
  } else {
    std::unique_ptr<BlobData> blob_data = BlobData::Create();
    blob_data->SetContentType(MimeType());
    resolver->Resolve(
        Blob::Create(BlobDataHandle::Create(std::move(blob_data), 0)));
  }
  return promise;
}

ScriptPromise Body::formData(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ParsedContentType parsedTypeWithParameters(ContentType());
  const String parsedType = parsedTypeWithParameters.MimeType().LowerASCII();
  promise = resolver->Promise();
  if (parsedType == "multipart/form-data") {
    const String boundary =
        parsedTypeWithParameters.ParameterValueForName("boundary");
    if (BodyBuffer() && !boundary.IsEmpty()) {
      BodyBuffer()->StartLoading(
          FetchDataLoader::CreateLoaderAsFormData(boundary),
          new BodyFormDataConsumer(resolver));
      return promise;
    }
  } else if (parsedType == "application/x-www-form-urlencoded") {
    if (BodyBuffer()) {
      BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                                 new BodyFormDataConsumer(resolver));
    } else {
      resolver->Resolve(FormData::Create());
    }
    return promise;
  } else {
    if (BodyBuffer()) {
      BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsFailure(),
                                 new BodyFormDataConsumer(resolver));
      return promise;
    }
  }

  resolver->Reject(V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                                     "Invalid MIME type"));
  return promise;
}

ScriptPromise Body::json(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();
  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                               new BodyJsonConsumer(resolver));
  } else {
    resolver->Reject(V8ThrowException::CreateSyntaxError(
        script_state->GetIsolate(), "Unexpected end of input"));
  }
  return promise;
}

ScriptPromise Body::text(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();
  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                               new BodyTextConsumer(resolver));
  } else {
    resolver->Resolve(String());
  }
  return promise;
}

ScriptValue Body::body(ScriptState* script_state) {
  if (!BodyBuffer())
    return ScriptValue::CreateNull(script_state);
  ScriptValue stream = BodyBuffer()->Stream();
  DCHECK_EQ(stream.GetScriptState(), script_state);
  return stream;
}

bool Body::bodyUsed() {
  return BodyBuffer() && BodyBuffer()->IsStreamDisturbed();
}

bool Body::IsBodyLocked() {
  return BodyBuffer() && BodyBuffer()->IsStreamLocked();
}

bool Body::HasPendingActivity() const {
  if (!GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
    return false;
  if (!BodyBuffer())
    return false;
  return BodyBuffer()->HasPendingActivity();
}

Body::Body(ExecutionContext* context) : ContextClient(context) {}

ScriptPromise Body::RejectInvalidConsumption(ScriptState* script_state) {
  const bool used = bodyUsed();
  if (IsBodyLocked() || used) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            used ? "body stream already read" : "body stream is locked"));
  }
  return ScriptPromise();
}

}  // namespace blink
