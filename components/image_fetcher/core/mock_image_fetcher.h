// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_IMAGE_FETCHER_CORE_MOCK_IMAGE_FETCHER_H_
#define COMPONENTS_IMAGE_FETCHER_CORE_MOCK_IMAGE_FETCHER_H_

#include "components/image_fetcher/core/image_fetcher.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace image_fetcher {
class MockImageFetcher : public ImageFetcher {
 public:
  MockImageFetcher();
  ~MockImageFetcher() override;
  MOCK_METHOD1(SetDataUseServiceName, void(DataUseServiceName));
  MOCK_METHOD1(SetImageDownloadLimit,
               void(base::Optional<int64_t> max_download_bytes));
  MOCK_METHOD1(SetDesiredImageFrameSize, void(const gfx::Size&));
  MOCK_METHOD5(FetchImageAndData_,
               void(const std::string&,
                    const GURL&,
                    ImageDataFetcherCallback*,
                    ImageFetcherCallback*,
                    const net::NetworkTrafficAnnotationTag&));
  void FetchImageAndData(
      const std::string& id,
      const GURL& image_url,
      ImageDataFetcherCallback image_data_callback,
      ImageFetcherCallback image_callback,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override;
  MOCK_METHOD0(GetImageDecoder, image_fetcher::ImageDecoder*());
};
}  // namespace image_fetcher

#endif  // COMPONENTS_IMAGE_FETCHER_CORE_MOCK_IMAGE_FETCHER_H_
