// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/image_fetcher/ios/ios_image_data_fetcher_wrapper.h"

#import "base/mac/bind_objc_block.h"
#include "base/task_scheduler/post_task.h"
#import "components/image_fetcher/ios/webp_decoder.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "url/url_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - IOSImageDataFetcherWrapper

namespace image_fetcher {

IOSImageDataFetcherWrapper::IOSImageDataFetcherWrapper(
    net::URLRequestContextGetter* url_request_context_getter)
    : image_data_fetcher_(url_request_context_getter) {}

IOSImageDataFetcherWrapper::~IOSImageDataFetcherWrapper() {}

void IOSImageDataFetcherWrapper::FetchImageDataWebpDecoded(
    const GURL& image_url,
    IOSImageDataFetcherCallback callback) {
  image_data_fetcher_.FetchImageData(image_url,
                                     CallbackForImageDataFetcher(callback),
                                     NO_TRAFFIC_ANNOTATION_YET);
}

void IOSImageDataFetcherWrapper::FetchImageDataWebpDecoded(
    const GURL& image_url,
    IOSImageDataFetcherCallback callback,
    const std::string& referrer,
    net::URLRequest::ReferrerPolicy referrer_policy) {
  DCHECK(callback);

  image_data_fetcher_.FetchImageData(
      image_url, CallbackForImageDataFetcher(callback), referrer,
      referrer_policy, NO_TRAFFIC_ANNOTATION_YET);
}

void IOSImageDataFetcherWrapper::SetDataUseServiceName(
    DataUseServiceName data_use_service_name) {
  image_data_fetcher_.SetDataUseServiceName(data_use_service_name);
}

ImageDataFetcher::ImageDataFetcherCallback
IOSImageDataFetcherWrapper::CallbackForImageDataFetcher(
    IOSImageDataFetcherCallback callback) {
  return base::BindBlockArc(^(const std::string& image_data,
                              const RequestMetadata& metadata) {
    // Create a NSData from the returned data and notify the callback.
    NSData* data =
        [NSData dataWithBytes:image_data.data() length:image_data.size()];

    if (!webp_transcode::WebpDecoder::IsWebpImage(image_data)) {
      callback(data, metadata);
      return;
    }

    // The image is a webp image.
    RequestMetadata webp_metadata = metadata;

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE,
        {
            base::TaskPriority::BACKGROUND,
            base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
        },
        base::BindBlockArc(^NSData*() {
          return webp_transcode::WebpDecoder::DecodeWebpImage(data);
        }),
        base::BindBlockArc(^(NSData* decoded_data) {
          callback(decoded_data, webp_metadata);
        }));
  });
}

}  // namespace image_fetcher
