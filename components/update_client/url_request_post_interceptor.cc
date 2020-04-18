// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/url_request_post_interceptor.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "components/update_client/test_configurator.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_simple_job.h"
#include "net/url_request/url_request_test_util.h"

namespace update_client {

// Returns a canned response.
class URLRequestPostInterceptor::URLRequestMockJob
    : public net::URLRequestSimpleJob {
 public:
  URLRequestMockJob(scoped_refptr<URLRequestPostInterceptor> interceptor,
                    net::URLRequest* request,
                    net::NetworkDelegate* network_delegate,
                    int response_code,
                    const std::string& response_body)
      : net::URLRequestSimpleJob(request, network_delegate),
        interceptor_(interceptor),
        response_code_(response_code),
        response_body_(response_body) {}

  void Start() override {
    if (interceptor_->is_paused_)
      return;
    net::URLRequestSimpleJob::Start();
  }

 protected:
  void GetResponseInfo(net::HttpResponseInfo* info) override {
    const std::string headers =
        base::StringPrintf("HTTP/1.1 %i OK\r\n\r\n", response_code_);
    info->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.length()));
  }

  int GetData(std::string* mime_type,
              std::string* charset,
              std::string* data,
              const net::CompletionCallback& callback) const override {
    mime_type->assign("text/plain");
    charset->assign("US-ASCII");
    data->assign(response_body_);
    return net::OK;
  }

 private:
  ~URLRequestMockJob() override {}

  scoped_refptr<URLRequestPostInterceptor> interceptor_;

  int response_code_;
  std::string response_body_;
  DISALLOW_COPY_AND_ASSIGN(URLRequestMockJob);
};

URLRequestPostInterceptor::URLRequestPostInterceptor(
    const GURL& url,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : url_(url), io_task_runner_(io_task_runner), hit_count_(0) {}

URLRequestPostInterceptor::~URLRequestPostInterceptor() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
}

GURL URLRequestPostInterceptor::GetUrl() const {
  return url_;
}

bool URLRequestPostInterceptor::ExpectRequest(
    std::unique_ptr<RequestMatcher> request_matcher) {
  return ExpectRequest(std::move(request_matcher), kResponseCode200);
}

bool URLRequestPostInterceptor::ExpectRequest(
    std::unique_ptr<RequestMatcher> request_matcher,
    int response_code) {
  expectations_.push(
      {std::move(request_matcher), ExpectationResponse(response_code, "")});
  return true;
}

bool URLRequestPostInterceptor::ExpectRequest(
    std::unique_ptr<RequestMatcher> request_matcher,
    const base::FilePath& filepath) {
  std::string response;
  if (filepath.empty() || !base::ReadFileToString(filepath, &response))
    return false;

  expectations_.push({std::move(request_matcher),
                      ExpectationResponse(kResponseCode200, response)});
  return true;
}

int URLRequestPostInterceptor::GetHitCount() const {
  base::AutoLock auto_lock(interceptor_lock_);
  return hit_count_;
}

int URLRequestPostInterceptor::GetCount() const {
  base::AutoLock auto_lock(interceptor_lock_);
  return static_cast<int>(requests_.size());
}

std::vector<URLRequestPostInterceptor::InterceptedRequest>
URLRequestPostInterceptor::GetRequests() const {
  base::AutoLock auto_lock(interceptor_lock_);
  return requests_;
}

std::string URLRequestPostInterceptor::GetRequestBody(size_t n) const {
  base::AutoLock auto_lock(interceptor_lock_);
  return requests_[n].first;
}

std::string URLRequestPostInterceptor::GetRequestsAsString() const {
  const std::vector<InterceptedRequest> requests = GetRequests();

  std::string s = "Requests are:";

  int i = 0;
  for (auto it = requests.cbegin(); it != requests.cend(); ++it)
    s.append(base::StringPrintf("\n  [%d]: %s", ++i, it->first.c_str()));

  return s;
}

void URLRequestPostInterceptor::Reset() {
  base::AutoLock auto_lock(interceptor_lock_);
  hit_count_ = 0;
  requests_.clear();
  base::queue<Expectation>().swap(expectations_);
}

void URLRequestPostInterceptor::Pause() {
  base::AutoLock auto_lock(interceptor_lock_);
  is_paused_ = true;
}

void URLRequestPostInterceptor::Resume() {
  base::AutoLock auto_lock(interceptor_lock_);
  is_paused_ = false;
  io_task_runner_->PostTask(FROM_HERE,
                            base::BindOnce(&URLRequestMockJob::Start,
                                           base::Unretained(request_job_)));
}

void URLRequestPostInterceptor::url_job_request_ready_callback(
    UrlJobRequestReadyCallback url_job_request_ready_callback) {
  base::AutoLock auto_lock(interceptor_lock_);
  url_job_request_ready_callback_ = std::move(url_job_request_ready_callback);
}

class URLRequestPostInterceptor::Delegate : public net::URLRequestInterceptor {
 public:
  Delegate(const std::string& scheme,
           const std::string& hostname,
           scoped_refptr<base::SequencedTaskRunner> io_task_runner)
      : scheme_(scheme), hostname_(hostname), io_task_runner_(io_task_runner) {}

