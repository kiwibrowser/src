// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/drive/base_requests.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/request_sender.h"
#include "google_apis/drive/request_util.h"
#include "google_apis/drive/task_util.h"
#include "google_apis/drive/time_util.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

using net::URLFetcher;

namespace {

// Template for optional OAuth2 authorization HTTP header.
const char kAuthorizationHeaderFormat[] = "Authorization: Bearer %s";
// Template for GData API version HTTP header.
const char kGDataVersionHeader[] = "GData-Version: 3.0";

// Maximum number of attempts for re-authentication per request.
const int kMaxReAuthenticateAttemptsPerRequest = 1;

// Template for initiate upload of both GData WAPI and Drive API v2.
const char kUploadContentType[] = "X-Upload-Content-Type: ";
const char kUploadContentLength[] = "X-Upload-Content-Length: ";
const char kUploadResponseLocation[] = "location";

// Template for upload data range of both GData WAPI and Drive API v2.
const char kUploadContentRange[] = "Content-Range: bytes ";
const char kUploadResponseRange[] = "range";

// Mime type of JSON.
const char kJsonMimeType[] = "application/json";

// Mime type of multipart related.
const char kMultipartRelatedMimeTypePrefix[] =
    "multipart/related; boundary=";

// Mime type of multipart mixed.
const char kMultipartMixedMimeTypePrefix[] =
    "multipart/mixed; boundary=";

// Header for each item in a multipart message.
const char kMultipartItemHeaderFormat[] = "--%s\nContent-Type: %s\n\n";

// Footer for whole multipart message.
const char kMultipartFooterFormat[] = "--%s--";

// Parses JSON passed in |json| on |blocking_task_runner|. Runs |callback| on
// the calling thread when finished with either success or failure.
// The callback must not be null.
void ParseJsonOnBlockingPool(
    base::TaskRunner* blocking_task_runner,
    const std::string& json,
    const base::Callback<void(std::unique_ptr<base::Value> value)>& callback) {
  base::PostTaskAndReplyWithResult(
      blocking_task_runner,
      FROM_HERE,
      base::Bind(&google_apis::ParseJson, json),
      callback);
}

// Returns response headers as a string. Returns a warning message if
// |url_fetcher| does not contain a valid response. Used only for debugging.
std::string GetResponseHeadersAsString(const URLFetcher* url_fetcher) {
  std::string headers;
  // Check that response code indicates response headers are valid (i.e. not
  // malformed) before we retrieve the headers.
  if (url_fetcher->GetResponseCode() == URLFetcher::RESPONSE_CODE_INVALID) {
    headers.assign("Response headers are malformed!!");
  } else {
    headers = net::HttpUtil::ConvertHeadersBackToHTTPResponse(
        url_fetcher->GetResponseHeaders()->raw_headers());
  }
  return headers;
}

// Obtains the multipart body for the metadata string and file contents. If
// predetermined_boundary is empty, the function generates the boundary string.
bool GetMultipartContent(const std::string& predetermined_boundary,
                         const std::string& metadata_json,
                         const std::string& content_type,
                         const base::FilePath& path,
                         std::string* upload_content_type,
                         std::string* upload_content_data) {
  std::vector<google_apis::ContentTypeAndData> parts(2);
  parts[0].type = kJsonMimeType;
  parts[0].data = metadata_json;
  parts[1].type = content_type;
  if (!ReadFileToString(path, &parts[1].data))
    return false;

  google_apis::ContentTypeAndData output;
  GenerateMultipartBody(google_apis::MULTIPART_RELATED, predetermined_boundary,
                        parts, &output, nullptr);
  upload_content_type->swap(output.type);
  upload_content_data->swap(output.data);
  return true;
}

// Parses JSON body and returns corresponding DriveApiErrorCode if it is found.
// The server may return detailed error status in JSON.
// See https://developers.google.com/drive/handle-errors
google_apis::DriveApiErrorCode MapJsonError(
    google_apis::DriveApiErrorCode code,
    const std::string& error_body) {
  if (IsSuccessfulDriveApiErrorCode(code))
    return code;

  DVLOG(1) << error_body;
  const char kErrorKey[] = "error";
  const char kErrorErrorsKey[] = "errors";
  const char kErrorReasonKey[] = "reason";
  const char kErrorMessageKey[] = "message";
  const char kErrorReasonRateLimitExceeded[] = "rateLimitExceeded";
  const char kErrorReasonUserRateLimitExceeded[] = "userRateLimitExceeded";
  const char kErrorReasonQuotaExceeded[] = "quotaExceeded";
  const char kErrorReasonResponseTooLarge[] = "responseTooLarge";

  std::unique_ptr<const base::Value> value(google_apis::ParseJson(error_body));
  const base::DictionaryValue* dictionary = NULL;
  const base::DictionaryValue* error = NULL;
  if (value &&
      value->GetAsDictionary(&dictionary) &&
      dictionary->GetDictionaryWithoutPathExpansion(kErrorKey, &error)) {
    // Get error message.
    std::string message;
    error->GetStringWithoutPathExpansion(kErrorMessageKey, &message);
    DLOG(ERROR) << "code: " << code << ", message: " << message;

    // Override the error code based on the reason of the first error.
    const base::ListValue* errors = NULL;
    const base::DictionaryValue* first_error = NULL;
    if (error->GetListWithoutPathExpansion(kErrorErrorsKey, &errors) &&
        errors->GetDictionary(0, &first_error)) {
      std::string reason;
      first_error->GetStringWithoutPathExpansion(kErrorReasonKey, &reason);
      if (reason == kErrorReasonRateLimitExceeded ||
          reason == kErrorReasonUserRateLimitExceeded) {
        return google_apis::HTTP_SERVICE_UNAVAILABLE;
      }
      if (reason == kErrorReasonQuotaExceeded)
        return google_apis::DRIVE_NO_SPACE;
      if (reason == kErrorReasonResponseTooLarge)
        return google_apis::DRIVE_RESPONSE_TOO_LARGE;
    }
  }

  return code;
}

}  // namespace

