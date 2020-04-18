// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_H_
#define COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace gfx {
class Image;
class Size;
}  // namespace gfx

namespace image_fetcher {

class ImageDecoder;

struct RequestMetadata;

// A class used to fetch server images. It can be called from any thread and the
// callback will be called on the thread which initiated the fetch.
class ImageFetcher {
 public:
  ImageFetcher() {}
  virtual ~ImageFetcher() {}

  using ImageFetcherCallback =
      base::OnceCallback<void(const std::string& id,
                              const gfx::Image& image,
                              const RequestMetadata& metadata)>;

  // Callback with the |image_data|. If an error prevented a http response,
  // |request_metadata.response_code| will be RESPONSE_CODE_INVALID.
  // TODO(treib): Use RefCountedBytes to avoid copying.
  using ImageDataFetcherCallback =
      base::OnceCallback<void(const std::string& image_data,
                              const RequestMetadata& request_metadata)>;

  using DataUseServiceName = data_use_measurement::DataUseUserData::ServiceName;

  // Sets a service name against which to track data usage.
  virtual void SetDataUseServiceName(
      DataUseServiceName data_use_service_name) = 0;

  // Sets an upper limit for image downloads that is by default disabled.
  // Setting |max_download_bytes| to a negative value will disable the limit.
  // Already running downloads are immediately affected.
  virtual void SetImageDownloadLimit(
      base::Optional<int64_t> max_download_bytes) = 0;

  // Sets the desired size for images with multiple frames (like .ico files).
  // By default, the image fetcher choses smaller images. Override to choose a
  // frame with a size as close as possible to |size| (trying to take one in
  // larger size if there's no precise match). Passing gfx::Size() as
  // |size| is also supported and will result in chosing the smallest available
  // size.
  virtual void SetDesiredImageFrameSize(const gfx::Size& size) = 0;

  // Fetch an image and optionally decode it. |image_data_callback| is called
  // when the image fetch completes, but |image_data_callback| may be empty.
  // |image_callback| is called when the image is finished decoding.
  // |image_callback| may be empty if image decoding is not required. If a
  // callback is provided, it will be called exactly once. On failure, an empty
  // string/gfx::Image is returned.
  virtual void FetchImageAndData(
      const std::string& id,
      const GURL& image_url,
      ImageDataFetcherCallback image_data_callback,
      ImageFetcherCallback image_callback,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) = 0;

  // Fetch an image and decode it. An empty gfx::Image will be returned to the
  // callback in case the image could not be fetched. This is the same as
  // calling FetchImageAndData without an |image_data_callback|.
  void FetchImage(const std::string& id,
                  const GURL& image_url,
                  ImageFetcherCallback callback,
                  const net::NetworkTrafficAnnotationTag& traffic_annotation) {
    FetchImageAndData(id, image_url, ImageDataFetcherCallback(),
                      std::move(callback), traffic_annotation);
  }

  // Just fetch the image data, do not decode. This is the same as
  // calling FetchImageAndData without an |image_callback|.
  void FetchImageData(
      const std::string& id,
      const GURL& image_url,
      ImageDataFetcherCallback callback,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) {
    FetchImageAndData(id, image_url, std::move(callback),
                      ImageFetcherCallback(), traffic_annotation);
  }

  virtual ImageDecoder* GetImageDecoder() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImageFetcher);
};

}  // namespace image_fetcher

#endif  // COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_FETCHER_H_
