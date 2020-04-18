// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_loader_factory.h"

#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/blob_url_loader.h"

namespace storage {

namespace {

// The BlobURLTokenPtr parameter is passed in to make sure the connection stays
// alive until this method is called, it is not otherwise used by this method.
void CreateFactoryForToken(blink::mojom::BlobURLTokenPtr,
                           const base::WeakPtr<BlobStorageContext>& context,
                           network::mojom::URLLoaderFactoryRequest request,
                           const base::UnguessableToken& token) {
  std::unique_ptr<BlobDataHandle> handle;
  GURL blob_url;
  if (context) {
    std::string uuid;
    if (context->registry().GetTokenMapping(token, &blob_url, &uuid))
      handle = context->GetBlobDataFromUUID(uuid);
  }
  BlobURLLoaderFactory::Create(std::move(handle), blob_url, std::move(request));
}

}  // namespace

// static
void BlobURLLoaderFactory::Create(
    std::unique_ptr<BlobDataHandle> handle,
    const GURL& blob_url,
    network::mojom::URLLoaderFactoryRequest request) {
  new BlobURLLoaderFactory(std::move(handle), blob_url, std::move(request));
}

// static
void BlobURLLoaderFactory::Create(
    blink::mojom::BlobURLTokenPtr token,
    base::WeakPtr<BlobStorageContext> context,
    network::mojom::URLLoaderFactoryRequest request) {
  // Not every URLLoaderFactory user deals with the URLLoaderFactory simply
  // disconnecting very well, so make sure we always at least bind the request
  // to some factory that can then fail with a network error. Hence the callback
  // is wrapped in WrapCallbackWithDefaultInvokeIfNotRun.
  auto* raw_token = token.get();
  raw_token->GetToken(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      base::BindOnce(&CreateFactoryForToken, std::move(token),
                     std::move(context), std::move(request)),
      base::UnguessableToken()));
}

void BlobURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (url_.is_valid() && request.url != url_) {
    bindings_.ReportBadMessage("Invalid URL when attempting to fetch Blob");
    client->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INVALID_URL));
    return;
  }
  DCHECK(!request.download_to_file);
  BlobURLLoader::CreateAndStart(
      std::move(loader), request, std::move(client),
      handle_ ? std::make_unique<BlobDataHandle>(*handle_) : nullptr);
}

void BlobURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

BlobURLLoaderFactory::BlobURLLoaderFactory(
    std::unique_ptr<BlobDataHandle> handle,
    const GURL& blob_url,
    network::mojom::URLLoaderFactoryRequest request)
    : handle_(std::move(handle)), url_(blob_url) {
  bindings_.AddBinding(this, std::move(request));
  bindings_.set_connection_error_handler(base::BindRepeating(
      &BlobURLLoaderFactory::OnConnectionError, base::Unretained(this)));
}

BlobURLLoaderFactory::~BlobURLLoaderFactory() = default;

void BlobURLLoaderFactory::OnConnectionError() {
  if (!bindings_.empty())
    return;
  delete this;
}

}  // namespace storage