namespace google_apis {

std::unique_ptr<base::Value> ParseJson(const std::string& json) {
  int error_code = -1;
  std::string error_message;
  std::unique_ptr<base::Value> value = base::JSONReader::ReadAndReturnError(
      json, base::JSON_PARSE_RFC, &error_code, &error_message);

  if (!value.get()) {
    std::string trimmed_json;
    if (json.size() < 80) {
      trimmed_json  = json;
    } else {
      // Take the first 50 and the last 10 bytes.
      trimmed_json =
          base::StringPrintf("%s [%s bytes] %s", json.substr(0, 50).c_str(),
                             base::NumberToString(json.size() - 60).c_str(),
                             json.substr(json.size() - 10).c_str());
    }
    LOG(WARNING) << "Error while parsing entry response: " << error_message
                 << ", code: " << error_code << ", json:\n" << trimmed_json;
  }
  return value;
}

void GenerateMultipartBody(MultipartType multipart_type,
                           const std::string& predetermined_boundary,
                           const std::vector<ContentTypeAndData>& parts,
                           ContentTypeAndData* output,
                           std::vector<uint64_t>* data_offset) {
  std::string boundary;
  // Generate random boundary.
  if (predetermined_boundary.empty()) {
    while (true) {
      boundary = net::GenerateMimeMultipartBoundary();
      bool conflict_with_content = false;
      for (auto& part : parts) {
        if (part.data.find(boundary, 0) != std::string::npos) {
          conflict_with_content = true;
          break;
        }
      }
      if (!conflict_with_content)
        break;
    }
  } else {
    boundary = predetermined_boundary;
  }

  switch (multipart_type) {
    case MULTIPART_RELATED:
      output->type = kMultipartRelatedMimeTypePrefix + boundary;
      break;
    case MULTIPART_MIXED:
      output->type = kMultipartMixedMimeTypePrefix + boundary;
      break;
  }

  output->data.clear();
  if (data_offset)
    data_offset->clear();
  for (auto& part : parts) {
    output->data.append(base::StringPrintf(
        kMultipartItemHeaderFormat, boundary.c_str(), part.type.c_str()));
    if (data_offset)
      data_offset->push_back(output->data.size());
    output->data.append(part.data);
    output->data.append("\n");
  }
  output->data.append(
      base::StringPrintf(kMultipartFooterFormat, boundary.c_str()));
}

//=========================== ResponseWriter ==================================
ResponseWriter::ResponseWriter(base::SequencedTaskRunner* file_task_runner,
                               const base::FilePath& file_path,
                               const GetContentCallback& get_content_callback)
    : get_content_callback_(get_content_callback),
      weak_ptr_factory_(this) {
  if (!file_path.empty()) {
    file_writer_.reset(
        new net::URLFetcherFileWriter(file_task_runner, file_path));
  }
}

ResponseWriter::~ResponseWriter() {
}

void ResponseWriter::DisownFile() {
  DCHECK(file_writer_);
  file_writer_->DisownFile();
}

int ResponseWriter::Initialize(const net::CompletionCallback& callback) {
  if (file_writer_)
    return file_writer_->Initialize(callback);

  data_.clear();
  return net::OK;
}

int ResponseWriter::Write(net::IOBuffer* buffer,
                          int num_bytes,
                          const net::CompletionCallback& callback) {
  if (!get_content_callback_.is_null()) {
    get_content_callback_.Run(
        HTTP_SUCCESS, std::make_unique<std::string>(buffer->data(), num_bytes));
  }

  if (file_writer_) {
    const int result = file_writer_->Write(
        buffer, num_bytes,
        base::Bind(&ResponseWriter::DidWrite, weak_ptr_factory_.GetWeakPtr(),
                   base::WrapRefCounted(buffer), callback));
    if (result != net::ERR_IO_PENDING)
      DidWrite(buffer, net::CompletionCallback(), result);
    return result;
  }

  data_.append(buffer->data(), num_bytes);
  return num_bytes;
}

int ResponseWriter::Finish(int net_error,
                           const net::CompletionCallback& callback) {
  if (file_writer_)
    return file_writer_->Finish(net_error, callback);

  return net::OK;
}

void ResponseWriter::DidWrite(scoped_refptr<net::IOBuffer> buffer,
                              const net::CompletionCallback& callback,
                              int result) {
  if (result > 0) {
    // Even if file_writer_ is used, append the data to |data_|, so that it can
    // be used to get error information in case of server side errors.
    // The size limit is to avoid consuming too much redundant memory.
    const size_t kMaxStringSize = 1024*1024;
    if (data_.size() < kMaxStringSize) {
      data_.append(buffer->data(), std::min(static_cast<size_t>(result),
                                            kMaxStringSize - data_.size()));
    }
  }

  if (!callback.is_null())
    callback.Run(result);
}

//============================ UrlFetchRequestBase ===========================

UrlFetchRequestBase::UrlFetchRequestBase(RequestSender* sender)
    : re_authenticate_count_(0),
      response_writer_(NULL),
      sender_(sender),
      error_code_(DRIVE_OTHER_ERROR),
      weak_ptr_factory_(this) {
}

UrlFetchRequestBase::~UrlFetchRequestBase() {}

void UrlFetchRequestBase::Start(const std::string& access_token,
                                const std::string& custom_user_agent,
                                const ReAuthenticateCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!access_token.empty());
  DCHECK(!callback.is_null());
  DCHECK(re_authenticate_callback_.is_null());
  Prepare(base::Bind(&UrlFetchRequestBase::StartAfterPrepare,
                     weak_ptr_factory_.GetWeakPtr(), access_token,
                     custom_user_agent, callback));
}

