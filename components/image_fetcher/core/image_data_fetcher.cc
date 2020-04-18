// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/image_fetcher/core/image_data_fetcher.h"

#include <utility>

#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

using data_use_measurement::DataUseUserData;

namespace {

const char kContentLocationHeader[] = "Content-Location";

}  // namespace

namespace image_fetcher {

const int ImageDataFetcher::kFirstUrlFetcherId = 163163;

// An active image URL fetcher request. The struct contains the related requests
// state.
struct ImageDataFetcher::ImageDataFetcherRequest {
  ImageDataFetcherRequest(const ImageDataFetcherCallback& callback,
                          std::unique_ptr<net::URLFetcher> url_fetcher)
      : callback(callback), url_fetcher(std::move(url_fetcher)) {}

  ~ImageDataFetcherRequest() {}

  // The callback to run after the image data was fetched. The callback will
  // be run even if the image data could not be fetched successfully.
  ImageDataFetcherCallback callback;

  std::unique_ptr<net::URLFetcher> url_fetcher;
};

ImageDataFetcher::ImageDataFetcher(
    net::URLRequestContextGetter* url_request_context_getter)
    : url_request_context_getter_(url_request_context_getter),
      data_use_service_name_(DataUseUserData::IMAGE_FETCHER_UNTAGGED),
      next_url_fetcher_id_(kFirstUrlFetcherId) {}

ImageDataFetcher::~ImageDataFetcher() {}

void ImageDataFetcher::SetDataUseServiceName(
    DataUseServiceName data_use_service_name) {
  data_use_service_name_ = data_use_service_name;
}

void ImageDataFetcher::SetImageDownloadLimit(
    base::Optional<int64_t> max_download_bytes) {
  max_download_bytes_ = max_download_bytes;
}

void ImageDataFetcher::FetchImageData(
    const GURL& image_url,
    const ImageDataFetcherCallback& callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  FetchImageData(
      image_url, callback, /*referrer=*/std::string(),
      net::URLRequest::CLEAR_REFERRER_ON_TRANSITION_FROM_SECURE_TO_INSECURE,
      traffic_annotation);
}

void ImageDataFetcher::FetchImageData(
    const GURL& image_url,
    const ImageDataFetcherCallback& callback,
    const std::string& referrer,
    net::URLRequest::ReferrerPolicy referrer_policy,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  std::unique_ptr<net::URLFetcher> url_fetcher =
      net::URLFetcher::Create(next_url_fetcher_id_++, image_url,
                              net::URLFetcher::GET, this, traffic_annotation);

  DataUseUserData::AttachToFetcher(url_fetcher.get(), data_use_service_name_);

  std::unique_ptr<ImageDataFetcherRequest> request(
      new ImageDataFetcherRequest(callback, std::move(url_fetcher)));
  request->url_fetcher->SetRequestContext(url_request_context_getter_.get());
  request->url_fetcher->SetReferrer(referrer);
  request->url_fetcher->SetReferrerPolicy(referrer_policy);
  request->url_fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                                     net::LOAD_DO_NOT_SAVE_COOKIES |
                                     net::LOAD_DO_NOT_SEND_AUTH_DATA);
  request->url_fetcher->Start();

  pending_requests_[request->url_fetcher.get()] = std::move(request);
}

void ImageDataFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(pending_requests_.find(source) != pending_requests_.end());
  bool success = source->GetStatus().status() == net::URLRequestStatus::SUCCESS;

  RequestMetadata metadata;
  if (success && source->GetResponseHeaders()) {
    source->GetResponseHeaders()->GetMimeType(&metadata.mime_type);
    metadata.http_response_code = source->GetResponseHeaders()->response_code();
    // Just read the first value-pair for this header (not caring about |iter|).
    source->GetResponseHeaders()->EnumerateHeader(
        /*iter=*/nullptr, kContentLocationHeader,
        &metadata.content_location_header);
    success &= (metadata.http_response_code == net::HTTP_OK);
  }

  std::string image_data;
  if (success) {
    source->GetResponseAsString(&image_data);
  }
  FinishRequest(source, metadata, image_data);
}

void ImageDataFetcher::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  if (!max_download_bytes_.has_value()) {
    return;
  }
  if (total <= max_download_bytes_.value() &&
      current <= max_download_bytes_.value()) {
    return;
  }
  DCHECK(pending_requests_.find(source) != pending_requests_.end());
  DLOG(WARNING) << "Image data exceeded download size limit.";
  RequestMetadata metadata;
  metadata.http_response_code = net::URLFetcher::RESPONSE_CODE_INVALID;

  FinishRequest(source, metadata, /*image_data=*/std::string());
}

void ImageDataFetcher::FinishRequest(const net::URLFetcher* source,
                                     const RequestMetadata& metadata,
                                     const std::string& image_data) {
  auto request_iter = pending_requests_.find(source);
  DCHECK(request_iter != pending_requests_.end());
  request_iter->second->callback.Run(image_data, metadata);
  pending_requests_.erase(request_iter);
}

}  // namespace image_fetcher
