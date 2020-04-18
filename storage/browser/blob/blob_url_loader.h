// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_URL_LOADER_H_
#define STORAGE_BROWSER_BLOB_BLOB_URL_LOADER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/http/http_status_code.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "storage/browser/blob/mojo_blob_reader.h"
#include "storage/browser/storage_browser_export.h"

namespace storage {
class BlobDataHandle;

// This class handles a request for a blob:// url. It self-destructs (directly,
// or after passing ownership to MojoBlobReader at the end of the Start
// method) when it has finished responding.
// Note: some of this code is duplicated from BlobURLRequestJob.
class STORAGE_EXPORT BlobURLLoader : public storage::MojoBlobReader::Delegate,
                                     public network::mojom::URLLoader {
 public:
  static void CreateAndStart(
      network::mojom::URLLoaderRequest url_loader_request,
      const network::ResourceRequest& request,
      network::mojom::URLLoaderClientPtr client,
      std::unique_ptr<BlobDataHandle> blob_handle);
  ~BlobURLLoader() override;

 private:
  BlobURLLoader(network::mojom::URLLoaderRequest url_loader_request,
                const network::ResourceRequest& request,
                network::mojom::URLLoaderClientPtr client,
                std::unique_ptr<BlobDataHandle> blob_handle);

  void Start(const network::ResourceRequest& request);

  // network::mojom::URLLoader implementation:
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override;
  void ProceedWithResponse() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  // storage::MojoBlobReader::Delegate implementation:
  mojo::ScopedDataPipeProducerHandle PassDataPipe() override;
  RequestSideData DidCalculateSize(uint64_t total_size,
                                   uint64_t content_size) override;
  void DidReadSideData(net::IOBufferWithSize* data) override;
  void DidRead(int num_bytes) override;
  void OnComplete(net::Error error_code, uint64_t total_written_bytes) override;

  void HeadersCompleted(net::HttpStatusCode status_code,
                        uint64_t content_size,
                        net::IOBufferWithSize* metadata);

  mojo::Binding<network::mojom::URLLoader> binding_;
  network::mojom::URLLoaderClientPtr client_;

  bool byte_range_set_ = false;
  net::HttpByteRange byte_range_;

  uint64_t total_size_ = 0;
  bool sent_headers_ = false;

  std::unique_ptr<BlobDataHandle> blob_handle_;
  mojo::ScopedDataPipeConsumerHandle response_body_consumer_handle_;

  base::WeakPtrFactory<BlobURLLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLLoader);
};

}  // namespace storage

#endif  // CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_
