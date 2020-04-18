// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_
#define CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/system/file_data_pipe_producer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace content {

class CONTENT_EXPORT FileURLLoaderObserver
    : public mojo::FileDataPipeProducer::Observer {
 public:
  FileURLLoaderObserver() {}
  ~FileURLLoaderObserver() override {}

  virtual void OnStart() {}
  virtual void OnSeekComplete(int64_t result) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FileURLLoaderObserver);
};

// Helper to create a self-owned URLLoader instance which fulfills |request|
// using the contents of the file at |path|. The URL in |request| must be a
// file:// URL. The *optionally* supplied |observer| will be called to report
// progress during the file loading.
//
// Note that this does not restrict filesystem access *in any way*, so if the
// file at |path| is accessible to the browser, it will be loaded and used to
// the request.
//
// The URLLoader created by this function does *not* automatically follow
// filesytem links (e.g. Windows shortcuts) or support directory listing.
// A directory path will always yield a FILE_NOT_FOUND network error.
CONTENT_EXPORT void CreateFileURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    network::mojom::URLLoaderClientPtr client,
    std::unique_ptr<FileURLLoaderObserver> observer,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers = nullptr);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_FILE_URL_LOADER_H_
