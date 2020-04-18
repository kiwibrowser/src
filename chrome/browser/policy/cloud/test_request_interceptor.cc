// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/test_request_interceptor.h"

#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_test_job.h"
#include "url/gurl.h"

namespace em = enterprise_management;

namespace policy {

namespace {

// Helper callback for jobs that should fail with a network |error|.
net::URLRequestJob* ErrorJobCallback(int error,
                                     net::URLRequest* request,
                                     net::NetworkDelegate* network_delegate) {
  return new net::URLRequestErrorJob(request, network_delegate, error);
}

// Helper callback for jobs that should fail with the specified HTTP error.
net::URLRequestJob* HttpErrorJobCallback(
    const std::string& error,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  std::string headers =
      "HTTP/1.1 " + error + "\nContent-type: application/protobuf\n\n";
  return new net::URLRequestTestJob(
      request, network_delegate, headers, std::string(), true);
}

// Helper callback for jobs that should fail with a 400 HTTP error.
net::URLRequestJob* BadRequestJobCallback(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  return HttpErrorJobCallback("400 Bad request", request, network_delegate);
}

net::URLRequestJob* FileJobCallback(const base::FilePath& file_path,
                                    net::URLRequest* request,
                                    net::NetworkDelegate* network_delegate) {
  return new net::URLRequestMockHTTPJob(request, network_delegate, file_path);
}

// Parses the upload data in |request| into |request_msg|, and validates the
// request. The query string in the URL must contain the |expected_type| for
// the "request" parameter. Returns true if all checks succeeded, and the
// request data has been parsed into |request_msg|.
bool ValidRequest(net::URLRequest* request,
                  const std::string& expected_type,
                  em::DeviceManagementRequest* request_msg) {
  if (request->method() != "POST")
    return false;
  std::string spec = request->url().spec();
  if (spec.find("request=" + expected_type) == std::string::npos)
    return false;

  // This assumes that the payload data was set from a single string. In that
  // case the UploadDataStream has a single UploadBytesElementReader with the
  // data in memory.
  const net::UploadDataStream* stream = request->get_upload();
  if (!stream)
    return false;
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      stream->GetElementReaders();
  if (!readers || readers->size() != 1u)
    return false;
  const net::UploadBytesElementReader* reader = (*readers)[0]->AsBytesReader();
  if (!reader)
    return false;
  std::string data(reader->bytes(), reader->length());
  if (!request_msg->ParseFromString(data))
    return false;

  return true;
}

// Helper callback for register jobs that should suceed. Validates the request
// parameters and returns an appropriate response job. If |expect_reregister|
// is true then the reregister flag must be set in the DeviceRegisterRequest
// protobuf.
net::URLRequestJob* RegisterJobCallback(
    em::DeviceRegisterRequest::Type expected_type,
    bool expect_reregister,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  em::DeviceManagementRequest request_msg;
  if (!ValidRequest(request, "register", &request_msg))
    return BadRequestJobCallback(request, network_delegate);

  if (!request_msg.has_register_request() ||
      request_msg.has_unregister_request() ||
      request_msg.has_policy_request() ||
      request_msg.has_device_status_report_request() ||
      request_msg.has_session_status_report_request() ||
      request_msg.has_auto_enrollment_request()) {
    return BadRequestJobCallback(request, network_delegate);
  }

  const em::DeviceRegisterRequest& register_request =
      request_msg.register_request();
  if (expect_reregister &&
      (!register_request.has_reregister() || !register_request.reregister())) {
    return BadRequestJobCallback(request, network_delegate);
  } else if (!expect_reregister &&
             register_request.has_reregister() &&
             register_request.reregister()) {
    return BadRequestJobCallback(request, network_delegate);
  }

  if (!register_request.has_type() || register_request.type() != expected_type)
    return BadRequestJobCallback(request, network_delegate);

  em::DeviceManagementResponse response;
  em::DeviceRegisterResponse* register_response =
      response.mutable_register_response();
  register_response->set_device_management_token("s3cr3t70k3n");
  std::string data;
  response.SerializeToString(&data);

  static const char kGoodHeaders[] =
      "HTTP/1.1 200 OK\n"
      "Content-type: application/protobuf\n"
      "\n";
  std::string headers(kGoodHeaders, arraysize(kGoodHeaders));
  return new net::URLRequestTestJob(
      request, network_delegate, headers, data, true);
}

void RegisterHttpInterceptor(
    const std::string& hostname,
    std::unique_ptr<net::URLRequestInterceptor> interceptor) {
  net::URLRequestFilter::GetInstance()->AddHostnameInterceptor(
      "http", hostname, std::move(interceptor));
}

void UnregisterHttpInterceptor(const std::string& hostname) {
  net::URLRequestFilter::GetInstance()->RemoveHostnameHandler("http", hostname);
}

}  // namespace

class TestRequestInterceptor::Delegate : public net::URLRequestInterceptor {
 public:
  Delegate(const std::string& hostname,
           scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~Delegate() override;

  // net::URLRequestInterceptor implementation:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  void GetPendingSize(size_t* pending_size) const;
  void AddRequestServicedCallback(const base::Closure& callback);
  void PushJobCallback(const JobCallback& callback);

 private:
  static void InvokeRequestServicedCallbacks(
      std::unique_ptr<std::vector<base::Closure>> callbacks);

  const std::string hostname_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // The queue of pending callbacks. 'mutable' because MaybeCreateJob() is a
  // const method; it can't reenter though, because it runs exclusively on
  // the IO thread.
  mutable base::queue<JobCallback> pending_job_callbacks_;

  // Queue of pending request serviced callbacks. Mutable for the same reason
  // as |pending_job_callbacks_|.
  mutable std::vector<base::Closure> request_serviced_callbacks_;
};

TestRequestInterceptor::Delegate::Delegate(
    const std::string& hostname,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : hostname_(hostname), io_task_runner_(io_task_runner) {}

TestRequestInterceptor::Delegate::~Delegate() {}

net::URLRequestJob* TestRequestInterceptor::Delegate::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  CHECK(io_task_runner_->RunsTasksInCurrentSequence());

  if (request->url().host_piece() != hostname_) {
    // Reject requests to other servers.
    return ErrorJobCallback(
        net::ERR_CONNECTION_REFUSED, request, network_delegate);
  }

  if (pending_job_callbacks_.empty()) {
    // Reject dmserver requests by default.
    return BadRequestJobCallback(request, network_delegate);
  }

  // Invoke any callbacks that are waiting for the next request to be serviced
  // after this job is serviced.
  if (!request_serviced_callbacks_.empty()) {
    std::unique_ptr<std::vector<base::Closure>> callbacks(
        new std::vector<base::Closure>);
    callbacks->swap(request_serviced_callbacks_);
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&Delegate::InvokeRequestServicedCallbacks,
                                  std::move(callbacks)));
  }

