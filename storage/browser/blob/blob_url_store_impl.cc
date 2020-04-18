// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_store_impl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/blob_url_loader_factory.h"
#include "storage/browser/blob/blob_url_utils.h"

namespace storage {

// Self deletes when the last binding to it is closed.
class BlobURLTokenImpl : public blink::mojom::BlobURLToken {
 public:
  BlobURLTokenImpl(base::WeakPtr<BlobStorageContext> context,
                   const GURL& url,
                   std::unique_ptr<BlobDataHandle> blob,
                   blink::mojom::BlobURLTokenRequest request)
      : context_(std::move(context)),
        url_(url),
        blob_(std::move(blob)),
        token_(base::UnguessableToken::Create()) {
    bindings_.AddBinding(this, std::move(request));
    bindings_.set_connection_error_handler(base::BindRepeating(
        &BlobURLTokenImpl::OnConnectionError, base::Unretained(this)));
    if (context_) {
      context_->mutable_registry()->AddTokenMapping(token_, url_,
                                                    blob_->uuid());
    }
  }

  ~BlobURLTokenImpl() override {
    if (context_)
      context_->mutable_registry()->RemoveTokenMapping(token_);
  }

  void GetToken(GetTokenCallback callback) override {
    std::move(callback).Run(token_);
  }

  void Clone(blink::mojom::BlobURLTokenRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

 private:
  void OnConnectionError() {
    if (!bindings_.empty())
      return;
    delete this;
  }

  base::WeakPtr<BlobStorageContext> context_;
  mojo::BindingSet<blink::mojom::BlobURLToken> bindings_;
  const GURL url_;
  const std::unique_ptr<BlobDataHandle> blob_;
  const base::UnguessableToken token_;
};

BlobURLStoreImpl::BlobURLStoreImpl(base::WeakPtr<BlobStorageContext> context,
                                   BlobRegistryImpl::Delegate* delegate)
    : context_(std::move(context)),
      delegate_(delegate),
      weak_ptr_factory_(this) {}

BlobURLStoreImpl::~BlobURLStoreImpl() {
  if (context_) {
    for (const auto& url : urls_)
      context_->RevokePublicBlobURL(url);
  }
}

void BlobURLStoreImpl::Register(blink::mojom::BlobPtr blob,
                                const GURL& url,
                                RegisterCallback callback) {
  if (!url.SchemeIsBlob() || !delegate_->CanCommitURL(url) ||
      BlobUrlUtils::UrlHasFragment(url)) {
    mojo::ReportBadMessage("Invalid Blob URL passed to BlobURLStore::Register");
    std::move(callback).Run();
    return;
  }

  blink::mojom::Blob* blob_ptr = blob.get();
  blob_ptr->GetInternalUUID(base::BindOnce(
      &BlobURLStoreImpl::RegisterWithUUID, weak_ptr_factory_.GetWeakPtr(),
      std::move(blob), url, std::move(callback)));
}

void BlobURLStoreImpl::Revoke(const GURL& url) {
  if (!url.SchemeIsBlob() || !delegate_->CanCommitURL(url) ||
      BlobUrlUtils::UrlHasFragment(url)) {
    mojo::ReportBadMessage("Invalid Blob URL passed to BlobURLStore::Revoke");
    return;
  }
  if (context_)
    context_->RevokePublicBlobURL(url);
  urls_.erase(url);
}

void BlobURLStoreImpl::Resolve(const GURL& url, ResolveCallback callback) {
  if (!context_) {
    std::move(callback).Run(nullptr);
    return;
  }
  blink::mojom::BlobPtr blob;
  std::unique_ptr<BlobDataHandle> blob_handle =
      context_->GetBlobDataFromPublicURL(url);
  if (blob_handle)
    BlobImpl::Create(std::move(blob_handle), MakeRequest(&blob));
  std::move(callback).Run(std::move(blob));
}

void BlobURLStoreImpl::ResolveAsURLLoaderFactory(
    const GURL& url,
    network::mojom::URLLoaderFactoryRequest request) {
  BlobURLLoaderFactory::Create(
      context_ ? context_->GetBlobDataFromPublicURL(url) : nullptr, url,
      std::move(request));
}

void BlobURLStoreImpl::ResolveForNavigation(
    const GURL& url,
    blink::mojom::BlobURLTokenRequest token) {
  if (!context_)
    return;
  std::unique_ptr<BlobDataHandle> blob_handle =
      context_->GetBlobDataFromPublicURL(url);
  if (!blob_handle)
    return;
  new BlobURLTokenImpl(context_, url, std::move(blob_handle), std::move(token));
}

void BlobURLStoreImpl::RegisterWithUUID(blink::mojom::BlobPtr blob,
                                        const GURL& url,
                                        RegisterCallback callback,
                                        const std::string& uuid) {
  // |blob| is unused, but is passed here to be kept alive until
  // RegisterPublicBlobURL increments the refcount of it via the uuid.
  if (context_)
    context_->RegisterPublicBlobURL(url, uuid);
  urls_.insert(url);
  std::move(callback).Run();
}

}  // namespace storage
