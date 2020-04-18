// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_type_util.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "content/common/service_worker/service_worker_types.h"
#include "storage/common/blob_storage/blob_handle.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/public/platform/web_http_header_visitor.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {

namespace {

class HeaderVisitor : public blink::WebHTTPHeaderVisitor {
 public:
  explicit HeaderVisitor(ServiceWorkerHeaderMap* headers) : headers_(headers) {}
  ~HeaderVisitor() override {}

  void VisitHeader(const blink::WebString& name,
                   const blink::WebString& value) override {
    // Headers are ISO Latin 1.
    const std::string& header_name = name.Latin1();
    const std::string& header_value = value.Latin1();
    CHECK(header_name.find('\0') == std::string::npos);
    CHECK(header_value.find('\0') == std::string::npos);
    headers_->insert(ServiceWorkerHeaderMap::value_type(
        header_name, header_value));
  }

 private:
  ServiceWorkerHeaderMap* const headers_;
};

std::unique_ptr<HeaderVisitor> MakeHeaderVisitor(
    ServiceWorkerHeaderMap* headers) {
  return std::unique_ptr<HeaderVisitor>(new HeaderVisitor(headers));
}

std::unique_ptr<ServiceWorkerHeaderMap> GetHeaderMap(
    const blink::WebServiceWorkerResponse& web_response) {
  std::unique_ptr<ServiceWorkerHeaderMap> result =
      std::make_unique<ServiceWorkerHeaderMap>();
  web_response.VisitHTTPHeaderFields(MakeHeaderVisitor(result.get()).get());
  return result;
}

std::unique_ptr<ServiceWorkerHeaderList> GetHeaderList(
    const blink::WebVector<blink::WebString>& web_headers) {
  std::unique_ptr<ServiceWorkerHeaderList> result =
      std::make_unique<ServiceWorkerHeaderList>(web_headers.size());
  std::transform(web_headers.begin(), web_headers.end(), result->begin(),
                 [](const blink::WebString& s) { return s.Latin1(); });
  return result;
}

std::unique_ptr<std::vector<GURL>> GetURLList(
    const blink::WebVector<blink::WebURL>& web_url_list) {
  std::unique_ptr<std::vector<GURL>> result =
      std::make_unique<std::vector<GURL>>(web_url_list.size());
  std::transform(web_url_list.begin(), web_url_list.end(), result->begin(),
                 [](const blink::WebURL& url) { return url; });
  return result;
}

}  // namespace

void GetServiceWorkerHeaderMapFromWebRequest(
    const blink::WebServiceWorkerRequest& web_request,
    ServiceWorkerHeaderMap* headers) {
  DCHECK(headers);
  DCHECK(headers->empty());
  web_request.VisitHTTPHeaderFields(MakeHeaderVisitor(headers).get());
}

ServiceWorkerResponse GetServiceWorkerResponseFromWebResponse(
    const blink::WebServiceWorkerResponse& web_response) {
  scoped_refptr<storage::BlobHandle> blob;
  auto blob_pipe = web_response.CloneBlobPtr();
  if (blob_pipe.is_valid()) {
    blink::mojom::BlobPtr blob_ptr;
    blob_ptr.Bind(blink::mojom::BlobPtrInfo(std::move(blob_pipe),
                                            blink::mojom::Blob::Version_));
    blob = base::MakeRefCounted<storage::BlobHandle>(std::move(blob_ptr));
  }

  ServiceWorkerResponse response = ServiceWorkerResponse(
      GetURLList(web_response.UrlList()), web_response.Status(),
      web_response.StatusText().Utf8(), web_response.ResponseType(),
      GetHeaderMap(web_response), web_response.BlobUUID().Utf8(),
      web_response.BlobSize(), std::move(blob), web_response.GetError(),
      web_response.ResponseTime(),
      !web_response.CacheStorageCacheName().IsNull(),
      web_response.CacheStorageCacheName().Utf8(),
      GetHeaderList(web_response.CorsExposedHeaderNames()));
  if (!web_response.SideDataBlobSize())
    return response;
  response.side_data_blob_uuid = web_response.SideDataBlobUUID().Utf8();
  response.side_data_blob_size = web_response.SideDataBlobSize();
  auto side_data_blob_pipe = web_response.CloneSideDataBlobPtr();
  if (side_data_blob_pipe.is_valid()) {
    blink::mojom::BlobPtr side_data_blob_ptr;
    side_data_blob_ptr.Bind(blink::mojom::BlobPtrInfo(
        std::move(side_data_blob_pipe), blink::mojom::Blob::Version_));
    response.side_data_blob = base::MakeRefCounted<storage::BlobHandle>(
        std::move(side_data_blob_ptr));
  }
  return response;
}

}  // namespace content