void UrlFetchRequestBase::Prepare(const PrepareCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!callback.is_null());
  callback.Run(HTTP_SUCCESS);
}

void UrlFetchRequestBase::StartAfterPrepare(
    const std::string& access_token,
    const std::string& custom_user_agent,
    const ReAuthenticateCallback& callback,
    DriveApiErrorCode code) {
  DCHECK(CalledOnValidThread());
  DCHECK(!access_token.empty());
  DCHECK(!callback.is_null());
  DCHECK(re_authenticate_callback_.is_null());

  const GURL url = GetURL();
  DriveApiErrorCode error_code;
  if (IsSuccessfulDriveApiErrorCode(code))
    error_code = code;
  else if (url.is_empty())
    error_code = DRIVE_OTHER_ERROR;
  else
    error_code = HTTP_SUCCESS;

  if (error_code != HTTP_SUCCESS) {
    // Error is found on generating the url or preparing the request. Send the
    // error message to the callback, and then return immediately without trying
    // to connect to the server.  We need to call CompleteRequestWithError
    // asynchronously because client code does not assume result callback is
    // called synchronously.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&UrlFetchRequestBase::CompleteRequestWithError,
                              weak_ptr_factory_.GetWeakPtr(), error_code));
    return;
  }

  re_authenticate_callback_ = callback;
  DVLOG(1) << "URL: " << url.spec();

  URLFetcher::RequestType request_type = GetRequestType();
  url_fetcher_ = URLFetcher::Create(url, request_type, this,
                                    sender_->get_traffic_annotation_tag());
  url_fetcher_->SetRequestContext(sender_->url_request_context_getter());
  // Always set flags to neither send nor save cookies.
  url_fetcher_->SetLoadFlags(
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_DISABLE_CACHE);

  base::FilePath output_file_path;
  GetContentCallback get_content_callback;
  GetOutputFilePath(&output_file_path, &get_content_callback);
  if (!get_content_callback.is_null())
    get_content_callback = CreateRelayCallback(get_content_callback);
  response_writer_ = new ResponseWriter(blocking_task_runner(),
                                        output_file_path,
                                        get_content_callback);
  url_fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherResponseWriter>(response_writer_));

  // Add request headers.
  // Note that SetExtraRequestHeaders clears the current headers and sets it
  // to the passed-in headers, so calling it for each header will result in
  // only the last header being set in request headers.
  if (!custom_user_agent.empty())
    url_fetcher_->AddExtraRequestHeader("User-Agent: " + custom_user_agent);
  url_fetcher_->AddExtraRequestHeader(kGDataVersionHeader);
  url_fetcher_->AddExtraRequestHeader(
      base::StringPrintf(kAuthorizationHeaderFormat, access_token.data()));
  std::vector<std::string> headers = GetExtraRequestHeaders();
  for (size_t i = 0; i < headers.size(); ++i) {
    url_fetcher_->AddExtraRequestHeader(headers[i]);
    DVLOG(1) << "Extra header: " << headers[i];
  }

  // Set upload data if available.
  std::string upload_content_type;
  std::string upload_content;
  if (GetContentData(&upload_content_type, &upload_content)) {
    url_fetcher_->SetUploadData(upload_content_type, upload_content);
  } else {
    base::FilePath local_file_path;
    int64_t range_offset = 0;
    int64_t range_length = 0;
    if (GetContentFile(&local_file_path, &range_offset, &range_length,
                       &upload_content_type)) {
      url_fetcher_->SetUploadFilePath(
          upload_content_type,
          local_file_path,
          range_offset,
          range_length,
          blocking_task_runner());
    } else {
      // Even if there is no content data, UrlFetcher requires to set empty
      // upload data string for POST, PUT and PATCH methods, explicitly.
      // It is because that most requests of those methods have non-empty
      // body, and UrlFetcher checks whether it is actually not forgotten.
      if (request_type == URLFetcher::POST ||
          request_type == URLFetcher::PUT ||
          request_type == URLFetcher::PATCH) {
        // Set empty upload content-type and upload content, so that
        // the request will have no "Content-type: " header and no content.
        url_fetcher_->SetUploadData(std::string(), std::string());
      }
    }
  }

  url_fetcher_->Start();
}

