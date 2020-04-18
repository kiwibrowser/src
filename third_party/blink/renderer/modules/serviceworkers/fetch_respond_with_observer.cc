// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/fetch_respond_with_observer.h"

#include <memory>
#include <utility>

#include <v8.h>
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_response.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/console_types.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/wait_until_observer.h"

using blink::mojom::ServiceWorkerResponseError;

namespace blink {
namespace {

// Returns the error message to let the developer know about the reason of the
// unusual failures.
const String GetMessageForResponseError(ServiceWorkerResponseError error,
                                        const KURL& request_url) {
  String error_message = "The FetchEvent for \"" + request_url.GetString() +
                         "\" resulted in a network error response: ";
  switch (error) {
    case ServiceWorkerResponseError::kPromiseRejected:
      error_message = error_message + "the promise was rejected.";
      break;
    case ServiceWorkerResponseError::kDefaultPrevented:
      error_message =
          error_message +
          "preventDefault() was called without calling respondWith().";
      break;
    case ServiceWorkerResponseError::kNoV8Instance:
      error_message =
          error_message +
          "an object that was not a Response was passed to respondWith().";
      break;
    case ServiceWorkerResponseError::kResponseTypeError:
      error_message = error_message +
                      "the promise was resolved with an error response object.";
      break;
    case ServiceWorkerResponseError::kResponseTypeOpaque:
      error_message =
          error_message +
          "an \"opaque\" response was used for a request whose type "
          "is not no-cors";
      break;
    case ServiceWorkerResponseError::kResponseTypeNotBasicOrDefault:
      NOTREACHED();
      break;
    case ServiceWorkerResponseError::kBodyUsed:
      error_message =
          error_message +
          "a Response whose \"bodyUsed\" is \"true\" cannot be used "
          "to respond to a request.";
      break;
    case ServiceWorkerResponseError::kResponseTypeOpaqueForClientRequest:
      error_message = error_message +
                      "an \"opaque\" response was used for a client request.";
      break;
    case ServiceWorkerResponseError::kResponseTypeOpaqueRedirect:
      error_message = error_message +
                      "an \"opaqueredirect\" type response was used for a "
                      "request whose redirect mode is not \"manual\".";
      break;
    case ServiceWorkerResponseError::kResponseTypeCORSForRequestModeSameOrigin:
      error_message = error_message +
                      "a \"cors\" type response was used for a request whose "
                      "mode is \"same-origin\".";
      break;
    case ServiceWorkerResponseError::kBodyLocked:
      error_message = error_message +
                      "a Response whose \"body\" is locked cannot be used to "
                      "respond to a request.";
      break;
    case ServiceWorkerResponseError::kRedirectedResponseForNotFollowRequest:
      error_message = error_message +
                      "a redirected response was used for a request whose "
                      "redirect mode is not \"follow\".";
      break;
    case ServiceWorkerResponseError::kDataPipeCreationFailed:
      error_message = error_message + "insufficient resources.";
      break;
    case ServiceWorkerResponseError::kUnknown:
    default:
      error_message = error_message + "an unexpected error occurred.";
      break;
  }
  return error_message;
}

bool IsNavigationRequest(network::mojom::RequestContextFrameType frame_type) {
  return frame_type != network::mojom::RequestContextFrameType::kNone;
}

bool IsClientRequest(network::mojom::RequestContextFrameType frame_type,
                     WebURLRequest::RequestContext request_context) {
  return IsNavigationRequest(frame_type) ||
         request_context == WebURLRequest::kRequestContextSharedWorker ||
         request_context == WebURLRequest::kRequestContextWorker;
}

// Notifies the result of FetchDataLoader to |handle_|. |handle_| pass through
// the result to its observer which is outside of blink.
class FetchLoaderClient final
    : public GarbageCollectedFinalized<FetchLoaderClient>,
      public FetchDataLoader::Client {
  WTF_MAKE_NONCOPYABLE(FetchLoaderClient);
  USING_GARBAGE_COLLECTED_MIXIN(FetchLoaderClient);

 public:
  explicit FetchLoaderClient(
      std::unique_ptr<WebServiceWorkerStreamHandle> handle)
      : handle_(std::move(handle)) {}

  void DidFetchDataLoadedDataPipe() override { handle_->Completed(); }
  void DidFetchDataLoadFailed() override { handle_->Aborted(); }
  void Abort() override {
    // A fetch() aborted via AbortSignal in the ServiceWorker will just look
    // like an ordinary failure to the page.
    // TODO(ricea): Should a fetch() on the page get an AbortError instead?
    handle_->Aborted();
  }

  void Trace(blink::Visitor* visitor) override {
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  std::unique_ptr<WebServiceWorkerStreamHandle> handle_;
};

}  // namespace

FetchRespondWithObserver* FetchRespondWithObserver::Create(
    ExecutionContext* context,
    int fetch_event_id,
    const KURL& request_url,
    network::mojom::FetchRequestMode request_mode,
    network::mojom::FetchRedirectMode redirect_mode,
    network::mojom::RequestContextFrameType frame_type,
    WebURLRequest::RequestContext request_context,
    WaitUntilObserver* observer) {
  return new FetchRespondWithObserver(context, fetch_event_id, request_url,
                                      request_mode, redirect_mode, frame_type,
                                      request_context, observer);
}

void FetchRespondWithObserver::OnResponseRejected(
    ServiceWorkerResponseError error) {
  DCHECK(GetExecutionContext());
  GetExecutionContext()->AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel,
                             GetMessageForResponseError(error, request_url_)));

