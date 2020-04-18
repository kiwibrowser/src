// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cache_storage/cache.h"

#include <memory>
#include <utility>
#include "base/optional.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/renderer/bindings/core/v8/callback_promise_adapter.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_response.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/modules/cache_storage/cache_storage_error.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

namespace {

void RecordResponseTypeForAdd(const Member<Response>& response) {
  UMA_HISTOGRAM_ENUMERATION("ServiceWorkerCache.Cache.AddResponseType",
                            response->GetResponse()->GetType());
}

bool VaryHeaderContainsAsterisk(const Response* response) {
  const FetchHeaderList* headers = response->headers()->HeaderList();
  String varyHeader;
  if (headers->Get("vary", varyHeader)) {
    Vector<String> fields;
    varyHeader.Split(',', fields);
    return std::any_of(fields.begin(), fields.end(), [](const String& field) {
      return field.StripWhiteSpace() == "*";
    });
  }
  return false;
}

bool ShouldGenerateV8CodeCache(ScriptState* script_state,
                               const Response* response) {
  if (!RuntimeEnabledFeatures::PWAFullCodeCacheEnabled())
    return false;
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (!context->IsServiceWorkerGlobalScope())
    return false;
  if (!ToServiceWorkerGlobalScope(context)->IsInstalling())
    return false;
  if (!MIMETypeRegistry::IsSupportedJavaScriptMIMEType(
          response->InternalMIMEType())) {
    return false;
  }
  if (!response->InternalBodyBuffer())
    return false;
  return true;
}

}  // namespace

// TODO(nhiroki): Unfortunately, we have to go through V8 to wait for the fetch
// promise. It should be better to achieve this only within C++ world.
class Cache::FetchResolvedForAdd final : public ScriptFunction {
 public:
  static v8::Local<v8::Function> Create(
      ScriptState* script_state,
      Cache* cache,
      const HeapVector<Member<Request>>& requests) {
    FetchResolvedForAdd* self =
        new FetchResolvedForAdd(script_state, cache, requests);
    return self->BindToV8Function();
  }

  ScriptValue Call(ScriptValue value) override {
    NonThrowableExceptionState exception_state;
    HeapVector<Member<Response>> responses =
        NativeValueTraits<IDLSequence<Response>>::NativeValue(
            GetScriptState()->GetIsolate(), value.V8Value(), exception_state);

    for (const auto& response : responses) {
      if (!response->ok()) {
        ScriptPromise rejection = ScriptPromise::Reject(
            GetScriptState(),
            V8ThrowException::CreateTypeError(GetScriptState()->GetIsolate(),
                                              "Request failed"));
        return ScriptValue(GetScriptState(), rejection.V8Value());
      }
      if (VaryHeaderContainsAsterisk(response)) {
        ScriptPromise rejection = ScriptPromise::Reject(
            GetScriptState(),
            V8ThrowException::CreateTypeError(GetScriptState()->GetIsolate(),
                                              "Vary header contains *"));
        return ScriptValue(GetScriptState(), rejection.V8Value());
      }
    }

    for (const auto& response : responses)
      RecordResponseTypeForAdd(response);

    ScriptPromise put_promise =
        cache_->PutImpl(GetScriptState(), requests_, responses);
    return ScriptValue(GetScriptState(), put_promise.V8Value());
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(cache_);
    visitor->Trace(requests_);
    ScriptFunction::Trace(visitor);
  }

 private:
  FetchResolvedForAdd(ScriptState* script_state,
                      Cache* cache,
                      const HeapVector<Member<Request>>& requests)
      : ScriptFunction(script_state), cache_(cache), requests_(requests) {}

  Member<Cache> cache_;
  HeapVector<Member<Request>> requests_;
};

