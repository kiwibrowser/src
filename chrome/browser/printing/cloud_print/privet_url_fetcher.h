// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_URL_FETCHER_H_
#define CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_URL_FETCHER_H_

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace base {
class FilePath;
}

namespace cloud_print {

// Privet-specific URLFetcher adapter. Currently supports only the subset
// of HTTP features required by Privet for GCP 1.5
// (/privet/info and /privet/register).
class PrivetURLFetcher : public net::URLFetcherDelegate {
 public:
  enum ErrorType {
    JSON_PARSE_ERROR,
    REQUEST_CANCELED,
    RESPONSE_CODE_ERROR,
    TOKEN_ERROR,
    UNKNOWN_ERROR,
  };

  using TokenCallback = base::OnceCallback<void(const std::string& /*token*/)>;

  class Delegate {
   public:
    virtual ~Delegate() {}

    // If you do not implement this method for PrivetV1 callers, you will always
    // get a TOKEN_ERROR error when your token is invalid.
    virtual void OnNeedPrivetToken(TokenCallback callback);

    // |response_code| is only needed for RESPONSE_CODE_ERROR.
    virtual void OnError(int response_code, ErrorType error) = 0;
    virtual void OnParsedJson(int response_code,
                              const base::DictionaryValue& value,
                              bool has_error) = 0;

    // If this method returns true, the data will not be parsed as JSON, and
    // OnParsedJson() will not be called. Otherwise, OnParsedJson() will be
    // called. This only happens in tests.
    virtual bool OnRawData(bool response_is_file,
                           const std::string& data_string,
                           const base::FilePath& data_file);
  };

  PrivetURLFetcher(
      const GURL& url,
      net::URLFetcher::RequestType request_type,
      const scoped_refptr<net::URLRequestContextGetter>& context_getter,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      Delegate* delegate);

  ~PrivetURLFetcher() override;

  // net::URLFetcherDelegate methods.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  static void SetTokenForHost(const std::string& host,
                              const std::string& token);

  static void ResetTokenMapForTest();

  void SetMaxRetriesForTest(int max_retries);

  void DoNotRetryOnTransientError();

  void SendEmptyPrivetToken();

  // Set the contents of the Range header. OnRawData() must return true if this
  // is called.
  void SetByteRange(int start, int end);

  // Save the response to a file. OnRawData() must return true if this is
  // called.
  void SaveResponseToFile();

  void Start();

  void SetUploadData(const std::string& upload_content_type,
                     const std::string& upload_data);

  const GURL& url() const {
    return url_fetcher_ ? url_fetcher_->GetOriginalURL() : url_;
  }
  int response_code() const {
    return url_fetcher_ ? url_fetcher_->GetResponseCode() : -1;
  }

 private:
  void OnURLFetchCompleteParseData(const net::URLFetcher* source);
  bool OnURLFetchCompleteDoNotParseData(const net::URLFetcher* source);

  std::string GetHostString();  // Get string representing the host.
  std::string GetPrivetAccessToken();
  void Try();
  void ScheduleRetry(int timeout_seconds);
  bool PrivetErrorTransient(const std::string& error);
  void RequestTokenRefresh();
  void RefreshToken(const std::string& token);

  // Lazily create |file_task_runner_| and return a reference.
  scoped_refptr<base::SequencedTaskRunner> GetFileTaskRunner();

  const GURL url_;
  const net::URLFetcher::RequestType request_type_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;
  Delegate* const delegate_;

  int max_retries_;
  bool do_not_retry_on_transient_error_ = false;
  bool send_empty_privet_token_ = false;
  bool has_byte_range_ = false;
  bool make_response_file_ = false;

  int byte_range_start_ = 0;
  int byte_range_end_ = 0;

  int tries_ = 0;
  std::string upload_data_;
  std::string upload_content_type_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<PrivetURLFetcher> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(PrivetURLFetcher);
};

}  // namespace cloud_print

#endif  // CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_URL_FETCHER_H_
