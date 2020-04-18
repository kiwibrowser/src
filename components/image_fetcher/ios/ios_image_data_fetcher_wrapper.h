// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_IMAGE_FETCHER_IOS_IOS_IMAGE_DATA_FETCHER_WRAPPER_H_
#define COMPONENTS_IMAGE_FETCHER_IOS_IOS_IMAGE_DATA_FETCHER_WRAPPER_H_

#import <Foundation/Foundation.h>

#include "base/memory/ref_counted.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/image_fetcher/core/image_data_fetcher.h"

namespace net {
class URLRequestContextGetter;
}

class GURL;

namespace image_fetcher {

// Callback that informs of the download of an image encoded in |data| and the
// associated metadata. If an error prevented a http response,
// |metadata.http_response_code| will be RESPONSE_CODE_INVALID.
using IOSImageDataFetcherCallback = void (^)(NSData* data,
                                             const RequestMetadata& metadata);

class IOSImageDataFetcherWrapper {
 public:
  using DataUseServiceName = data_use_measurement::DataUseUserData::ServiceName;

  // The TaskRunner is used to decode the image if it is WebP-encoded.
  explicit IOSImageDataFetcherWrapper(
      net::URLRequestContextGetter* url_request_context_getter);
  virtual ~IOSImageDataFetcherWrapper();

  // Helper to start downloading and possibly decoding the image without a
  // referrer.
  virtual void FetchImageDataWebpDecoded(const GURL& image_url,
                                         IOSImageDataFetcherCallback callback);

  // Start downloading the image at the given |image_url|. The |callback| will
  // be called with the downloaded image, or nil if any error happened. If the
  // image is WebP it will be decoded.
  // The |referrer| and |referrer_policy| will be passed on to the underlying
  // URLFetcher.
  // |callback| cannot be nil.
  void FetchImageDataWebpDecoded(
      const GURL& image_url,
      IOSImageDataFetcherCallback callback,
      const std::string& referrer,
      net::URLRequest::ReferrerPolicy referrer_policy);

  // Sets a service name against which to track data usage.
  void SetDataUseServiceName(DataUseServiceName data_use_service_name);

 private:
  ImageDataFetcher::ImageDataFetcherCallback CallbackForImageDataFetcher(
      IOSImageDataFetcherCallback callback);

  ImageDataFetcher image_data_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(IOSImageDataFetcherWrapper);
};

}  // namespace image_fetcher

#endif  // COMPONENTS_IMAGE_FETCHER_IOS_IOS_IMAGE_DATA_FETCHER_WRAPPER_H_
