// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_IMPL_H_
#define COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/image_fetcher/core/image_data_fetcher.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace gfx {
class Image;
}

namespace net {
class URLRequestContextGetter;
}

namespace image_fetcher {

// The standard (non-test) implementation of ImageFetcher.
class ImageFetcherImpl : public ImageFetcher {
 public:
  ImageFetcherImpl(std::unique_ptr<ImageDecoder> image_decoder,
                   net::URLRequestContextGetter* url_request_context);
  ~ImageFetcherImpl() override;

  // Sets a service name against which to track data usage.
  void SetDataUseServiceName(DataUseServiceName data_use_service_name) override;

  void SetDesiredImageFrameSize(const gfx::Size& size) override;

  void SetImageDownloadLimit(
      base::Optional<int64_t> max_download_bytes) override;

  void FetchImageAndData(
      const std::string& id,
      const GURL& image_url,
      ImageDataFetcherCallback image_data_callback,
      ImageFetcherCallback image_callback,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override;

  ImageDecoder* GetImageDecoder() override;

 private:
  // State related to an image fetch (id, pending callbacks).
  struct ImageRequest {
    ImageRequest();
    ~ImageRequest();
    ImageRequest(ImageRequest&& other);

    std::string id;
    // These have the default value if the image data has not yet been fetched.
    RequestMetadata request_metadata;
    std::string image_data;
    // Queue for pending callbacks, which may accumulate while the request is in
    // flight.
    std::vector<ImageFetcherCallback> image_callbacks;
    std::vector<ImageDataFetcherCallback> image_data_callbacks;

    DISALLOW_COPY_AND_ASSIGN(ImageRequest);
  };

  using ImageRequestMap = std::map<GURL, ImageRequest>;

  // Processes image URL fetched events. This is the continuation method used
  // for creating callbacks that are passed to the ImageDataFetcher.
  void OnImageURLFetched(const GURL& image_url,
                         const std::string& image_data,
                         const RequestMetadata& metadata);

  // Processes image decoded events. This is the continuation method used for
  // creating callbacks that are passed to the ImageDecoder.
  void OnImageDecoded(const GURL& image_url,
                      const RequestMetadata& metadata,
                      const gfx::Image& image);

  gfx::Size desired_image_frame_size_;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_;

  std::unique_ptr<ImageDecoder> image_decoder_;

  std::unique_ptr<ImageDataFetcher> image_data_fetcher_;

  // Map from each image URL to the request information (associated website
  // url, fetcher, pending callbacks).
  ImageRequestMap pending_net_requests_;

  DISALLOW_COPY_AND_ASSIGN(ImageFetcherImpl);
};

}  // namespace image_fetcher

#endif  // COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_IMPL_H_
