// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/image_fetcher/core/mock_image_fetcher.h"

#include "ui/gfx/geometry/size.h"

namespace image_fetcher {

MockImageFetcher::MockImageFetcher() {}
MockImageFetcher::~MockImageFetcher() {}

void MockImageFetcher::FetchImageAndData(
    const std::string& id,
    const GURL& image_url,
    ImageDataFetcherCallback image_data_callback,
    ImageFetcherCallback image_callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  FetchImageAndData_(id, image_url, &image_data_callback, &image_callback,
                     traffic_annotation);
}

}  // namespace image_fetcher