class Cache::BarrierCallbackForPut final
    : public GarbageCollectedFinalized<BarrierCallbackForPut> {
 public:
  BarrierCallbackForPut(int number_of_operations,
                        Cache* cache,
                        ScriptPromiseResolver* resolver)
      : number_of_remaining_operations_(number_of_operations),
        cache_(cache),
        resolver_(resolver) {
    DCHECK_LT(0, number_of_remaining_operations_);
    batch_operations_.resize(number_of_operations);
  }

  void OnSuccess(size_t index,
                 mojom::blink::BatchOperationPtr batch_operation) {
    DCHECK_LT(index, batch_operations_.size());
    if (!StillActive())
      return;
    batch_operations_[index] = std::move(batch_operation);
    if (--number_of_remaining_operations_ != 0)
      return;
    MaybeReportInstalledScripts();
    cache_->cache_ptr_->Batch(
        std::move(batch_operations_),
        WTF::Bind(
            [](ScriptPromiseResolver* resolver, TimeTicks start_time,
               mojom::blink::CacheStorageError error) {
              if (!resolver->GetExecutionContext() ||
                  resolver->GetExecutionContext()->IsContextDestroyed())
                return;
              if (error == mojom::blink::CacheStorageError::kSuccess) {
                UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Batch",
                                    TimeTicks::Now() - start_time);
                resolver->Resolve();
              } else {
                resolver->Reject(CacheStorageError::CreateException(error));
              }
            },
            WrapPersistent(resolver_.Get()), TimeTicks::Now()));
  }

  void OnError(const String& error_message) {
    if (!StillActive())
      return;
    completed_ = true;
    ScriptState* state = resolver_->GetScriptState();
    ScriptState::Scope scope(state);
    resolver_->Reject(
        V8ThrowException::CreateTypeError(state->GetIsolate(), error_message));
  }

  void Abort() {
    if (!StillActive())
      return;
    completed_ = true;
    ScriptState::Scope scope(resolver_->GetScriptState());
    resolver_->Reject(DOMException::Create(kAbortError));
  }

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(cache_);
    visitor->Trace(resolver_);
  }

 private:
  bool StillActive() {
    if (completed_)
      return false;
    if (!resolver_->GetExecutionContext() ||
        resolver_->GetExecutionContext()->IsContextDestroyed())
      return false;

    return true;
  }

  // Report the script stats if this cache storage is for service worker
  // execution context and it's in installation phase.
  void MaybeReportInstalledScripts() {
    ExecutionContext* context = resolver_->GetExecutionContext();
    if (!context || !context->IsServiceWorkerGlobalScope())
      return;
    ServiceWorkerGlobalScope* global_scope =
        ToServiceWorkerGlobalScope(context);
    if (!global_scope->IsInstalling())
      return;

    for (const auto& operation : batch_operations_) {
      scoped_refptr<BlobDataHandle> blob_data_handle =
          operation->response->blob;
      if (!blob_data_handle)
        continue;
      if (!MIMETypeRegistry::IsSupportedJavaScriptMIMEType(
              blob_data_handle->GetType())) {
        continue;
      }
      uint64_t side_data_blob_size =
          operation->response->side_data_blob
              ? operation->response->side_data_blob->size()
              : 0;
      global_scope->CountCacheStorageInstalledScript(blob_data_handle->size(),
                                                     side_data_blob_size);
    }
  }

  bool completed_ = false;
  int number_of_remaining_operations_;
  Member<Cache> cache_;
  Member<ScriptPromiseResolver> resolver_;
  Vector<mojom::blink::BatchOperationPtr> batch_operations_;
};