URLFetcher::RequestType UrlFetchRequestBase::GetRequestType() const {
  return URLFetcher::GET;
}

std::vector<std::string> UrlFetchRequestBase::GetExtraRequestHeaders() const {
  return std::vector<std::string>();
}

bool UrlFetchRequestBase::GetContentData(std::string* upload_content_type,
                                         std::string* upload_content) {
  return false;
}

bool UrlFetchRequestBase::GetContentFile(base::FilePath* local_file_path,
                                         int64_t* range_offset,
                                         int64_t* range_length,
                                         std::string* upload_content_type) {
  return false;
}

void UrlFetchRequestBase::GetOutputFilePath(
    base::FilePath* local_file_path,
    GetContentCallback* get_content_callback) {
}

void UrlFetchRequestBase::Cancel() {
  response_writer_ = NULL;
  url_fetcher_.reset(NULL);
  CompleteRequestWithError(DRIVE_CANCELLED);
}

DriveApiErrorCode UrlFetchRequestBase::GetErrorCode() {
  return error_code_;
}

bool UrlFetchRequestBase::CalledOnValidThread() {
  return thread_checker_.CalledOnValidThread();
}

base::SequencedTaskRunner* UrlFetchRequestBase::blocking_task_runner() const {
  return sender_->blocking_task_runner();
}