  JobCallback callback = pending_job_callbacks_.front();
  pending_job_callbacks_.pop();
  return callback.Run(request, network_delegate);
}

void TestRequestInterceptor::Delegate::GetPendingSize(
    size_t* pending_size) const {
  CHECK(io_task_runner_->RunsTasksInCurrentSequence());
  *pending_size = pending_job_callbacks_.size();
}

void TestRequestInterceptor::Delegate::AddRequestServicedCallback(
    const base::Closure& callback) {
  CHECK(io_task_runner_->RunsTasksInCurrentSequence());
  request_serviced_callbacks_.push_back(callback);
}

void TestRequestInterceptor::Delegate::PushJobCallback(
    const JobCallback& callback) {
  CHECK(io_task_runner_->RunsTasksInCurrentSequence());
  pending_job_callbacks_.push(callback);
}

// static
void TestRequestInterceptor::Delegate::InvokeRequestServicedCallbacks(
    std::unique_ptr<std::vector<base::Closure>> callbacks) {
  for (const auto& p : *callbacks)
    p.Run();
}

TestRequestInterceptor::TestRequestInterceptor(const std::string& hostname,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : hostname_(hostname),
      io_task_runner_(io_task_runner) {
  delegate_ = new Delegate(hostname_, io_task_runner_);
  std::unique_ptr<net::URLRequestInterceptor> interceptor(delegate_);
  PostToIOAndWait(
      base::Bind(&RegisterHttpInterceptor, hostname_,
                 base::Passed(&interceptor)));
}

TestRequestInterceptor::~TestRequestInterceptor() {
  // RemoveHostnameHandler() destroys the |delegate_|, which is owned by
  // the URLRequestFilter.
  delegate_ = NULL;
  PostToIOAndWait(base::Bind(&UnregisterHttpInterceptor, hostname_));
}

size_t TestRequestInterceptor::GetPendingSize() {
  size_t pending_size = std::numeric_limits<size_t>::max();
  PostToIOAndWait(base::Bind(&Delegate::GetPendingSize,
                             base::Unretained(delegate_),
                             &pending_size));
  return pending_size;
}

void TestRequestInterceptor::AddRequestServicedCallback(
    const base::Closure& callback) {
  base::Closure post_callback =
      base::Bind(base::IgnoreResult(&base::TaskRunner::PostTask),
                 base::ThreadTaskRunnerHandle::Get(),
                 FROM_HERE,
                 callback);
  PostToIOAndWait(base::Bind(&Delegate::AddRequestServicedCallback,
                             base::Unretained(delegate_),
                             post_callback));
}

void TestRequestInterceptor::PushJobCallback(const JobCallback& callback) {
  PostToIOAndWait(base::Bind(&Delegate::PushJobCallback,
                             base::Unretained(delegate_),
                             callback));
}

// static
TestRequestInterceptor::JobCallback TestRequestInterceptor::ErrorJob(
    int error) {
  return base::Bind(&ErrorJobCallback, error);
}

// static
TestRequestInterceptor::JobCallback TestRequestInterceptor::BadRequestJob() {
  return base::Bind(&BadRequestJobCallback);
}

// static
TestRequestInterceptor::JobCallback TestRequestInterceptor::HttpErrorJob(
    std::string error) {
  return base::Bind(&HttpErrorJobCallback, error);
}

// static
TestRequestInterceptor::JobCallback TestRequestInterceptor::RegisterJob(
    em::DeviceRegisterRequest::Type expected_type,
    bool expect_reregister) {
  return base::Bind(&RegisterJobCallback, expected_type, expect_reregister);
}

// static
TestRequestInterceptor::JobCallback TestRequestInterceptor::FileJob(
    const base::FilePath& file_path) {
  return base::Bind(&FileJobCallback, file_path);
}

void TestRequestInterceptor::PostToIOAndWait(const base::Closure& task) {
  io_task_runner_->PostTask(FROM_HERE, task);
  base::RunLoop run_loop;
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(base::IgnoreResult(&base::TaskRunner::PostTask),
                                base::ThreadTaskRunnerHandle::Get(), FROM_HERE,
                                run_loop.QuitClosure()));
  run_loop.Run();
}

}  // namespace policy
