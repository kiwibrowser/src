// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_WRITE_TO_CACHE_JOB_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_WRITE_TO_CACHE_JOB_H_

#include <stdint.h>

#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_type.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace content {

class ServiceWorkerCacheWriter;
class ServiceWorkerContextCore;
class ServiceWorkerContextRequestHandlerTest;

// A URLRequestJob derivative used to cache the main script
// and its imports during the initial install of a new version.
// Another separate URLRequest is started which will perform
// a network fetch. The response produced for that separate
// request is written to the service worker script cache and piped
// to the consumer of the ServiceWorkerWriteToCacheJob for delivery
// to the renderer process housing the worker.
//
// For updates, the main script is not written to disk until a change with the
// incumbent script is detected. The incumbent script is progressively compared
// with the new script as it is read from network. Once a change is detected,
// everything that matched is copied to disk, and from then on the script is
// written as it continues to be read from network. If the scripts are
// identical, the resulting ServiceWorkerScriptCacheMap's main script status is
// set to kIdenticalScriptError.
class CONTENT_EXPORT ServiceWorkerWriteToCacheJob
    : public net::URLRequestJob,
      public net::URLRequest::Delegate {
 public:
  ServiceWorkerWriteToCacheJob(net::URLRequest* request,
                               net::NetworkDelegate* network_delegate,
                               ResourceType resource_type,
                               base::WeakPtr<ServiceWorkerContextCore> context,
                               ServiceWorkerVersion* version,
                               int extra_load_flags,
                               int64_t resource_id,
                               int64_t incumbent_resource_id);

  // The error code used when update fails because the new
  // script is byte-by-byte identical to the incumbent script.
  const static net::Error kIdenticalScriptError;

 private:
  friend class ServiceWorkerContextRequestHandlerTest;
  FRIEND_TEST_ALL_PREFIXES(ServiceWorkerContextRequestHandlerTest,
                           ServiceWorkerDataRequestAnnotation);

  ~ServiceWorkerWriteToCacheJob() override;

  // net::URLRequestJob overrides
  void Start() override;
  void StartAsync();
  void Kill() override;
  net::LoadState GetLoadState() const override;
  bool GetCharset(std::string* charset) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;

  const net::HttpResponseInfo* http_info() const;

  // Methods to drive the net request forward and
  // write data to the disk cache.
  void InitNetRequest(int extra_load_flags);
  void StartNetRequest();
  int ReadNetData(net::IOBuffer* buf, int buf_size);

  // Callbacks for writing headers and data via |cache_writer_|. Note that since
  // the MaybeWriteHeaders and MaybeWriteData methods on |cache_writer_| are
  // guaranteed not to do short writes, these functions only receive a
  // net::Error indicating success or failure, not a count of bytes written.
  void OnWriteHeadersComplete(net::Error error);
  void OnWriteDataComplete(net::Error error);

  // net::URLRequest::Delegate overrides that observe the net request.
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnAuthRequired(net::URLRequest* request,
                      net::AuthChallengeInfo* auth_info) override;
  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

  bool CheckPathRestriction(net::URLRequest* request);

  // Writes network data back to the script cache if needed, and notifies the
  // script cache of fetch completion at EOF. This function returns
  // net::IO_PENDING if the IO is to be completed asynchronously, returns a
  // negative number that represents a corresponding net error code (other than
  // net::IO_PENDING) if an error occurred, or returns a non-negative number
  // that represents the number of network bytes read. If the return value is
  // non-negative, all of the data in |io_buffer_| has been written back to the
  // script cache if necessary.
  int HandleNetData(int bytes_read);

  void NotifyStartErrorHelper(net::Error net_error,
                              const std::string& status_message);

  // Returns the error code, which is |net_error| or
  // a new one if an additional error is found.
  net::Error NotifyFinishedCaching(net::Error net_error,
                                   const std::string& status_message);

  // Returns true when the incumbent service worker exists and it's required to
  // do the byte-for-byte check.
  bool ShouldByteForByteCheck() const;

  const ResourceType resource_type_;  // Differentiate main script and imports
  scoped_refptr<net::IOBuffer> io_buffer_;
  int io_buffer_bytes_;
  base::WeakPtr<ServiceWorkerContextCore> context_;
  const GURL url_;
  const int64_t resource_id_;
  const int64_t incumbent_resource_id_;
  std::unique_ptr<net::URLRequest> net_request_;
  std::unique_ptr<net::HttpResponseInfo> http_info_;
  std::unique_ptr<ServiceWorkerResponseWriter> writer_;
  scoped_refptr<ServiceWorkerVersion> version_;
  std::unique_ptr<ServiceWorkerCacheWriter> cache_writer_;
  bool has_been_killed_ = false;
  bool did_notify_started_ = false;
  bool did_notify_finished_ = false;

  base::WeakPtrFactory<ServiceWorkerWriteToCacheJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerWriteToCacheJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_WRITE_TO_CACHE_JOB_H_
