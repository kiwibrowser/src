// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_BLOB_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
#define CONTENT_BROWSER_DOWNLOAD_BLOB_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_

#include "components/download/public/common/download_url_loader_factory_getter.h"
#include "url/gurl.h"

namespace storage {
class BlobDataHandle;
}

namespace content {

// Class for retrieving the URLLoaderFactory for a blob URL.
class BlobDownloadURLLoaderFactoryGetter
    : public download::DownloadURLLoaderFactoryGetter {
 public:
  BlobDownloadURLLoaderFactoryGetter(
      const GURL& url,
      std::unique_ptr<storage::BlobDataHandle> blob_data_handle);

  // download::DownloadURLLoaderFactoryGetter implementation.
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory() override;

 protected:
  ~BlobDownloadURLLoaderFactoryGetter() override;

 private:
  GURL url_;
  std::unique_ptr<storage::BlobDataHandle> blob_data_handle_;

  DISALLOW_COPY_AND_ASSIGN(BlobDownloadURLLoaderFactoryGetter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_BLOB_DOWNLOAD_URL_LOADER_FACTORY_GETTER_H_
