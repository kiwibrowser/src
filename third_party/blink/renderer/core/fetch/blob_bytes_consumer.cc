// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/blob_bytes_consumer.h"

#include "third_party/blink/renderer/core/fetch/bytes_consumer_for_data_consumer_handle.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/blob/blob_registry.h"
#include "third_party/blink/renderer/platform/blob/blob_url.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

BlobBytesConsumer::BlobBytesConsumer(
    ExecutionContext* execution_context,
    scoped_refptr<BlobDataHandle> blob_data_handle,
    ThreadableLoader* loader)
    : ContextLifecycleObserver(execution_context),
      blob_data_handle_(std::move(blob_data_handle)),
      loader_(loader) {
  if (!blob_data_handle_) {
    // Note that |m_loader| is non-null only in tests.
    if (loader_) {
      loader_->Cancel();
      loader_ = nullptr;
    }
    state_ = PublicState::kClosed;
  }
}

BlobBytesConsumer::BlobBytesConsumer(
    ExecutionContext* execution_context,
    scoped_refptr<BlobDataHandle> blob_data_handle)
    : BlobBytesConsumer(execution_context,
                        std::move(blob_data_handle),
                        nullptr) {}

BlobBytesConsumer::~BlobBytesConsumer() {
  if (!blob_url_.IsEmpty())
    BlobRegistry::RevokePublicBlobURL(blob_url_);
}

BytesConsumer::Result BlobBytesConsumer::BeginRead(const char** buffer,
                                                   size_t* available) {
  *buffer = nullptr;
  *available = 0;

  if (state_ == PublicState::kClosed) {
    // It's possible that |cancel| has been called before the first
    // |beginRead| call. That's why we need to check this condition
    // before checking |isClean()|.
    return Result::kDone;
  }

  if (IsClean()) {
    DCHECK(blob_url_.IsEmpty());
    blob_url_ =
        BlobURL::CreatePublicURL(GetExecutionContext()->GetSecurityOrigin());
    if (blob_url_.IsEmpty()) {
      GetError();
    } else {
      BlobRegistry::RegisterPublicBlobURL(
          GetExecutionContext()->GetMutableSecurityOrigin(), blob_url_,
          blob_data_handle_);

      // m_loader is non-null only in tests.
      if (!loader_)
        loader_ = CreateLoader();

      ResourceRequest request(blob_url_);
      request.SetRequestContext(WebURLRequest::kRequestContextInternal);
      request.SetFetchRequestMode(
          network::mojom::FetchRequestMode::kSameOrigin);
      request.SetFetchCredentialsMode(
          network::mojom::FetchCredentialsMode::kOmit);
      request.SetUseStreamOnResponse(true);
      // We intentionally skip
      // 'setExternalRequestStateFromRequestorAddressSpace', as 'blob:'
      // can never be external.
      loader_->Start(request);
    }
    blob_data_handle_ = nullptr;
  }
  DCHECK_NE(state_, PublicState::kClosed);

  if (state_ == PublicState::kErrored)
    return Result::kError;

  if (!body_) {
    // The response has not arrived.
    return Result::kShouldWait;
  }

  auto result = body_->BeginRead(buffer, available);
  switch (result) {
    case Result::kOk:
    case Result::kShouldWait:
      break;
    case Result::kDone:
      has_seen_end_of_data_ = true;
      if (has_finished_loading_)
        Close();
      return state_ == PublicState::kClosed ? Result::kDone
                                            : Result::kShouldWait;
    case Result::kError:
      GetError();
      break;
  }
  return result;
}

BytesConsumer::Result BlobBytesConsumer::EndRead(size_t read) {
  DCHECK(body_);
  return body_->EndRead(read);
}

scoped_refptr<BlobDataHandle> BlobBytesConsumer::DrainAsBlobDataHandle(
    BlobSizePolicy policy) {
  if (!IsClean())
    return nullptr;
  DCHECK(blob_data_handle_);
  if (policy == BlobSizePolicy::kDisallowBlobWithInvalidSize &&
      blob_data_handle_->size() == UINT64_MAX)
    return nullptr;
  Close();
  return std::move(blob_data_handle_);
}

scoped_refptr<EncodedFormData> BlobBytesConsumer::DrainAsFormData() {
  scoped_refptr<BlobDataHandle> handle =
      DrainAsBlobDataHandle(BlobSizePolicy::kAllowBlobWithInvalidSize);
  if (!handle)
    return nullptr;
  scoped_refptr<EncodedFormData> form_data = EncodedFormData::Create();
  form_data->AppendBlob(handle->Uuid(), handle);
  return form_data;
}

void BlobBytesConsumer::SetClient(BytesConsumer::Client* client) {
  DCHECK(!client_);
  DCHECK(client);
  client_ = client;
}

