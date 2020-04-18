// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_URL_REQUEST_JOB_H_
#define STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_URL_REQUEST_JOB_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/net_errors.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request_job.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/storage_browser_export.h"

class GURL;

namespace storage {
class FileStreamReader;
}

namespace storage {
class FileSystemContext;

// A request job that handles reading filesystem: URLs
class STORAGE_EXPORT FileSystemURLRequestJob : public net::URLRequestJob {
 public:
  FileSystemURLRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const std::string& storage_domain,
      FileSystemContext* file_system_context);

  // URLRequestJob methods:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool IsRedirectResponse(GURL* location, int* http_status_code) override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;

  // FilterContext methods (via URLRequestJob):
  bool GetMimeType(std::string* mime_type) const override;

 private:
  class CallbackDispatcher;

  ~FileSystemURLRequestJob() override;

  void StartAsync();
  void DidAttemptAutoMount(base::File::Error result);
  void DidGetMetadata(base::File::Error error_code,
                      const base::File::Info& file_info);
  void DidRead(int result);

  const std::string storage_domain_;
  FileSystemContext* file_system_context_;
  std::unique_ptr<storage::FileStreamReader> reader_;
  FileSystemURL url_;
  bool is_directory_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;
  int64_t remaining_bytes_;
  net::Error range_parse_result_;
  net::HttpByteRange byte_range_;
  base::WeakPtrFactory<FileSystemURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemURLRequestJob);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_URL_REQUEST_JOB_H_
