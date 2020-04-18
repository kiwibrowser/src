// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_DATA_FETCHER_H_
#define COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_DATA_FETCHER_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace image_fetcher {

class ImageDataFetcher : public net::URLFetcherDelegate {
 public:
  // Fetchers created by this class will be assigned an incremental id starting
  // from |kFirstUrlFetcherId|, so unit tests can differentiate the URLFetchers
  // used by this class from other fetchers.
  const static int kFirstUrlFetcherId;

  // Callback with the |image_data|. If an error prevented a http response,
  // |request_metadata.response_code| will be RESPONSE_CODE_INVALID.
  // TODO(treib): Pass |image_data| out by value, or use RefCountedBytes, to
  // avoid copying.
  using ImageDataFetcherCallback =
      base::Callback<void(const std::string& image_data,
                          const RequestMetadata& request_metadata)>;

  using DataUseServiceName = data_use_measurement::DataUseUserData::ServiceName;

  explicit ImageDataFetcher(
      net::URLRequestContextGetter* url_request_context_getter);
  ~ImageDataFetcher() override;

  // Sets a service name against which to track data usage.
  void SetDataUseServiceName(DataUseServiceName data_use_service_name);

  // Sets an upper limit for image downloads.
  // Already running downloads are affected.
  void SetImageDownloadLimit(base::Optional<int64_t> max_download_bytes);

  // Fetches the raw image bytes from the given |image_url| and calls the given
  // |callback|. The callback is run even if fetching the URL fails. In case
  // of an error an empty string is passed to the callback.
  void FetchImageData(
      const GURL& image_url,
      const ImageDataFetcherCallback& callback,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

  // Like above, but lets the caller set a referrer.
  void FetchImageData(
      const GURL& image_url,
      const ImageDataFetcherCallback& callback,
      const std::string& referrer,
      net::URLRequest::ReferrerPolicy referrer_policy,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

 private:
  struct ImageDataFetcherRequest;

  // Methods inherited from URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes) override;

  void FinishRequest(const net::URLFetcher* source,
                     const RequestMetadata& metadata,
                     const std::string& image_data);

  // All active image url requests.
  std::map<const net::URLFetcher*, std::unique_ptr<ImageDataFetcherRequest>>
      pending_requests_;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  DataUseServiceName data_use_service_name_;

  // The next ID to use for a newly created URLFetcher. Each URLFetcher gets an
  // id when it is created. The |url_fetcher_id_| is incremented by one for each
  // newly created URLFetcher. The URLFetcher ID can be used during testing to
  // get individual URLFetchers and modify their state. Outside of tests this ID
  // is not used.
  int next_url_fetcher_id_;

  // Upper limit for the number of bytes to download per image.
  base::Optional<int64_t> max_download_bytes_;

  DISALLOW_COPY_AND_ASSIGN(ImageDataFetcher);
};

}  // namespace image_fetcher

#endif  // COMPONENTS_IMAGE_FETCHER_CORE_IMAGE_DATA_FETCHER_H_