void UrlFetchRequestBase::OnProcessURLFetchResultsComplete() {
  sender_->RequestFinished(this);
}

void UrlFetchRequestBase::OnURLFetchComplete(const URLFetcher* source) {
  DVLOG(1) << "Response headers:\n" << GetResponseHeadersAsString(source);

  // Determine error code.
  error_code_ = static_cast<DriveApiErrorCode>(source->GetResponseCode());
  if (!source->GetStatus().is_success()) {
    switch (source->GetStatus().error()) {
      case net::ERR_NETWORK_CHANGED:
        error_code_ = DRIVE_NO_CONNECTION;
        break;
      default:
        error_code_ = DRIVE_OTHER_ERROR;
    }
  }

  error_code_ = MapJsonError(error_code_, response_writer_->data());

  // Handle authentication failure.
  if (error_code_ == HTTP_UNAUTHORIZED) {
    if (++re_authenticate_count_ <= kMaxReAuthenticateAttemptsPerRequest) {
      // Reset re_authenticate_callback_ so Start() can be called again.
      ReAuthenticateCallback callback = re_authenticate_callback_;
      re_authenticate_callback_.Reset();
      callback.Run(this);
      return;
    }

    OnAuthFailed(error_code_);
    return;
  }

  // Overridden by each specialization
  ProcessURLFetchResults(source);
}

void UrlFetchRequestBase::CompleteRequestWithError(DriveApiErrorCode code) {
  RunCallbackOnPrematureFailure(code);
  sender_->RequestFinished(this);
}

void UrlFetchRequestBase::OnAuthFailed(DriveApiErrorCode code) {
  CompleteRequestWithError(code);
}

base::WeakPtr<AuthenticatedRequestInterface>
UrlFetchRequestBase::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

//============================ EntryActionRequest ============================

EntryActionRequest::EntryActionRequest(RequestSender* sender,
                                       const EntryActionCallback& callback)
    : UrlFetchRequestBase(sender),
      callback_(callback) {
  DCHECK(!callback_.is_null());
}

EntryActionRequest::~EntryActionRequest() {}

void EntryActionRequest::ProcessURLFetchResults(const URLFetcher* source) {
  callback_.Run(GetErrorCode());
  OnProcessURLFetchResultsComplete();
}

void EntryActionRequest::RunCallbackOnPrematureFailure(DriveApiErrorCode code) {
  callback_.Run(code);
}

//========================= InitiateUploadRequestBase ========================

InitiateUploadRequestBase::InitiateUploadRequestBase(
    RequestSender* sender,
    const InitiateUploadCallback& callback,
    const std::string& content_type,
    int64_t content_length)
    : UrlFetchRequestBase(sender),
      callback_(callback),
      content_type_(content_type),
      content_length_(content_length) {
  DCHECK(!callback_.is_null());
  DCHECK(!content_type_.empty());
  DCHECK_GE(content_length_, 0);
}

InitiateUploadRequestBase::~InitiateUploadRequestBase() {}

void InitiateUploadRequestBase::ProcessURLFetchResults(
    const URLFetcher* source) {
  DriveApiErrorCode code = GetErrorCode();

  std::string upload_location;
  if (code == HTTP_SUCCESS) {
    // Retrieve value of the first "Location" header.
    source->GetResponseHeaders()->EnumerateHeader(NULL,
                                                  kUploadResponseLocation,
                                                  &upload_location);
  }

  callback_.Run(code, GURL(upload_location));
  OnProcessURLFetchResultsComplete();
}

void InitiateUploadRequestBase::RunCallbackOnPrematureFailure(
    DriveApiErrorCode code) {
  callback_.Run(code, GURL());
}

std::vector<std::string>
InitiateUploadRequestBase::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(kUploadContentType + content_type_);
  headers.push_back(
      kUploadContentLength + base::Int64ToString(content_length_));
  return headers;
}

//============================ UploadRangeResponse =============================

UploadRangeResponse::UploadRangeResponse()
    : code(HTTP_SUCCESS),
      start_position_received(0),
      end_position_received(0) {
}

