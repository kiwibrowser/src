// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/android_stream_reader_url_request_job.h"

#include <memory>
#include <string>
#include <utility>

#include "android_webview/browser/input_stream.h"
#include "android_webview/browser/net/input_stream_reader.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job_manager.h"

using base::android::AttachCurrentThread;

namespace android_webview {

namespace {

const int kHTTPOk = 200;
const int kHTTPNotFound = 404;

const char kResponseHeaderViaShouldInterceptRequest[] =
    "Client-Via: shouldInterceptRequest";
const char kHTTPOkText[] = "OK";
const char kHTTPNotFoundText[] = "Not Found";

} // namespace

// The requests posted to the worker thread might outlive the job.  Thread-safe
// ref counting is used to ensure that the InputStream and InputStreamReader
// members of this class are still there when the closure is run on the worker
// thread.
class InputStreamReaderWrapper :
    public base::RefCountedThreadSafe<InputStreamReaderWrapper> {
 public:
  InputStreamReaderWrapper(
      std::unique_ptr<InputStream> input_stream,
      std::unique_ptr<InputStreamReader> input_stream_reader)
      : input_stream_(std::move(input_stream)),
        input_stream_reader_(std::move(input_stream_reader)) {
    DCHECK(input_stream_);
    DCHECK(input_stream_reader_);
  }

  InputStream* input_stream() {
    return input_stream_.get();
  }

  int Seek(const net::HttpByteRange& byte_range) {
    return input_stream_reader_->Seek(byte_range);
  }

  int ReadRawData(net::IOBuffer* buffer, int buffer_size) {
    return input_stream_reader_->ReadRawData(buffer, buffer_size);
  }

 private:
  friend class base::RefCountedThreadSafe<InputStreamReaderWrapper>;
  ~InputStreamReaderWrapper() {}

  std::unique_ptr<InputStream> input_stream_;
  std::unique_ptr<InputStreamReader> input_stream_reader_;

  DISALLOW_COPY_AND_ASSIGN(InputStreamReaderWrapper);
};

AndroidStreamReaderURLRequestJob::AndroidStreamReaderURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    std::unique_ptr<Delegate> delegate)
    : URLRequestJob(request, network_delegate),
      range_parse_result_(net::OK),
      delegate_(std::move(delegate)),
      weak_factory_(this) {
  DCHECK(delegate_);
}

AndroidStreamReaderURLRequestJob::AndroidStreamReaderURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    std::unique_ptr<DelegateObtainer> delegate_obtainer,
    bool)
    : URLRequestJob(request, network_delegate),
      range_parse_result_(net::OK),
      delegate_obtainer_(std::move(delegate_obtainer)),
      weak_factory_(this) {
  DCHECK(delegate_obtainer_);
}

AndroidStreamReaderURLRequestJob::~AndroidStreamReaderURLRequestJob() {
}

namespace {

using OnInputStreamOpenedCallback = base::OnceCallback<void(
    std::unique_ptr<AndroidStreamReaderURLRequestJob::Delegate>,
    std::unique_ptr<InputStream>)>;

// static
void OpenInputStreamOnWorkerThread(
    scoped_refptr<base::SingleThreadTaskRunner> job_thread_task_runner,
    std::unique_ptr<AndroidStreamReaderURLRequestJob::Delegate> delegate,
    const GURL& url,
    OnInputStreamOpenedCallback callback) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  std::unique_ptr<InputStream> input_stream =
      delegate->OpenInputStream(env, url);
  job_thread_task_runner->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(delegate),
                                std::move(input_stream)));
}

} // namespace

void AndroidStreamReaderURLRequestJob::Start() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!delegate_) {
    DCHECK(delegate_obtainer_);
    delegate_obtainer_->ObtainDelegate(
        request(),
        base::BindOnce(&AndroidStreamReaderURLRequestJob::DelegateObtained,
                       weak_factory_.GetWeakPtr()));
  } else {
    // Run DoStart asynchronously to avoid re-entering the delegate.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&AndroidStreamReaderURLRequestJob::DoStart,
                                  weak_factory_.GetWeakPtr()));
  }
}

void AndroidStreamReaderURLRequestJob::Kill() {
  DCHECK(thread_checker_.CalledOnValidThread());
  weak_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

std::unique_ptr<InputStreamReader>
AndroidStreamReaderURLRequestJob::CreateStreamReader(InputStream* stream) {
  return std::make_unique<InputStreamReader>(stream);
}

void AndroidStreamReaderURLRequestJob::OnInputStreamOpened(
    std::unique_ptr<Delegate> returned_delegate,
    std::unique_ptr<android_webview::InputStream> input_stream) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(returned_delegate);
  delegate_ = std::move(returned_delegate);

  if (!input_stream) {
    bool restart_required = false;
    delegate_->OnInputStreamOpenFailed(request(), &restart_required);
    if (restart_required) {
      NotifyRestartRequired();
    } else {
      HeadersComplete(kHTTPNotFound, kHTTPNotFoundText);
    }
    return;
  }

  std::unique_ptr<InputStreamReader> input_stream_reader(
      CreateStreamReader(input_stream.get()));
  DCHECK(input_stream_reader);

  DCHECK(!input_stream_reader_wrapper_.get());
  input_stream_reader_wrapper_ = new InputStreamReaderWrapper(
      std::move(input_stream), std::move(input_stream_reader));

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&InputStreamReaderWrapper::Seek,
                     input_stream_reader_wrapper_, byte_range_),
      base::BindOnce(&AndroidStreamReaderURLRequestJob::OnReaderSeekCompleted,
                     weak_factory_.GetWeakPtr()));
}