class Cache::BlobHandleCallbackForPut final
    : public GarbageCollectedFinalized<BlobHandleCallbackForPut>,
      public FetchDataLoader::Client {
  USING_GARBAGE_COLLECTED_MIXIN(BlobHandleCallbackForPut);

 public:
  BlobHandleCallbackForPut(size_t index,
                           BarrierCallbackForPut* barrier_callback,
                           Request* request,
                           Response* response)
      : index_(index), barrier_callback_(barrier_callback) {
    request->PopulateWebServiceWorkerRequest(web_request_);
    fetch_api_response_ = response->PopulateFetchAPIResponse();
  }
  ~BlobHandleCallbackForPut() override = default;

  void DidFetchDataLoadedBlobHandle(
      scoped_refptr<BlobDataHandle> handle) override {
    mojom::blink::BatchOperationPtr batch_operation =
        mojom::blink::BatchOperation::New();
    batch_operation->operation_type = mojom::blink::OperationType::kPut;
    batch_operation->request = web_request_;
    batch_operation->response = std::move(fetch_api_response_);
    batch_operation->response->blob = handle;
    barrier_callback_->OnSuccess(index_, std::move(batch_operation));
  }

  void DidFetchDataLoadFailed() override {
    barrier_callback_->OnError("network error");
  }

  void Abort() override { barrier_callback_->Abort(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(barrier_callback_);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  const size_t index_;
  Member<BarrierCallbackForPut> barrier_callback_;

  WebServiceWorkerRequest web_request_;
  mojom::blink::FetchAPIResponsePtr fetch_api_response_;
};

class Cache::CodeCacheHandleCallbackForPut final
    : public GarbageCollectedFinalized<CodeCacheHandleCallbackForPut>,
      public FetchDataLoader::Client {
  USING_GARBAGE_COLLECTED_MIXIN(CodeCacheHandleCallbackForPut);

 public:
  CodeCacheHandleCallbackForPut(ScriptState* script_state,
                                size_t index,
                                BarrierCallbackForPut* barrier_callback,
                                Request* request,
                                Response* response)
      : script_state_(script_state),
        index_(index),
        barrier_callback_(barrier_callback),
        mime_type_(response->InternalMIMEType()) {
    request->PopulateWebServiceWorkerRequest(web_request_);
    fetch_api_response_ = response->PopulateFetchAPIResponse();
  }
  ~CodeCacheHandleCallbackForPut() override = default;

  void DidFetchDataLoadedArrayBuffer(DOMArrayBuffer* array_buffer) override {
    mojom::blink::BatchOperationPtr batch_operation =
        mojom::blink::BatchOperation::New();
    batch_operation->operation_type = mojom::blink::OperationType::kPut;
    batch_operation->request = web_request_;
    batch_operation->response = std::move(fetch_api_response_);

    std::unique_ptr<BlobData> blob_data = BlobData::Create();
    blob_data->SetContentType(mime_type_);
    blob_data->AppendBytes(array_buffer->Data(), array_buffer->ByteLength());
    batch_operation->response->blob = BlobDataHandle::Create(
        std::move(blob_data), array_buffer->ByteLength());

    // Currently we only support UTF8 encoding.
    // TODO(horo): Use the charset in Content-type header of the response.
    // See crbug.com/743311.
    std::unique_ptr<TextResourceDecoder> text_decoder =
        TextResourceDecoder::Create(
            TextResourceDecoderOptions::CreateAlwaysUseUTF8ForText());

    scoped_refptr<CachedMetadata> cached_metadata =
        V8ScriptRunner::GenerateFullCodeCache(
            script_state_.get(),
            text_decoder->Decode(static_cast<const char*>(array_buffer->Data()),
                                 array_buffer->ByteLength()),
            web_request_.Url().GetString(), text_decoder->Encoding(),
            batch_operation->response->response_type ==
                    network::mojom::FetchResponseType::kOpaque
                ? V8ScriptRunner::OpaqueMode::kOpaque
                : V8ScriptRunner::OpaqueMode::kNotOpaque);
    if (!cached_metadata) {
      barrier_callback_->OnSuccess(index_, std::move(batch_operation));
      return;
    }
    const Vector<char>& serialized_data = cached_metadata->SerializedData();
    std::unique_ptr<BlobData> side_data_blob_data = BlobData::Create();
    side_data_blob_data->AppendBytes(serialized_data.data(),
                                     serialized_data.size());

    batch_operation->response->side_data_blob = BlobDataHandle::Create(
        std::move(side_data_blob_data), serialized_data.size());
    barrier_callback_->OnSuccess(index_, std::move(batch_operation));
  }

  void DidFetchDataLoadFailed() override {
    barrier_callback_->OnError("network error");
  }

  void Abort() override { barrier_callback_->Abort(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(barrier_callback_);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  const scoped_refptr<ScriptState> script_state_;
  const size_t index_;
  Member<BarrierCallbackForPut> barrier_callback_;
  const String mime_type_;

  WebServiceWorkerRequest web_request_;
  mojom::blink::FetchAPIResponsePtr fetch_api_response_;
};

Cache* Cache::Create(
    GlobalFetch::ScopedFetcher* fetcher,
    mojom::blink::CacheStorageCacheAssociatedPtrInfo cache_ptr_info) {
  return new Cache(fetcher, std::move(cache_ptr_info));
}

ScriptPromise Cache::match(ScriptState* script_state,
                           const RequestInfo& request,
                           const CacheQueryOptions& options,
                           ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  if (request.IsRequest())
    return MatchImpl(script_state, request.GetAsRequest(), options);
  Request* new_request =
      Request::Create(script_state, request.GetAsUSVString(), exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  return MatchImpl(script_state, new_request, options);
}

ScriptPromise Cache::matchAll(ScriptState* script_state,
                              ExceptionState& exception_state) {
  return MatchAllImpl(script_state, nullptr, CacheQueryOptions());
}

ScriptPromise Cache::matchAll(ScriptState* script_state,
                              const RequestInfo& request,
                              const CacheQueryOptions& options,
                              ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  if (request.IsRequest())
    return MatchAllImpl(script_state, request.GetAsRequest(), options);
  Request* new_request =
      Request::Create(script_state, request.GetAsUSVString(), exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  return MatchAllImpl(script_state, new_request, options);
}

ScriptPromise Cache::add(ScriptState* script_state,
                         const RequestInfo& request,
                         ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  HeapVector<Member<Request>> requests;
  if (request.IsRequest()) {
    requests.push_back(request.GetAsRequest());
  } else {
    requests.push_back(Request::Create(script_state, request.GetAsUSVString(),
                                       exception_state));
    if (exception_state.HadException())
      return ScriptPromise();
  }

  return AddAllImpl(script_state, requests, exception_state);
}

ScriptPromise Cache::addAll(ScriptState* script_state,
                            const HeapVector<RequestInfo>& raw_requests,
                            ExceptionState& exception_state) {
  HeapVector<Member<Request>> requests;
  for (RequestInfo request : raw_requests) {
    if (request.IsRequest()) {
      requests.push_back(request.GetAsRequest());
    } else {
      requests.push_back(Request::Create(script_state, request.GetAsUSVString(),
                                         exception_state));
      if (exception_state.HadException())
        return ScriptPromise();
    }
  }

  return AddAllImpl(script_state, requests, exception_state);
}

ScriptPromise Cache::Delete(ScriptState* script_state,
                            const RequestInfo& request,
                            const CacheQueryOptions& options,
                            ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  if (request.IsRequest())
    return DeleteImpl(script_state, request.GetAsRequest(), options);
  Request* new_request =
      Request::Create(script_state, request.GetAsUSVString(), exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  return DeleteImpl(script_state, new_request, options);
}

ScriptPromise Cache::put(ScriptState* script_state,
                         const RequestInfo& request,
                         Response* response,
                         ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  if (request.IsRequest()) {
    return PutImpl(script_state,
                   HeapVector<Member<Request>>(1, request.GetAsRequest()),
                   HeapVector<Member<Response>>(1, response));
  }
  Request* new_request =
      Request::Create(script_state, request.GetAsUSVString(), exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  return PutImpl(script_state, HeapVector<Member<Request>>(1, new_request),
                 HeapVector<Member<Response>>(1, response));
}

ScriptPromise Cache::keys(ScriptState* script_state, ExceptionState&) {
  return KeysImpl(script_state, nullptr, CacheQueryOptions());
}

ScriptPromise Cache::keys(ScriptState* script_state,
                          const RequestInfo& request,
                          const CacheQueryOptions& options,
                          ExceptionState& exception_state) {
  DCHECK(!request.IsNull());
  if (request.IsRequest())
    return KeysImpl(script_state, request.GetAsRequest(), options);
  Request* new_request =
      Request::Create(script_state, request.GetAsUSVString(), exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  return KeysImpl(script_state, new_request, options);
}

// static
mojom::blink::QueryParamsPtr Cache::ToQueryParams(
    const CacheQueryOptions& options) {
  mojom::blink::QueryParamsPtr query_params = mojom::blink::QueryParams::New();
  query_params->ignore_search = options.ignoreSearch();
  query_params->ignore_method = options.ignoreMethod();
  query_params->ignore_vary = options.ignoreVary();
  query_params->cache_name = options.cacheName();
  return query_params;
}

Cache::Cache(GlobalFetch::ScopedFetcher* fetcher,
             mojom::blink::CacheStorageCacheAssociatedPtrInfo cache_ptr_info)
    : scoped_fetcher_(fetcher) {
  cache_ptr_.Bind(std::move(cache_ptr_info));
}

void Cache::Trace(blink::Visitor* visitor) {
  visitor->Trace(scoped_fetcher_);
  ScriptWrappable::Trace(visitor);
}

ScriptPromise Cache::MatchImpl(ScriptState* script_state,
                               const Request* request,
                               const CacheQueryOptions& options) {
  WebServiceWorkerRequest web_request;
  request->PopulateWebServiceWorkerRequest(web_request);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ScriptPromise promise = resolver->Promise();
  if (request->method() != HTTPNames::GET && !options.ignoreMethod()) {
    resolver->Resolve();
    return promise;
  }

  cache_ptr_->Match(
      web_request, ToQueryParams(options),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver, TimeTicks start_time,
             mojom::blink::MatchResultPtr result) {
            if (!resolver->GetExecutionContext() ||
                resolver->GetExecutionContext()->IsContextDestroyed())
              return;
            if (result->is_status()) {
              switch (result->get_status()) {
                case mojom::CacheStorageError::kErrorNotFound:
                  resolver->Resolve();
                  break;
                default:
                  resolver->Reject(
                      CacheStorageError::CreateException(result->get_status()));
                  break;
              }
            } else {
              UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Match",
                                  TimeTicks::Now() - start_time);
              ScriptState::Scope scope(resolver->GetScriptState());
              resolver->Resolve(Response::Create(resolver->GetScriptState(),
                                                 *result->get_response()));
            }
          },
          WrapPersistent(resolver), TimeTicks::Now()));

  return promise;
}

ScriptPromise Cache::MatchAllImpl(ScriptState* script_state,
                                  const Request* request,
                                  const CacheQueryOptions& options) {
  base::Optional<WebServiceWorkerRequest> web_request;
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ScriptPromise promise = resolver->Promise();

  if (request) {
    request->PopulateWebServiceWorkerRequest(web_request.emplace());

    if (request->method() != HTTPNames::GET && !options.ignoreMethod()) {
      resolver->Resolve(HeapVector<Member<Response>>());
      return promise;
    }
  }

  cache_ptr_->MatchAll(
      web_request, ToQueryParams(options),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver, TimeTicks start_time,
             mojom::blink::MatchAllResultPtr result) {
            if (!resolver->GetExecutionContext() ||
                resolver->GetExecutionContext()->IsContextDestroyed())
              return;
            if (result->is_status()) {
              resolver->Reject(
                  CacheStorageError::CreateException(result->get_status()));
            } else {
              UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.MatchAll",
                                  TimeTicks::Now() - start_time);
              ScriptState::Scope scope(resolver->GetScriptState());
              HeapVector<Member<Response>> responses;
              responses.ReserveInitialCapacity(result->get_responses().size());
              for (auto& response : result->get_responses()) {
                responses.push_back(
                    Response::Create(resolver->GetScriptState(), *response));
              }
              resolver->Resolve(responses);
            }
          },
          WrapPersistent(resolver), TimeTicks::Now()));
  return promise;
}

ScriptPromise Cache::AddAllImpl(ScriptState* script_state,
                                const HeapVector<Member<Request>>& requests,
                                ExceptionState& exception_state) {
  if (requests.IsEmpty())
    return ScriptPromise::CastUndefined(script_state);

  HeapVector<RequestInfo> request_infos;
  request_infos.resize(requests.size());
  Vector<ScriptPromise> promises;
  promises.resize(requests.size());
  for (size_t i = 0; i < requests.size(); ++i) {
    if (!requests[i]->url().ProtocolIsInHTTPFamily()) {
      return ScriptPromise::Reject(script_state,
                                   V8ThrowException::CreateTypeError(
                                       script_state->GetIsolate(),
                                       "Add/AddAll does not support schemes "
                                       "other than \"http\" or \"https\""));
    }
    if (requests[i]->method() != HTTPNames::GET) {
      return ScriptPromise::Reject(
          script_state,
          V8ThrowException::CreateTypeError(
              script_state->GetIsolate(),
              "Add/AddAll only supports the GET request method."));
    }
    request_infos[i].SetRequest(requests[i]);

    promises[i] = scoped_fetcher_->Fetch(script_state, request_infos[i],
                                         Dictionary(), exception_state);
  }

  return ScriptPromise::All(script_state, promises)
      .Then(FetchResolvedForAdd::Create(script_state, this, requests));
}

ScriptPromise Cache::DeleteImpl(ScriptState* script_state,
                                const Request* request,
                                const CacheQueryOptions& options) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ScriptPromise promise = resolver->Promise();
  if (request->method() != HTTPNames::GET && !options.ignoreMethod()) {
    resolver->Resolve(false);
    return promise;
  }

  Vector<mojom::blink::BatchOperationPtr> batch_operations;
  batch_operations.push_back(mojom::blink::BatchOperation::New());
  auto& operation = batch_operations.back();
  operation->operation_type = mojom::blink::OperationType::kDelete;
  request->PopulateWebServiceWorkerRequest(operation->request);
  operation->match_params = ToQueryParams(options);

  cache_ptr_->Batch(
      std::move(batch_operations),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver, TimeTicks start_time,
             mojom::blink::CacheStorageError error) {
            if (!resolver->GetExecutionContext() ||
                resolver->GetExecutionContext()->IsContextDestroyed())
              return;
            if (error != mojom::blink::CacheStorageError::kSuccess) {
              switch (error) {
                case mojom::blink::CacheStorageError::kErrorNotFound:
                  resolver->Resolve(false);
                  break;
                default:
                  resolver->Reject(CacheStorageError::CreateException(error));
                  break;
              }
            } else {
              UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Batch",
                                  TimeTicks::Now() - start_time);
              resolver->Resolve(true);
            }
          },
          WrapPersistent(resolver), TimeTicks::Now()));
  return promise;
}