UploadRangeResponse::UploadRangeResponse(DriveApiErrorCode code,
                                         int64_t start_position_received,
                                         int64_t end_position_received)
    : code(code),
      start_position_received(start_position_received),
      end_position_received(end_position_received) {}

UploadRangeResponse::~UploadRangeResponse() {
}

//========================== UploadRangeRequestBase ==========================

UploadRangeRequestBase::UploadRangeRequestBase(RequestSender* sender,
                                               const GURL& upload_url)
    : UrlFetchRequestBase(sender),
      upload_url_(upload_url),
      weak_ptr_factory_(this) {
}

UploadRangeRequestBase::~UploadRangeRequestBase() {}

GURL UploadRangeRequestBase::GetURL() const {
  // This is very tricky to get json from this request. To do that, &alt=json
  // has to be appended not here but in InitiateUploadRequestBase::GetURL().
  return upload_url_;
}

URLFetcher::RequestType UploadRangeRequestBase::GetRequestType() const {
  return URLFetcher::PUT;
}

void UploadRangeRequestBase::ProcessURLFetchResults(
    const URLFetcher* source) {
  DriveApiErrorCode code = GetErrorCode();
  net::HttpResponseHeaders* hdrs = source->GetResponseHeaders();

  if (code == HTTP_RESUME_INCOMPLETE) {
    // Retrieve value of the first "Range" header.
    // The Range header is appeared only if there is at least one received
    // byte. So, initialize the positions by 0 so that the [0,0) will be
    // returned via the |callback_| for empty data case.
    int64_t start_position_received = 0;
    int64_t end_position_received = 0;
    std::string range_received;
    hdrs->EnumerateHeader(NULL, kUploadResponseRange, &range_received);
    if (!range_received.empty()) {  // Parse the range header.
      std::vector<net::HttpByteRange> ranges;
      if (net::HttpUtil::ParseRangeHeader(range_received, &ranges) &&
          !ranges.empty() ) {
        // We only care about the first start-end pair in the range.
        //
        // Range header represents the range inclusively, while we are treating
        // ranges exclusively (i.e., end_position_received should be one passed
        // the last valid index). So "+ 1" is added.
        start_position_received = ranges[0].first_byte_position();
        end_position_received = ranges[0].last_byte_position() + 1;
      }
    }
    // The Range header has the received data range, so the start position
    // should be always 0.
    DCHECK_EQ(start_position_received, 0);

    OnRangeRequestComplete(UploadRangeResponse(code, start_position_received,
                                               end_position_received),
                           std::unique_ptr<base::Value>());

    OnProcessURLFetchResultsComplete();
  } else if (code == HTTP_CREATED || code == HTTP_SUCCESS) {
    // The upload is successfully done. Parse the response which should be
    // the entry's metadata.
    ParseJsonOnBlockingPool(blocking_task_runner(),
                            response_writer()->data(),
                            base::Bind(&UploadRangeRequestBase::OnDataParsed,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       code));
  } else {
    // Failed to upload. Run callbacks to notify the error.
    OnRangeRequestComplete(UploadRangeResponse(code, -1, -1),
                           std::unique_ptr<base::Value>());
    OnProcessURLFetchResultsComplete();
  }
}

void UploadRangeRequestBase::OnDataParsed(DriveApiErrorCode code,
                                          std::unique_ptr<base::Value> value) {
  DCHECK(CalledOnValidThread());
  DCHECK(code == HTTP_CREATED || code == HTTP_SUCCESS);

  OnRangeRequestComplete(UploadRangeResponse(code, -1, -1), std::move(value));
  OnProcessURLFetchResultsComplete();
}

void UploadRangeRequestBase::RunCallbackOnPrematureFailure(
    DriveApiErrorCode code) {
  OnRangeRequestComplete(UploadRangeResponse(code, 0, 0),
                         std::unique_ptr<base::Value>());
}

//========================== ResumeUploadRequestBase =========================

ResumeUploadRequestBase::ResumeUploadRequestBase(
    RequestSender* sender,
    const GURL& upload_location,
    int64_t start_position,
    int64_t end_position,
    int64_t content_length,
    const std::string& content_type,
    const base::FilePath& local_file_path)
    : UploadRangeRequestBase(sender, upload_location),
      start_position_(start_position),
      end_position_(end_position),
      content_length_(content_length),
      content_type_(content_type),
      local_file_path_(local_file_path) {
  DCHECK_LE(start_position_, end_position_);
}

