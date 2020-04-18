// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_BLOB_READER_H_
#define EXTENSIONS_BROWSER_BLOB_READER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace net {
class URLFetcher;
}

// This class may only be used from the UI thread.
class BlobReader : public net::URLFetcherDelegate {
 public:
  // |blob_data| contains the portion of the Blob requested. |blob_total_size|
  // is the total size of the Blob, and may be larger than |blob_data->size()|.
  // |blob_total_size| is -1 if it cannot be determined.
  typedef base::Callback<void(std::unique_ptr<std::string> blob_data,
                              int64_t blob_total_size)>
      BlobReadCallback;

  BlobReader(content::BrowserContext* browser_context,
             const std::string& blob_uuid,
             BlobReadCallback callback);
  ~BlobReader() override;

  void SetByteRange(int64_t offset, int64_t length);

  void Start();

 private:
  // Overridden from net::URLFetcherDelegate.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  BlobReadCallback callback_;
  std::unique_ptr<net::URLFetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(BlobReader);
};

#endif  // EXTENSIONS_BROWSER_BLOB_READER_H_
