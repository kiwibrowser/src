// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_FETCHER_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_FETCHER_H_

#include <stddef.h>
#include <stdint.h>

#include "content/browser/appcache/appcache_response.h"
#include "content/browser/appcache/appcache_update_job.h"
#include "net/base/io_buffer.h"
#include "url/gurl.h"

namespace content {

// Helper class to fetch resources. Depending on the fetch type,
// can either fetch to an in-memory string or write the response
// data out to the disk cache.
class AppCacheUpdateJob::URLFetcher {
 public:
  enum FetchType {
    MANIFEST_FETCH,
    URL_FETCH,
    MASTER_ENTRY_FETCH,
    MANIFEST_REFETCH,
  };
  URLFetcher(const GURL& url,
             FetchType fetch_type,
             AppCacheUpdateJob* job,
             int buffer_size);
  ~URLFetcher();
  void Start();
  FetchType fetch_type() const { return fetch_type_; }
  UpdateRequestBase* request() const { return request_.get(); }
  const AppCacheEntry& existing_entry() const { return existing_entry_; }
  const std::string& manifest_data() const { return manifest_data_; }
  AppCacheResponseWriter* response_writer() const {
    return response_writer_.get();
  }
  void set_existing_response_headers(net::HttpResponseHeaders* headers) {
    existing_response_headers_ = headers;
  }
  void set_existing_entry(const AppCacheEntry& entry) {
    existing_entry_ = entry;
  }
  AppCacheUpdateJob::ResultType result() const { return result_; }
  int redirect_response_code() const { return redirect_response_code_; }

 private:
  void OnReceivedRedirect(const net::RedirectInfo& redirect_info);
  void OnResponseStarted(int net_error);
  void OnReadCompleted(net::IOBuffer* buffer, int bytes_read);

  void AddConditionalHeaders(const net::HttpResponseHeaders* headers);
  void OnWriteComplete(int result);
  void ReadResponseData();
  bool ConsumeResponseData(net::IOBuffer* buffer, int bytes_read);
  void OnResponseCompleted(int net_error);
  bool MaybeRetryRequest();

  friend class UpdateURLRequest;
  friend class UpdateURLLoaderRequest;

  GURL url_;
  AppCacheUpdateJob* job_;
  FetchType fetch_type_;
  int retry_503_attempts_;
  std::unique_ptr<UpdateRequestBase> request_;
  AppCacheEntry existing_entry_;
  scoped_refptr<net::HttpResponseHeaders> existing_response_headers_;
  std::string manifest_data_;
  AppCacheUpdateJob::ResultType result_;
  int redirect_response_code_;
  std::unique_ptr<AppCacheResponseWriter> response_writer_;
  int buffer_size_;
};  // class URLFetcher

}  // namespace content.

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_FETCHER_H_