ScriptPromise Cache::PutImpl(ScriptState* script_state,
                             const HeapVector<Member<Request>>& requests,
                             const HeapVector<Member<Response>>& responses) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ScriptPromise promise = resolver->Promise();
  BarrierCallbackForPut* barrier_callback =
      new BarrierCallbackForPut(requests.size(), this, resolver);

  for (size_t i = 0; i < requests.size(); ++i) {
    KURL url(NullURL(), requests[i]->url());
    if (!url.ProtocolIsInHTTPFamily()) {
      barrier_callback->OnError("Request scheme '" + url.Protocol() +
                                "' is unsupported");
      return promise;
    }
    if (requests[i]->method() != HTTPNames::GET) {
      barrier_callback->OnError("Request method '" + requests[i]->method() +
                                "' is unsupported");
      return promise;
    }
    DCHECK(!requests[i]->HasBody());

    if (VaryHeaderContainsAsterisk(responses[i])) {
      barrier_callback->OnError("Vary header contains *");
      return promise;
    }
    if (responses[i]->status() == 206) {
      barrier_callback->OnError(
          "Partial response (status code 206) is unsupported");
      return promise;
    }
    if (responses[i]->IsBodyLocked() || responses[i]->bodyUsed()) {
      barrier_callback->OnError("Response body is already used");
      return promise;
    }

    BodyStreamBuffer* buffer = responses[i]->InternalBodyBuffer();

    if (ShouldGenerateV8CodeCache(script_state, responses[i])) {
      FetchDataLoader* loader = FetchDataLoader::CreateLoaderAsArrayBuffer();
      buffer->StartLoading(loader, new CodeCacheHandleCallbackForPut(
                                       script_state, i, barrier_callback,
                                       requests[i], responses[i]));
      continue;
    }

    if (buffer) {
      // If the response has body, read the all data and create
      // the blob handle and dispatch the put batch asynchronously.
      FetchDataLoader* loader = FetchDataLoader::CreateLoaderAsBlobHandle(
          responses[i]->InternalMIMEType());
      buffer->StartLoading(
          loader, new BlobHandleCallbackForPut(i, barrier_callback, requests[i],
                                               responses[i]));
      continue;
    }

    mojom::blink::BatchOperationPtr batch_operation =
        mojom::blink::BatchOperation::New();
    batch_operation->operation_type = mojom::blink::OperationType::kPut;
    requests[i]->PopulateWebServiceWorkerRequest(batch_operation->request);
    batch_operation->response = responses[i]->PopulateFetchAPIResponse();
    barrier_callback->OnSuccess(i, std::move(batch_operation));
  }

  return promise;
}