  void Register() {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    net::URLRequestFilter::GetInstance()->AddHostnameInterceptor(
        scheme_, hostname_, std::unique_ptr<net::URLRequestInterceptor>(this));
  }

  void Unregister() {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    interceptors_.clear();
    net::URLRequestFilter::GetInstance()->RemoveHostnameHandler(scheme_,
                                                                hostname_);
  }

  void OnCreateInterceptor(
      scoped_refptr<URLRequestPostInterceptor> interceptor) {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    DCHECK_EQ(0u, interceptors_.count(interceptor->GetUrl()));

    interceptors_.insert(std::make_pair(interceptor->GetUrl(), interceptor));
  }

 private:
  ~Delegate() override {}

  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

    // Only intercepts POST.
    if (!request->has_upload())
      return nullptr;

    GURL url = request->url();
    if (url.has_query()) {
      GURL::Replacements replacements;
      replacements.ClearQuery();
      url = url.ReplaceComponents(replacements);
    }

    const auto it = interceptors_.find(url);
    if (it == interceptors_.end())
      return nullptr;

    // There is an interceptor hooked up for this url. Read the request body,
    // check the existing expectations, and handle the matching case by
    // popping the expectation off the queue, counting the match, and
    // returning a mock object to serve the canned response.
    auto interceptor = it->second;

    const net::UploadDataStream* stream = request->get_upload();
    const net::UploadBytesElementReader* reader =
        (*stream->GetElementReaders())[0]->AsBytesReader();
    const int size = reader->length();
    scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(size));
    const std::string request_body(reader->bytes());

    {
      base::AutoLock auto_lock(interceptor->interceptor_lock_);
      interceptor->requests_.push_back(
          {request_body, request->extra_request_headers()});
      if (interceptor->expectations_.empty())
        return nullptr;
      const auto& expectation = interceptor->expectations_.front();
      if (expectation.first->Match(request_body)) {
        const int response_code(expectation.second.response_code);
        const std::string response_body(expectation.second.response_body);
        interceptor->expectations_.pop();
        ++interceptor->hit_count_;
        interceptor->request_job_ =
            new URLRequestMockJob(interceptor, request, network_delegate,
                                  response_code, response_body);
        if (interceptor->url_job_request_ready_callback_) {
          io_task_runner_->PostTask(
              FROM_HERE,
              std::move(interceptor->url_job_request_ready_callback_));
        }
        return interceptor->request_job_;
      }
    }

    return nullptr;
  }

  using InterceptorMap =
      std::map<GURL, scoped_refptr<URLRequestPostInterceptor>>;
  InterceptorMap interceptors_;

  const std::string scheme_;
  const std::string hostname_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

URLRequestPostInterceptorFactory::URLRequestPostInterceptorFactory(
    const std::string& scheme,
    const std::string& hostname,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : scheme_(scheme),
      hostname_(hostname),
      io_task_runner_(io_task_runner),
      delegate_(new URLRequestPostInterceptor::Delegate(scheme,
                                                        hostname,
                                                        io_task_runner)) {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&URLRequestPostInterceptor::Delegate::Register,
                                base::Unretained(delegate_)));
}

URLRequestPostInterceptorFactory::~URLRequestPostInterceptorFactory() {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&URLRequestPostInterceptor::Delegate::Unregister,
                     base::Unretained(delegate_)));
}

scoped_refptr<URLRequestPostInterceptor>
URLRequestPostInterceptorFactory::CreateInterceptor(
    const base::FilePath& filepath) {
  const GURL base_url(
      base::StringPrintf("%s://%s", scheme_.c_str(), hostname_.c_str()));
  GURL absolute_url(base_url.Resolve(filepath.MaybeAsASCII()));
  auto interceptor = scoped_refptr<URLRequestPostInterceptor>(
      new URLRequestPostInterceptor(absolute_url, io_task_runner_));
  bool res = io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&URLRequestPostInterceptor::Delegate::OnCreateInterceptor,
                     base::Unretained(delegate_), interceptor));
  return res ? interceptor : nullptr;
}

bool PartialMatch::Match(const std::string& actual) const {
  return actual.find(expected_) != std::string::npos;
}

bool AnyMatch::Match(const std::string&) const {
  return true;
}

InterceptorFactory::InterceptorFactory(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : URLRequestPostInterceptorFactory(POST_INTERCEPT_SCHEME,
                                       POST_INTERCEPT_HOSTNAME,
                                       io_task_runner) {}

InterceptorFactory::~InterceptorFactory() {
}

scoped_refptr<URLRequestPostInterceptor>
InterceptorFactory::CreateInterceptor() {
  return CreateInterceptorForPath(POST_INTERCEPT_PATH);
}

scoped_refptr<URLRequestPostInterceptor>
InterceptorFactory::CreateInterceptorForPath(const char* url_path) {
  return URLRequestPostInterceptorFactory::CreateInterceptor(
      base::FilePath::FromUTF8Unsafe(url_path));
}

}  // namespace update_client
