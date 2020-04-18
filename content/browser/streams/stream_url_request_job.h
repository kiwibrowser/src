// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_URL_REQUEST_JOB_H_
#define CONTENT_BROWSER_STREAMS_STREAM_URL_REQUEST_JOB_H_

#include "base/macros.h"
#include "content/browser/streams/stream_read_observer.h"
#include "content/common/content_export.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_range_request_job.h"

namespace content {

class Stream;

// A request job that handles reading stream URLs.
class CONTENT_EXPORT StreamURLRequestJob
    : public net::URLRangeRequestJob,
      public StreamReadObserver {
 public:
  StreamURLRequestJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate,
                      scoped_refptr<Stream> stream);

  // StreamObserver methods.
  void OnDataAvailable(Stream* stream) override;

  // net::URLRequestJob methods.
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int64_t GetTotalReceivedBytes() const override;
  int64_t prefilter_bytes_read() const override;

 protected:
  ~StreamURLRequestJob() override;

 private:
  void DidStart();
  void NotifyMethodNotSupported();
  void UpdateNetworkBytesRead();
  void ClearStream();

  scoped_refptr<content::Stream> stream_;
  scoped_refptr<net::IOBuffer> pending_buffer_;
  int pending_buffer_size_;

  // Total bytes received for this job.
  int total_bytes_read_;
  int64_t raw_body_bytes_;

  int max_range_;
  bool request_failed_;
  std::unique_ptr<net::HttpResponseInfo> failed_response_info_;
  int error_code_;  // Only set if request_failed_.

  base::WeakPtrFactory<StreamURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StreamURLRequestJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_URL_REQUEST_JOB_H_
