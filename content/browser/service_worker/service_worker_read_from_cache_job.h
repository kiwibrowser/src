// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_READ_FROM_CACHE_JOB_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_READ_FROM_CACHE_JOB_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request_job.h"

namespace content {

class ServiceWorkerContextCore;
class ServiceWorkerResponseReader;
class ServiceWorkerVersion;

// A URLRequestJob derivative used to retrieve script resources
// from the service workers script cache. It uses a response reader
// and pipes the response to the consumer of this url request job.
class CONTENT_EXPORT ServiceWorkerReadFromCacheJob
    : public net::URLRequestJob {
 public:
  ServiceWorkerReadFromCacheJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      ResourceType resource_type,
      base::WeakPtr<ServiceWorkerContextCore> context,
      const scoped_refptr<ServiceWorkerVersion>& version,
      int64_t resource_id);
  ~ServiceWorkerReadFromCacheJob() override;

 private:
  friend class ServiceWorkerReadFromCacheJobTest;
  FRIEND_TEST_ALL_PREFIXES(ServiceWorkerContextRequestHandlerTestP,
                           DuplicateScriptImport);

  bool is_main_script() const {
    return resource_type_ == RESOURCE_TYPE_SERVICE_WORKER;
  }

  // net::URLRequestJob overrides
  void Start() override;
  void Kill() override;
  net::LoadState GetLoadState() const override;
  bool GetCharset(std::string* charset) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;

  // Reader completion callbacks.
  void OnReadInfoComplete(int result);
  void OnReadComplete(int result);

  // Helpers
  void StartAsync();
  const net::HttpResponseInfo* http_info() const;
  bool is_range_request() const { return range_requested_.IsValid(); }
  void SetupRangeResponse(int response_data_size);
  void Done(const net::URLRequestStatus& status);

  const ResourceType resource_type_;
  const int64_t resource_id_;

  base::WeakPtr<ServiceWorkerContextCore> context_;
  scoped_refptr<ServiceWorkerVersion> version_;
  std::unique_ptr<ServiceWorkerResponseReader> reader_;
  scoped_refptr<HttpResponseInfoIOBuffer> http_info_io_buffer_;
  std::unique_ptr<net::HttpResponseInfo> http_info_;
  net::HttpByteRange range_requested_;
  std::unique_ptr<net::HttpResponseInfo> range_response_info_;
  bool has_been_killed_ = false;

  base::WeakPtrFactory<ServiceWorkerReadFromCacheJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerReadFromCacheJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_READ_FROM_CACHE_JOB_H_
