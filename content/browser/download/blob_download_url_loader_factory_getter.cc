// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/blob_download_url_loader_factory_getter.h"

#include "components/download/public/common/download_task_runner.h"
#include "content/browser/url_loader_factory_getter.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_url_loader_factory.h"

namespace content {

BlobDownloadURLLoaderFactoryGetter::BlobDownloadURLLoaderFactoryGetter(
    const GURL& url,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle)
    : url_(url), blob_data_handle_(std::move(blob_data_handle)) {
  DCHECK(url.SchemeIs(url::kBlobScheme));
}

BlobDownloadURLLoaderFactoryGetter::~BlobDownloadURLLoaderFactoryGetter() =
    default;

scoped_refptr<network::SharedURLLoaderFactory>
BlobDownloadURLLoaderFactoryGetter::GetURLLoaderFactory() {
  DCHECK(download::GetIOTaskRunner());
  DCHECK(download::GetIOTaskRunner()->BelongsToCurrentThread());
  network::mojom::URLLoaderFactoryPtrInfo url_loader_factory_ptr_info;
  storage::BlobURLLoaderFactory::Create(
      std::move(blob_data_handle_), url_,
      mojo::MakeRequest(&url_loader_factory_ptr_info));
  return base::MakeRefCounted<network::WrapperSharedURLLoaderFactory>(
      std::move(url_loader_factory_ptr_info));
}

}  // namespace content
