// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_URL_REQUEST_POST_INTERCEPTOR_H_
#define COMPONENTS_UPDATE_CLIENT_URL_REQUEST_POST_INTERCEPTOR_H_

#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}

namespace net {
class HttpRequestHeaders;
}

namespace update_client {

class URLRequestMockJob;

// Intercepts requests to a file path, counts them, and captures the body of
// the requests. Optionally, for each request, it can return a canned response
// from a given file. The class maintains a queue of expectations, and returns
// one and only one response for each request that matches the expectation.
// Then, the expectation is removed from the queue.
class URLRequestPostInterceptor
    : public base::RefCountedThreadSafe<URLRequestPostInterceptor> {
 public:
  using InterceptedRequest = std::pair<std::string, net::HttpRequestHeaders>;

  // Called when the job associated with the url request which is intercepted
  // by this object has been created.
  using UrlJobRequestReadyCallback = base::OnceCallback<void()>;

  // Allows a generic string maching interface when setting up expectations.
  class RequestMatcher {
   public:
    virtual bool Match(const std::string& actual) const = 0;
    virtual ~RequestMatcher() {}
  };

  // Returns the url that is intercepted.
  GURL GetUrl() const;

  // Sets an expection for the body of the POST request and optionally,
  // provides a canned response identified by a |file_path| to be returned when
  // the expectation is met. If no |file_path| is provided, then an empty
  // response body is served. If |response_code| is provided, then an empty
  // response body with that response code is returned.
  // Returns |true| if the expectation was set.
  bool ExpectRequest(std::unique_ptr<RequestMatcher> request_matcher);
  bool ExpectRequest(std::unique_ptr<RequestMatcher> request_matcher,
                     int response_code);
  bool ExpectRequest(std::unique_ptr<RequestMatcher> request_matcher,
                     const base::FilePath& filepath);

  // Returns how many requests have been intercepted and matched by
  // an expectation. One expectation can only be matched by one request.
  int GetHitCount() const;

  // Returns how many requests in total have been captured by the interceptor.
  int GetCount() const;

  // Returns all requests that have been intercepted, matched or not.
  std::vector<InterceptedRequest> GetRequests() const;

  // Return the body of the n-th request, zero-based.
  std::string GetRequestBody(size_t n) const;

  // Returns the joined bodies of all requests for debugging purposes.
  std::string GetRequestsAsString() const;

  // Resets the state of the interceptor so that new expectations can be set.
  void Reset();

  // Prevents the intercepted request from starting, as a way to simulate
  // the effects of a very slow network. Call this function before the actual
  // network request occurs.
  void Pause();

  // Allows a previously paused request to continue.
  void Resume();

  // Sets a callback to be invoked when the request job associated with
  // an intercepted request is created. This allows the test execution to
  // synchronize with network tasks running on the IO thread and avoid polling
  // using idle run loops. A paused request can be resumed after this callback
  // has been invoked.
  void url_job_request_ready_callback(
      UrlJobRequestReadyCallback url_job_request_ready_callback);

 private:
  class Delegate;
  class URLRequestMockJob;

  friend class URLRequestPostInterceptorFactory;
  friend class base::RefCountedThreadSafe<URLRequestPostInterceptor>;

  static const int kResponseCode200 = 200;

  struct ExpectationResponse {
    ExpectationResponse(int code, const std::string& body)
        : response_code(code), response_body(body) {}
    const int response_code;
    const std::string response_body;
  };
  using Expectation =
      std::pair<std::unique_ptr<RequestMatcher>, ExpectationResponse>;

  URLRequestPostInterceptor(
      const GURL& url,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~URLRequestPostInterceptor();

  void ClearExpectations();

  const GURL url_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  mutable base::Lock interceptor_lock_;

  // Contains the count of the request matching expectations.
  int hit_count_;

  // Contains the request body and the extra headers of the intercepted
  // requests.
  std::vector<InterceptedRequest> requests_;

  // Contains the expectations which this interceptor tries to match.
  base::queue<Expectation> expectations_;

  URLRequestMockJob* request_job_ = nullptr;

  bool is_paused_ = false;

  UrlJobRequestReadyCallback url_job_request_ready_callback_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestPostInterceptor);
};

class URLRequestPostInterceptorFactory {
 public:
  URLRequestPostInterceptorFactory(
      const std::string& scheme,
      const std::string& hostname,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~URLRequestPostInterceptorFactory();

  // Creates an interceptor object for the specified url path.
  scoped_refptr<URLRequestPostInterceptor> CreateInterceptor(
      const base::FilePath& filepath);

 private:
  const std::string scheme_;
  const std::string hostname_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // After creation, |delegate_| lives on the IO thread and it is owned by
  // a URLRequestFilter after registration. A task to unregister it and
  // implicitly destroy it is posted from ~URLRequestPostInterceptorFactory().
  URLRequestPostInterceptor::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestPostInterceptorFactory);
};

// Intercepts HTTP POST requests sent to "localhost2".
class InterceptorFactory : public URLRequestPostInterceptorFactory {
 public:
  explicit InterceptorFactory(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~InterceptorFactory();

  // Creates an interceptor for the url path defined by POST_INTERCEPT_PATH.
  scoped_refptr<URLRequestPostInterceptor> CreateInterceptor();

  // Creates an interceptor for the given url path.
  scoped_refptr<URLRequestPostInterceptor> CreateInterceptorForPath(
      const char* url_path);

 private:
  DISALLOW_COPY_AND_ASSIGN(InterceptorFactory);
};

class PartialMatch : public URLRequestPostInterceptor::RequestMatcher {
 public:
  explicit PartialMatch(const std::string& expected) : expected_(expected) {}
  bool Match(const std::string& actual) const override;

 private:
  const std::string expected_;

  DISALLOW_COPY_AND_ASSIGN(PartialMatch);
};

class AnyMatch : public URLRequestPostInterceptor::RequestMatcher {
 public:
  AnyMatch() = default;
  bool Match(const std::string& actual) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AnyMatch);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_URL_REQUEST_POST_INTERCEPTOR_H_