void BlobBytesConsumer::ClearClient() {
  client_ = nullptr;
}

void BlobBytesConsumer::Cancel() {
  if (state_ == PublicState::kClosed || state_ == PublicState::kErrored)
    return;
  Close();
  if (body_) {
    body_->Cancel();
    body_ = nullptr;
  }
  if (!blob_url_.IsEmpty()) {
    BlobRegistry::RevokePublicBlobURL(blob_url_);
    blob_url_ = KURL();
  }
  blob_data_handle_ = nullptr;
}

BytesConsumer::Error BlobBytesConsumer::GetError() const {
  DCHECK_EQ(PublicState::kErrored, state_);
  return Error("Failed to load a blob.");
}

BytesConsumer::PublicState BlobBytesConsumer::GetPublicState() const {
  return state_;
}

void BlobBytesConsumer::ContextDestroyed(ExecutionContext*) {
  if (state_ != PublicState::kReadableOrWaiting)
    return;

  BytesConsumer::Client* client = client_;
  GetError();
  if (client)
    client->OnStateChange();
}

void BlobBytesConsumer::OnStateChange() {
  if (state_ != PublicState::kReadableOrWaiting)
    return;
  DCHECK(body_);

  BytesConsumer::Client* client = client_;
  switch (body_->GetPublicState()) {
    case PublicState::kReadableOrWaiting:
      break;
    case PublicState::kClosed:
      has_seen_end_of_data_ = true;
      if (has_finished_loading_)
        Close();
      break;
    case PublicState::kErrored:
      GetError();
      break;
  }
  if (client)
    client->OnStateChange();
}

void BlobBytesConsumer::DidReceiveResponse(
    unsigned long identifier,
    const ResourceResponse&,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK(handle);
  DCHECK(!body_);
  DCHECK_EQ(PublicState::kReadableOrWaiting, state_);

  body_ = new BytesConsumerForDataConsumerHandle(GetExecutionContext(),
                                                 std::move(handle));
  body_->SetClient(this);

  if (IsClean()) {
    // This function is called synchronously in ThreadableLoader::start.
    return;
  }
  OnStateChange();
}

void BlobBytesConsumer::DidFinishLoading(unsigned long identifier) {
  DCHECK_EQ(PublicState::kReadableOrWaiting, state_);
  has_finished_loading_ = true;
  loader_ = nullptr;
  if (!has_seen_end_of_data_)
    return;
  DCHECK(!IsClean());
  BytesConsumer::Client* client = client_;
  Close();
  if (client)
    client->OnStateChange();
}

void BlobBytesConsumer::DidFail(const ResourceError& e) {
  if (e.IsCancellation()) {
    if (state_ != PublicState::kReadableOrWaiting)
      return;
  }
  DCHECK_EQ(PublicState::kReadableOrWaiting, state_);
  loader_ = nullptr;
  BytesConsumer::Client* client = client_;
  GetError();
  if (IsClean()) {
    // This function is called synchronously in ThreadableLoader::start.
    return;
  }
  if (client) {
    client->OnStateChange();
    client = nullptr;
  }
}

void BlobBytesConsumer::DidFailRedirectCheck() {
  NOTREACHED();
}

void BlobBytesConsumer::Trace(blink::Visitor* visitor) {
  visitor->Trace(body_);
  visitor->Trace(client_);
  visitor->Trace(loader_);
  BytesConsumer::Trace(visitor);
  BytesConsumer::Client::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

BlobBytesConsumer* BlobBytesConsumer::CreateForTesting(
    ExecutionContext* execution_context,
    scoped_refptr<BlobDataHandle> blob_data_handle,
    ThreadableLoader* loader) {
  return new BlobBytesConsumer(execution_context, std::move(blob_data_handle),
                               loader);
}

ThreadableLoader* BlobBytesConsumer::CreateLoader() {
  ThreadableLoaderOptions options;

  ResourceLoaderOptions resource_loader_options;
  resource_loader_options.data_buffering_policy = kDoNotBufferData;
  resource_loader_options.initiator_info.name =
      FetchInitiatorTypeNames::internal;

  return ThreadableLoader::Create(*GetExecutionContext(), this, options,
                                  resource_loader_options);
}

void BlobBytesConsumer::Close() {
  DCHECK_EQ(state_, PublicState::kReadableOrWaiting);
  state_ = PublicState::kClosed;
  Clear();
}

void BlobBytesConsumer::GetError() {
  DCHECK_EQ(state_, PublicState::kReadableOrWaiting);
  state_ = PublicState::kErrored;
  Clear();
}

void BlobBytesConsumer::Clear() {
  DCHECK_NE(state_, PublicState::kReadableOrWaiting);
  if (loader_) {
    loader_->Cancel();
    loader_ = nullptr;
  }
  client_ = nullptr;
}

}  // namespace blink