ResumeUploadRequestBase::~ResumeUploadRequestBase() {}

std::vector<std::string>
ResumeUploadRequestBase::GetExtraRequestHeaders() const {
  if (content_length_ == 0) {
    // For uploading an empty document, just PUT an empty content.
    DCHECK_EQ(start_position_, 0);
    DCHECK_EQ(end_position_, 0);
    return std::vector<std::string>();
  }

  // The header looks like
  // Content-Range: bytes <start_position>-<end_position>/<content_length>
  // for example:
  // Content-Range: bytes 7864320-8388607/13851821
  // The header takes inclusive range, so we adjust by "end_position - 1".
  DCHECK_GE(start_position_, 0);
  DCHECK_GT(end_position_, 0);
  DCHECK_GE(content_length_, 0);

  std::vector<std::string> headers;
  headers.push_back(
      std::string(kUploadContentRange) +
      base::Int64ToString(start_position_) + "-" +
      base::Int64ToString(end_position_ - 1) + "/" +
      base::Int64ToString(content_length_));
  return headers;
}

bool ResumeUploadRequestBase::GetContentFile(base::FilePath* local_file_path,
                                             int64_t* range_offset,
                                             int64_t* range_length,
                                             std::string* upload_content_type) {
  if (start_position_ == end_position_) {
    // No content data.
    return false;
  }

  *local_file_path = local_file_path_;
  *range_offset = start_position_;
  *range_length = end_position_ - start_position_;
  *upload_content_type = content_type_;
  return true;
}

//======================== GetUploadStatusRequestBase ========================

GetUploadStatusRequestBase::GetUploadStatusRequestBase(RequestSender* sender,
                                                       const GURL& upload_url,
                                                       int64_t content_length)
    : UploadRangeRequestBase(sender, upload_url),
      content_length_(content_length) {}

GetUploadStatusRequestBase::~GetUploadStatusRequestBase() {}

std::vector<std::string>
GetUploadStatusRequestBase::GetExtraRequestHeaders() const {
  // The header looks like
  // Content-Range: bytes */<content_length>
  // for example:
  // Content-Range: bytes */13851821
  DCHECK_GE(content_length_, 0);

  std::vector<std::string> headers;
  headers.push_back(
      std::string(kUploadContentRange) + "*/" +
      base::Int64ToString(content_length_));
  return headers;
}

//========================= MultipartUploadRequestBase ========================

MultipartUploadRequestBase::MultipartUploadRequestBase(
    base::SequencedTaskRunner* blocking_task_runner,
    const std::string& metadata_json,
    const std::string& content_type,
    int64_t content_length,
    const base::FilePath& local_file_path,
    const FileResourceCallback& callback,
    const ProgressCallback& progress_callback)
    : blocking_task_runner_(blocking_task_runner),
      metadata_json_(metadata_json),
      content_type_(content_type),
      local_path_(local_file_path),
      callback_(callback),
      progress_callback_(progress_callback),
      weak_ptr_factory_(this) {
  DCHECK(!content_type.empty());
  DCHECK_GE(content_length, 0);
  DCHECK(!local_file_path.empty());
  DCHECK(!callback.is_null());
}

MultipartUploadRequestBase::~MultipartUploadRequestBase() {
}

std::vector<std::string> MultipartUploadRequestBase::GetExtraRequestHeaders()
    const {
  return std::vector<std::string>();
}

void MultipartUploadRequestBase::Prepare(const PrepareCallback& callback) {
  // If the request is cancelled, the request instance will be deleted in
  // |UrlFetchRequestBase::Cancel| and OnPrepareUploadContent won't be called.
  std::string* const upload_content_type = new std::string();
  std::string* const upload_content_data = new std::string();
  PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&GetMultipartContent, boundary_, metadata_json_, content_type_,
                 local_path_, base::Unretained(upload_content_type),
                 base::Unretained(upload_content_data)),
      base::Bind(&MultipartUploadRequestBase::OnPrepareUploadContent,
                 weak_ptr_factory_.GetWeakPtr(), callback,
                 base::Owned(upload_content_type),
                 base::Owned(upload_content_data)));
}