ScriptPromise Cache::KeysImpl(ScriptState* script_state,
                              const Request* request,
                              const CacheQueryOptions& options) {
  base::Optional<WebServiceWorkerRequest> web_request;
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ScriptPromise promise = resolver->Promise();

  if (request) {
    request->PopulateWebServiceWorkerRequest(web_request.emplace());

    if (request->method() != HTTPNames::GET && !options.ignoreMethod()) {
      resolver->Resolve(HeapVector<Member<Response>>());
      return promise;
    }
  }

  cache_ptr_->Keys(
      web_request, ToQueryParams(options),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver, TimeTicks start_time,
             mojom::blink::CacheKeysResultPtr result) {
            if (!resolver->GetExecutionContext() ||
                resolver->GetExecutionContext()->IsContextDestroyed())
              return;
            if (result->is_status()) {
              resolver->Reject(
                  CacheStorageError::CreateException(result->get_status()));
            } else {
              UMA_HISTOGRAM_TIMES("ServiceWorkerCache.Cache.Keys",
                                  TimeTicks::Now() - start_time);
              ScriptState::Scope scope(resolver->GetScriptState());
              HeapVector<Member<Request>> requests;
              requests.ReserveInitialCapacity(result->get_keys().size());
              for (auto& request : result->get_keys()) {
                requests.push_back(
                    Request::Create(resolver->GetScriptState(), request));
              }
              resolver->Resolve(requests);
            }
          },
          WrapPersistent(resolver), TimeTicks::Now()));
  return promise;
}

}  // namespace blink
