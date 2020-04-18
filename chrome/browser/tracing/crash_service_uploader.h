// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRACING_CRASH_SERVICE_UPLOADER_H_
#define CHROME_BROWSER_TRACING_CRASH_SERVICE_UPLOADER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/public/browser/trace_uploader.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

// TraceCrashServiceUploader uploads traces to the Chrome crash service.
class TraceCrashServiceUploader : public content::TraceUploader,
                                  public net::URLFetcherDelegate {
 public:
  explicit TraceCrashServiceUploader(
      net::URLRequestContextGetter* request_context);
  ~TraceCrashServiceUploader() override;

  void SetUploadURL(const std::string& url);

  void SetMaxUploadBytes(size_t max_upload_bytes);

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  void OnURLFetchUploadProgress(const net::URLFetcher* source,
                                int64_t current,
                                int64_t total) override;

  // content::TraceUploader
  void DoUpload(const std::string& file_contents,
                UploadMode upload_mode,
                std::unique_ptr<const base::DictionaryValue> metadata,
                const UploadProgressCallback& progress_callback,
                UploadDoneCallback done_callback) override;

 private:
  void DoCompressOnBackgroundThread(
      const std::string& file_contents,
      UploadMode upload_mode,
      const std::string& upload_url,
      std::unique_ptr<const base::DictionaryValue> metadata);

  // Sets up a multipart body to be uploaded. The body is produced according
  // to RFC 2046.
  void SetupMultipart(const std::string& product,
                      const std::string& version,
                      std::unique_ptr<const base::DictionaryValue> metadata,
                      const std::string& trace_filename,
                      const std::string& trace_contents,
                      std::string* post_data);
  void AddTraceFile(const std::string& trace_filename,
                    const std::string& trace_contents,
                    std::string* post_data);
  // Compresses the input and returns whether compression was successful.
  bool Compress(std::string input,
                int max_compressed_bytes,
                char* compressed_contents,
                int* compressed_bytes);
  void CreateAndStartURLFetcher(const std::string& upload_url,
                                const std::string& post_data);
  void OnUploadError(const std::string& error_message);

  std::unique_ptr<net::URLFetcher> url_fetcher_;
  UploadProgressCallback progress_callback_;
  UploadDoneCallback done_callback_;

  net::URLRequestContextGetter* request_context_;

  std::string upload_url_;

  size_t max_upload_bytes_;

  DISALLOW_COPY_AND_ASSIGN(TraceCrashServiceUploader);
};

#endif  // CHROME_BROWSER_TRACING_CRASH_SERVICE_UPLOADER_H_