void MultipartUploadRequestBase::OnPrepareUploadContent(
    const PrepareCallback& callback,
    std::string* upload_content_type,
    std::string* upload_content_data,
    bool result) {
  if (!result) {
    callback.Run(DRIVE_FILE_ERROR);
    return;
  }
  upload_content_type_.swap(*upload_content_type);
  upload_content_data_.swap(*upload_content_data);
  callback.Run(HTTP_SUCCESS);
}

void MultipartUploadRequestBase::SetBoundaryForTesting(
    const std::string& boundary) {
  boundary_ = boundary;
}

bool MultipartUploadRequestBase::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content_data) {
  // TODO(hirono): Pass stream instead of actual data to reduce memory usage.
  upload_content_type->swap(upload_content_type_);
  upload_content_data->swap(upload_content_data_);
  return true;
}

void MultipartUploadRequestBase::NotifyResult(
    DriveApiErrorCode code,
    const std::string& body,
    const base::Closure& notify_complete_callback) {
  // The upload is successfully done. Parse the response which should be
  // the entry's metadata.
  if (code == HTTP_CREATED || code == HTTP_SUCCESS) {
    ParseJsonOnBlockingPool(
        blocking_task_runner_.get(), body,
        base::Bind(&MultipartUploadRequestBase::OnDataParsed,
                   weak_ptr_factory_.GetWeakPtr(), code,
                   notify_complete_callback));
  } else {
    NotifyError(MapJsonError(code, body));
    notify_complete_callback.Run();
  }
}

void MultipartUploadRequestBase::NotifyError(DriveApiErrorCode code) {
  callback_.Run(code, std::unique_ptr<FileResource>());
}

void MultipartUploadRequestBase::NotifyUploadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total) {
  if (!progress_callback_.is_null())
    progress_callback_.Run(current, total);
}

void MultipartUploadRequestBase::OnDataParsed(
    DriveApiErrorCode code,
    const base::Closure& notify_complete_callback,
    std::unique_ptr<base::Value> value) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (value)
    callback_.Run(code, google_apis::FileResource::CreateFrom(*value));
  else
    NotifyError(DRIVE_PARSE_ERROR);
  notify_complete_callback.Run();
}

//============================ DownloadFileRequestBase =========================

DownloadFileRequestBase::DownloadFileRequestBase(
    RequestSender* sender,
    const DownloadActionCallback& download_action_callback,
    const GetContentCallback& get_content_callback,
    const ProgressCallback& progress_callback,
    const GURL& download_url,
    const base::FilePath& output_file_path)
    : UrlFetchRequestBase(sender),
      download_action_callback_(download_action_callback),
      get_content_callback_(get_content_callback),
      progress_callback_(progress_callback),
      download_url_(download_url),
      output_file_path_(output_file_path) {
  DCHECK(!download_action_callback_.is_null());
  DCHECK(!output_file_path_.empty());
  // get_content_callback may be null.
}

DownloadFileRequestBase::~DownloadFileRequestBase() {}

// Overridden from UrlFetchRequestBase.
GURL DownloadFileRequestBase::GetURL() const {
  return download_url_;
}

void DownloadFileRequestBase::GetOutputFilePath(
    base::FilePath* local_file_path,
    GetContentCallback* get_content_callback) {
  // Configure so that the downloaded content is saved to |output_file_path_|.
  *local_file_path = output_file_path_;
  *get_content_callback = get_content_callback_;
}

void DownloadFileRequestBase::OnURLFetchDownloadProgress(
    const URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  if (!progress_callback_.is_null())
    progress_callback_.Run(current, total);
}

void DownloadFileRequestBase::ProcessURLFetchResults(const URLFetcher* source) {
  DriveApiErrorCode code = GetErrorCode();

  // Take over the ownership of the the downloaded temp file.
  base::FilePath temp_file;
  if (code == HTTP_SUCCESS) {
    response_writer()->DisownFile();
    temp_file = output_file_path_;
  }

  download_action_callback_.Run(code, temp_file);
  OnProcessURLFetchResultsComplete();
}

void DownloadFileRequestBase::RunCallbackOnPrematureFailure(
    DriveApiErrorCode code) {
  download_action_callback_.Run(code, base::FilePath());
}

}  // namespace google_apis