  // The default value of WebServiceWorkerResponse's status is 0, which maps
  // to a network error.
  WebServiceWorkerResponse web_response;
  web_response.SetError(error);
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToFetchEvent(event_id_, web_response, event_dispatch_time_);
}

void FetchRespondWithObserver::OnResponseFulfilled(const ScriptValue& value) {
  DCHECK(GetExecutionContext());
  if (!V8Response::hasInstance(value.V8Value(),
                               ToIsolate(GetExecutionContext()))) {
    OnResponseRejected(ServiceWorkerResponseError::kNoV8Instance);
    return;
  }
  Response* response = V8Response::ToImplWithTypeCheck(
      ToIsolate(GetExecutionContext()), value.V8Value());
  // "If one of the following conditions is true, return a network error:
  //   - |response|'s type is |error|.
  //   - |request|'s mode is |same-origin| and |response|'s type is |cors|.
  //   - |request|'s mode is not |no-cors| and response's type is |opaque|.
  //   - |request| is a client request and |response|'s type is neither
  //     |basic| nor |default|."
  const network::mojom::FetchResponseType response_type =
      response->GetResponse()->GetType();
  if (response_type == network::mojom::FetchResponseType::kError) {
    OnResponseRejected(ServiceWorkerResponseError::kResponseTypeError);
    return;
  }
  if (response_type == network::mojom::FetchResponseType::kCORS &&
      request_mode_ == network::mojom::FetchRequestMode::kSameOrigin) {
    OnResponseRejected(
        ServiceWorkerResponseError::kResponseTypeCORSForRequestModeSameOrigin);
    return;
  }
  if (response_type == network::mojom::FetchResponseType::kOpaque) {
    if (request_mode_ != network::mojom::FetchRequestMode::kNoCORS) {
      OnResponseRejected(ServiceWorkerResponseError::kResponseTypeOpaque);
      return;
    }

    // The request mode of client requests should be "same-origin" but it is
    // not explicitly stated in the spec yet. So we need to check here.
    // FIXME: Set the request mode of client requests to "same-origin" and
    // remove this check when the spec will be updated.
    // Spec issue: https://github.com/whatwg/fetch/issues/101
    if (IsClientRequest(frame_type_, request_context_)) {
      OnResponseRejected(
          ServiceWorkerResponseError::kResponseTypeOpaqueForClientRequest);
      return;
    }
  }
  if (redirect_mode_ != network::mojom::FetchRedirectMode::kManual &&
      response_type == network::mojom::FetchResponseType::kOpaqueRedirect) {
    OnResponseRejected(ServiceWorkerResponseError::kResponseTypeOpaqueRedirect);
    return;
  }
  if (redirect_mode_ != network::mojom::FetchRedirectMode::kFollow &&
      response->redirected()) {
    OnResponseRejected(
        ServiceWorkerResponseError::kRedirectedResponseForNotFollowRequest);
    return;
  }
  if (response->IsBodyLocked()) {
    OnResponseRejected(ServiceWorkerResponseError::kBodyLocked);
    return;
  }
  if (response->bodyUsed()) {
    OnResponseRejected(ServiceWorkerResponseError::kBodyUsed);
    return;
  }

  WebServiceWorkerResponse web_response;
  response->PopulateWebServiceWorkerResponse(web_response);

  BodyStreamBuffer* buffer = response->InternalBodyBuffer();
  if (buffer) {
    scoped_refptr<BlobDataHandle> blob_data_handle =
        buffer->DrainAsBlobDataHandle(
            BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize);
    if (blob_data_handle) {
      // Handle the blob response body.
      web_response.SetBlobDataHandle(blob_data_handle);
      ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
          ->RespondToFetchEvent(event_id_, web_response, event_dispatch_time_);
      return;
    }
    // Handle the stream response body.
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    MojoResult result = mojo::CreateDataPipe(nullptr, &producer, &consumer);
    if (result != MOJO_RESULT_OK) {
      OnResponseRejected(ServiceWorkerResponseError::kDataPipeCreationFailed);
      return;
    }
    DCHECK(producer.is_valid());
    DCHECK(consumer.is_valid());

    std::unique_ptr<WebServiceWorkerStreamHandle> body_stream_handle =
        std::make_unique<WebServiceWorkerStreamHandle>(std::move(consumer));
    ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
        ->RespondToFetchEventWithResponseStream(event_id_, web_response,
                                                body_stream_handle.get(),
                                                event_dispatch_time_);

    buffer->StartLoading(FetchDataLoader::CreateLoaderAsDataPipe(
                             std::move(producer), task_runner_),
                         new FetchLoaderClient(std::move(body_stream_handle)));
    return;
  }
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToFetchEvent(event_id_, web_response, event_dispatch_time_);
}

void FetchRespondWithObserver::OnNoResponse() {
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToFetchEventWithNoResponse(event_id_, event_dispatch_time_);
}

FetchRespondWithObserver::FetchRespondWithObserver(
    ExecutionContext* context,
    int fetch_event_id,
    const KURL& request_url,
    network::mojom::FetchRequestMode request_mode,
    network::mojom::FetchRedirectMode redirect_mode,
    network::mojom::RequestContextFrameType frame_type,
    WebURLRequest::RequestContext request_context,
    WaitUntilObserver* observer)
    : RespondWithObserver(context, fetch_event_id, observer),
      request_url_(request_url),
      request_mode_(request_mode),
      redirect_mode_(redirect_mode),
      frame_type_(frame_type),
      request_context_(request_context),
      task_runner_(context->GetTaskRunner(TaskType::kNetworking)) {}

void FetchRespondWithObserver::Trace(blink::Visitor* visitor) {
  RespondWithObserver::Trace(visitor);
}

}  // namespace blink
