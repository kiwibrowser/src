// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_NET_NET_METRICS_LOG_UPLOADER_H_
#define COMPONENTS_METRICS_NET_NET_METRICS_LOG_UPLOADER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "components/metrics/metrics_log_uploader.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "third_party/metrics_proto/reporting_info.pb.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace metrics {

// Implementation of MetricsLogUploader using the Chrome network stack.
class NetMetricsLogUploader : public MetricsLogUploader,
                              public net::URLFetcherDelegate {
 public:
  // Constructs a NetMetricsLogUploader which uploads data to |server_url| with
  // the specified |mime_type|. The |service_type| marks which service the
  // data usage should be attributed to. The |on_upload_complete| callback will
  // be called with the HTTP response code of the upload or with -1 on an error.
  // The caller must ensure that |request_context_getter| remains valid for the
  // lifetime of this class.
  NetMetricsLogUploader(
      net::URLRequestContextGetter* request_context_getter,
      base::StringPiece server_url,
      base::StringPiece mime_type,
      MetricsLogUploader::MetricServiceType service_type,
      const MetricsLogUploader::UploadCallback& on_upload_complete);

  // This constructor allows a secondary non-HTTPS URL to be passed in as
  // |insecure_server_url|. That URL is used as a fallback if a connection
  // to |server_url| fails, requests are encrypted when sent to an HTTP URL.
  NetMetricsLogUploader(
      net::URLRequestContextGetter* request_context_getter,
      base::StringPiece server_url,
      base::StringPiece insecure_server_url,
      base::StringPiece mime_type,
      MetricsLogUploader::MetricServiceType service_type,
      const MetricsLogUploader::UploadCallback& on_upload_complete);

  ~NetMetricsLogUploader() override;

  // MetricsLogUploader:
  // Uploads a log to the server_url specified in the constructor.
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& log_hash,
                 const ReportingInfo& reporting_info) override;

 private:
  // Uploads a log to a URL passed as a parameter.
  void UploadLogToURL(const std::string& compressed_log_data,
                      const std::string& log_hash,
                      const ReportingInfo& reporting_info,
                      const GURL& url);

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Encrypts a |plaintext| string, using the encrypted_messages component,
  // returns |encrypted| which is a serialized EncryptedMessage object.
  bool EncryptString(const std::string& plaintext, std::string* encrypted);

  // The request context for fetches done using the network stack.
  net::URLRequestContextGetter* const request_context_getter_;

  const GURL server_url_;
  const GURL insecure_server_url_;
  const std::string mime_type_;
  const MetricsLogUploader ::MetricServiceType service_type_;
  const MetricsLogUploader::UploadCallback on_upload_complete_;
  // The outstanding transmission appears as a URL Fetch operation.
  std::unique_ptr<net::URLFetcher> current_fetch_;

  DISALLOW_COPY_AND_ASSIGN(NetMetricsLogUploader);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_NET_NET_METRICS_LOG_UPLOADER_H_