void AndroidStreamReaderURLRequestJob::OnReaderSeekCompleted(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (result >= 0) {
    set_expected_content_size(result);
    HeadersComplete(kHTTPOk, kHTTPOkText);
  } else {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, result));
  }
}

void AndroidStreamReaderURLRequestJob::OnReaderReadCompleted(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  ReadRawDataComplete(result);
}

int AndroidStreamReaderURLRequestJob::ReadRawData(net::IOBuffer* dest,
                                                  int dest_size) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!input_stream_reader_wrapper_.get()) {
    // This will happen if opening the InputStream fails in which case the
    // error is communicated by setting the HTTP response status header rather
    // than failing the request during the header fetch phase.
    return 0;
  }

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&InputStreamReaderWrapper::ReadRawData,
                     input_stream_reader_wrapper_, base::RetainedRef(dest),
                     dest_size),
      base::BindOnce(&AndroidStreamReaderURLRequestJob::OnReaderReadCompleted,
                     weak_factory_.GetWeakPtr()));

  return net::ERR_IO_PENDING;
}

bool AndroidStreamReaderURLRequestJob::GetMimeType(
    std::string* mime_type) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  if (!input_stream_reader_wrapper_.get())
    return false;

  return delegate_->GetMimeType(
      env, request(), input_stream_reader_wrapper_->input_stream(), mime_type);
}

bool AndroidStreamReaderURLRequestJob::GetCharset(std::string* charset) {
  DCHECK(thread_checker_.CalledOnValidThread());
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  if (!input_stream_reader_wrapper_.get())
    return false;

  // Since it's possible for this call to alter the InputStream a
  // Seek or ReadRawData operation running in the background is not permitted.
  DCHECK(!request_->status().is_io_pending());

  return delegate_->GetCharset(
      env, request(), input_stream_reader_wrapper_->input_stream(), charset);
}

void AndroidStreamReaderURLRequestJob::DelegateObtained(
    std::unique_ptr<Delegate> delegate) {
  DCHECK(!delegate_);
  delegate_obtainer_.reset();
  if (delegate) {
    delegate_.swap(delegate);
    DoStart();
  } else {
    NotifyRestartRequired();
  }
}

void AndroidStreamReaderURLRequestJob::DoStart() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (range_parse_result_ != net::OK) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           range_parse_result_));
    return;
  }

  // This could be done in the InputStreamReader but would force more
  // complex synchronization in the delegate.
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &OpenInputStreamOnWorkerThread, base::ThreadTaskRunnerHandle::Get(),
          // This is intentional - the job could be deleted while the callback
          // is executing on the background thread.
          // The delegate will be "returned" to the job once the InputStream
          // open attempt is completed.
          std::move(delegate_), request()->url(),
          base::BindOnce(&AndroidStreamReaderURLRequestJob::OnInputStreamOpened,
                         weak_factory_.GetWeakPtr())));
}

void AndroidStreamReaderURLRequestJob::HeadersComplete(
    int status_code,
    const std::string& status_text) {
  std::string status("HTTP/1.1 ");
  status.append(base::IntToString(status_code));
  status.append(" ");
  status.append(status_text);
  // HttpResponseHeaders expects its input string to be terminated by two NULs.
  status.append("\0\0", 2);
  net::HttpResponseHeaders* headers = new net::HttpResponseHeaders(status);

  if (status_code == kHTTPOk) {
    if (expected_content_size() != -1) {
      std::string content_length_header(
          net::HttpRequestHeaders::kContentLength);
      content_length_header.append(": ");
      content_length_header.append(
          base::Int64ToString(expected_content_size()));
      headers->AddHeader(content_length_header);
    }

    std::string mime_type;
    if (GetMimeType(&mime_type) && !mime_type.empty()) {
      std::string content_type_header(net::HttpRequestHeaders::kContentType);
      content_type_header.append(": ");
      content_type_header.append(mime_type);
      headers->AddHeader(content_type_header);
    }
  }

  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  delegate_->AppendResponseHeaders(env, headers);

  // Indicate that the response had been obtained via shouldInterceptRequest.
  headers->AddHeader(kResponseHeaderViaShouldInterceptRequest);

  response_info_.reset(new net::HttpResponseInfo());
  response_info_->headers = headers;

  NotifyHeadersComplete();
}

void AndroidStreamReaderURLRequestJob::GetResponseInfo(
    net::HttpResponseInfo* info) {
  if (response_info_)
    *info = *response_info_;
}

void AndroidStreamReaderURLRequestJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  std::string range_header;
  if (headers.GetHeader(net::HttpRequestHeaders::kRange, &range_header)) {
    // This job only cares about the Range header so that we know how many bytes
    // in the stream to skip and how many to read after that. Note that
    // validation is deferred to DoStart(), because NotifyStartError() is not
    // legal to call since the job has not started.
    std::vector<net::HttpByteRange> ranges;
    if (net::HttpUtil::ParseRangeHeader(range_header, &ranges)) {
      if (ranges.size() == 1)
        byte_range_ = ranges[0];
    } else {
      // We don't support multiple range requests in one single URL request,
      // because we need to do multipart encoding here.
      range_parse_result_ = net::ERR_REQUEST_RANGE_NOT_SATISFIABLE;
    }
  }
}

}  // namespace android_webview
