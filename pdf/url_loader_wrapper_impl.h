// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_URL_LOADER_WRAPPER_IMPL_H_
#define PDF_URL_LOADER_WRAPPER_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "pdf/url_loader_wrapper.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ui/gfx/range/range.h"

namespace pp {
class Instance;
};

namespace chrome_pdf {

class URLLoaderWrapperImpl : public URLLoaderWrapper {
 public:
  URLLoaderWrapperImpl(pp::Instance* plugin_instance,
                       const pp::URLLoader& url_loader);
  ~URLLoaderWrapperImpl() override;

  // URLLoaderWrapper overrides:
  int GetContentLength() const override;
  bool IsAcceptRangesBytes() const override;
  bool IsContentEncoded() const override;
  std::string GetContentType() const override;
  std::string GetContentDisposition() const override;
  int GetStatusCode() const override;
  bool IsMultipart() const override;
  bool GetByteRangeStart(int* start) const override;
  bool GetDownloadProgress(int64_t* bytes_received,
                           int64_t* total_bytes_to_be_received) const override;
  void Close() override;
  void OpenRange(const std::string& url,
                 const std::string& referrer_url,
                 uint32_t position,
                 uint32_t size,
                 const pp::CompletionCallback& cc) override;
  void ReadResponseBody(char* buffer,
                        int buffer_size,
                        const pp::CompletionCallback& cc) override;

  void SetResponseHeaders(const std::string& response_headers);

 private:
  class ReadStarter;

  void SetHeadersFromLoader();
  void ParseHeaders();
  void DidOpen(int32_t result);
  void DidRead(int32_t result);

  void ReadResponseBodyImpl();

  pp::Instance* const plugin_instance_;
  pp::URLLoader url_loader_;
  std::string response_headers_;

  int content_length_ = -1;
  bool accept_ranges_bytes_ = false;
  bool content_encoded_ = false;
  std::string content_type_;
  std::string content_disposition_;
  std::string multipart_boundary_;
  gfx::Range byte_range_ = gfx::Range::InvalidRange();
  bool is_multipart_ = false;
  char* buffer_ = nullptr;
  uint32_t buffer_size_ = 0;
  bool multi_part_processed_ = false;

  pp::CompletionCallback did_open_callback_;
  pp::CompletionCallback did_read_callback_;
  pp::CompletionCallbackFactory<URLLoaderWrapperImpl> callback_factory_;

  std::unique_ptr<ReadStarter> read_starter_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderWrapperImpl);
};

}  // namespace chrome_pdf

#endif  // PDF_URL_LOADER_WRAPPER_IMPL_H_
