// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/device_management_service.h"

#include <utility>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace em = enterprise_management;

namespace policy {

namespace {

const char kPostContentType[] = "application/protobuf";

const char kServiceTokenAuthHeader[] = "Authorization: GoogleLogin auth=";
const char kDMTokenAuthHeader[] = "Authorization: GoogleDMToken token=";
const char kEnrollmentTokenAuthHeader[] =
    "Authorization: GoogleEnrollmentToken token=";

// Number of times to retry on ERR_NETWORK_CHANGED errors.
const int kMaxRetries = 3;

// HTTP Error Codes of the DM Server with their concrete meanings in the context
// of the DM Server communication.
const int kSuccess = 200;
const int kInvalidArgument = 400;
const int kInvalidAuthCookieOrDMToken = 401;
const int kMissingLicenses = 402;
const int kDeviceManagementNotAllowed = 403;
const int kInvalidURL = 404;  // This error is not coming from the GFE.
const int kInvalidSerialNumber = 405;
const int kDomainMismatch = 406;
const int kDeviceIdConflict = 409;
const int kDeviceNotFound = 410;
const int kPendingApproval = 412;
const int kInternalServerError = 500;
const int kServiceUnavailable = 503;
const int kPolicyNotFound = 902;
const int kDeprovisioned = 903;
const int kArcDisabled = 904;

// Delay after first unsuccessful upload attempt. After each additional failure,
// the delay increases exponentially. Can be changed for testing to prevent
// timeouts.
long g_retry_delay_ms = 10000;

bool IsProxyError(const net::URLRequestStatus status) {
  switch (status.error()) {
    case net::ERR_PROXY_CONNECTION_FAILED:
    case net::ERR_TUNNEL_CONNECTION_FAILED:
    case net::ERR_PROXY_AUTH_UNSUPPORTED:
    case net::ERR_HTTPS_PROXY_TUNNEL_RESPONSE:
    case net::ERR_MANDATORY_PROXY_CONFIGURATION_FAILED:
    case net::ERR_PROXY_CERTIFICATE_INVALID:
    case net::ERR_SOCKS_CONNECTION_FAILED:
    case net::ERR_SOCKS_CONNECTION_HOST_UNREACHABLE:
      return true;
  }
  return false;
}

bool IsConnectionError(const net::URLRequestStatus status) {
  switch (status.error()) {
    case net::ERR_NETWORK_CHANGED:
    case net::ERR_NAME_NOT_RESOLVED:
    case net::ERR_INTERNET_DISCONNECTED:
    case net::ERR_ADDRESS_UNREACHABLE:
    case net::ERR_CONNECTION_TIMED_OUT:
    case net::ERR_NAME_RESOLUTION_FAILED:
      return true;
  }
  return false;
}

bool IsProtobufMimeType(const net::URLFetcher* fetcher) {
  return fetcher->GetResponseHeaders()->HasHeaderValue(
      "content-type", "application/x-protobuffer");
}

bool FailedWithProxy(const net::URLFetcher* fetcher) {
  if ((fetcher->GetLoadFlags() & net::LOAD_BYPASS_PROXY) != 0) {
    // The request didn't use a proxy.
    return false;
  }

  if (!fetcher->GetStatus().is_success() &&
      IsProxyError(fetcher->GetStatus())) {
    LOG(WARNING) << "Proxy failed while contacting dmserver.";
    return true;
  }

  if (fetcher->GetStatus().is_success() &&
      fetcher->GetResponseCode() == kSuccess &&
      fetcher->WasFetchedViaProxy() &&
      !IsProtobufMimeType(fetcher)) {
    // The proxy server can be misconfigured but pointing to an existing
    // server that replies to requests. Try to recover if a successful
    // request that went through a proxy returns an unexpected mime type.
    LOG(WARNING) << "Got bad mime-type in response from dmserver that was "
                 << "fetched via a proxy.";
    return true;
  }

  return false;
}

const char* JobTypeToRequestType(DeviceManagementRequestJob::JobType type) {
  switch (type) {
    case DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT:
      return dm_protocol::kValueRequestAutoEnrollment;
    case DeviceManagementRequestJob::TYPE_REGISTRATION:
      return dm_protocol::kValueRequestRegister;
    case DeviceManagementRequestJob::TYPE_POLICY_FETCH:
      return dm_protocol::kValueRequestPolicy;
    case DeviceManagementRequestJob::TYPE_API_AUTH_CODE_FETCH:
      return dm_protocol::kValueRequestApiAuthorization;
    case DeviceManagementRequestJob::TYPE_UNREGISTRATION:
      return dm_protocol::kValueRequestUnregister;
    case DeviceManagementRequestJob::TYPE_UPLOAD_CERTIFICATE:
      return dm_protocol::kValueRequestUploadCertificate;
    case DeviceManagementRequestJob::TYPE_DEVICE_STATE_RETRIEVAL:
      return dm_protocol::kValueRequestDeviceStateRetrieval;
    case DeviceManagementRequestJob::TYPE_UPLOAD_STATUS:
      return dm_protocol::kValueRequestUploadStatus;
    case DeviceManagementRequestJob::TYPE_REMOTE_COMMANDS:
      return dm_protocol::kValueRequestRemoteCommands;
    case DeviceManagementRequestJob::TYPE_ATTRIBUTE_UPDATE_PERMISSION:
      return dm_protocol::kValueRequestDeviceAttributeUpdatePermission;
    case DeviceManagementRequestJob::TYPE_ATTRIBUTE_UPDATE:
      return dm_protocol::kValueRequestDeviceAttributeUpdate;
    case DeviceManagementRequestJob::TYPE_GCM_ID_UPDATE:
      return dm_protocol::kValueRequestGcmIdUpdate;
    case DeviceManagementRequestJob::TYPE_ANDROID_MANAGEMENT_CHECK:
      return dm_protocol::kValueRequestCheckAndroidManagement;
    case DeviceManagementRequestJob::TYPE_CERT_BASED_REGISTRATION:
      return dm_protocol::kValueRequestCertBasedRegister;
    case DeviceManagementRequestJob::TYPE_ACTIVE_DIRECTORY_ENROLL_PLAY_USER:
      return dm_protocol::kValueRequestActiveDirectoryEnrollPlayUser;
    case DeviceManagementRequestJob::TYPE_ACTIVE_DIRECTORY_PLAY_ACTIVITY:
      return dm_protocol::kValueRequestActiveDirectoryPlayActivity;
    case DeviceManagementRequestJob::TYPE_REQUEST_LICENSE_TYPES:
      return dm_protocol::kValueRequestCheckDeviceLicense;
    case DeviceManagementRequestJob::TYPE_UPLOAD_APP_INSTALL_REPORT:
      return dm_protocol::kValueRequestAppInstallReport;
    case DeviceManagementRequestJob::TYPE_TOKEN_ENROLLMENT:
      return dm_protocol::kValueRequestTokenEnrollment;
    case DeviceManagementRequestJob::TYPE_CHROME_DESKTOP_REPORT:
      return dm_protocol::kValueRequestChromeDesktopReport;
    case DeviceManagementRequestJob::TYPE_INITIAL_ENROLLMENT_STATE_RETRIEVAL:
      return dm_protocol::kValueRequestInitialEnrollmentStateRetrieval;
  }
  NOTREACHED() << "Invalid job type " << type;
  return "";
}

}  // namespace

// Request job implementation used with DeviceManagementService.
class DeviceManagementRequestJobImpl : public DeviceManagementRequestJob {
 public:
  DeviceManagementRequestJobImpl(
      JobType type,
      const std::string& agent_parameter,
      const std::string& platform_parameter,
      DeviceManagementService* service,
      const scoped_refptr<net::URLRequestContextGetter>& request_context);
  ~DeviceManagementRequestJobImpl() override;

  // Handles the URL request response.
  void HandleResponse(const net::URLRequestStatus& status,
                      int response_code,
                      const std::string& data);

  // Gets the URL to contact.
  GURL GetURL(const std::string& server_url);

  // Configures the fetcher, setting up payload and headers.
  void ConfigureRequest(net::URLFetcher* fetcher);

  enum RetryMethod {
    // No retry required for this request.
    NO_RETRY,
    // Should retry immediately (no delay).
    RETRY_IMMEDIATELY,
    // Should retry after a delay.
    RETRY_WITH_DELAY
  };

  // Returns if and how this job should be retried. |fetcher| has just
  // completed, and can be inspected to determine if the request failed and
  // should be retried.
  RetryMethod ShouldRetry(const net::URLFetcher* fetcher);

  // Returns the delay before the next retry with the specified RetryMethod.
  int GetRetryDelay(RetryMethod method);

  // Invoked right before retrying this job.
  void PrepareRetry();

  // Get weak pointer
  base::WeakPtr<DeviceManagementRequestJobImpl> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 protected:
  // DeviceManagementRequestJob:
  void Run() override;

 private:
  // Invokes the callback with the given error code.
  void ReportError(DeviceManagementStatus code);

  // Pointer to the service this job is associated with.
  DeviceManagementService* service_;

  // Whether the BYPASS_PROXY flag should be set by ConfigureRequest().
  bool bypass_proxy_;

  // Number of times that this job has been retried due to connection errors.
  int retries_count_;

  // The last error why we had to retry.
  int last_error_ = 0;

  // The request context to use for this job.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // Used to get notified if the job has been canceled while waiting for retry.
  base::WeakPtrFactory<DeviceManagementRequestJobImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceManagementRequestJobImpl);
};

// Used in the Enterprise.DMServerRequestSuccess histogram, shows how many
// retries we had to do to execute the DeviceManagementRequestJob.
enum DMServerRequestSuccess {
  // No retries happened, the request succeeded for the first try.
  REQUEST_NO_RETRY = 0,

  // 1..kMaxRetries: number of retries

  // The request failed (too many retries or non-retriable error).
  REQUEST_FAILED = 10,
  // The server responded with an error.
  REQUEST_ERROR,

  REQUEST_MAX
};

DeviceManagementRequestJobImpl::DeviceManagementRequestJobImpl(
    JobType type,
    const std::string& agent_parameter,
    const std::string& platform_parameter,
    DeviceManagementService* service,
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
    : DeviceManagementRequestJob(type, agent_parameter, platform_parameter),
      service_(service),
      bypass_proxy_(false),
      retries_count_(0),
      request_context_(request_context),
      weak_ptr_factory_(this) {}

DeviceManagementRequestJobImpl::~DeviceManagementRequestJobImpl() {
  service_->RemoveJob(this);
}

void DeviceManagementRequestJobImpl::Run() {
  service_->AddJob(this);
}

void DeviceManagementRequestJobImpl::HandleResponse(
    const net::URLRequestStatus& status,
    int response_code,
    const std::string& data) {
  if (status.status() != net::URLRequestStatus::SUCCESS) {
    UMA_HISTOGRAM_ENUMERATION("Enterprise.DMServerRequestSuccess",
                              DMServerRequestSuccess::REQUEST_FAILED,
                              DMServerRequestSuccess::REQUEST_MAX);
    LOG(WARNING) << "DMServer request failed, status: " << status.status()
                 << ", error: " << status.error();
    em::DeviceManagementResponse dummy_response;
    callback_.Run(DM_STATUS_REQUEST_FAILED, status.error(), dummy_response);
    return;
  }

  if (response_code != kSuccess) {
    UMA_HISTOGRAM_ENUMERATION("Enterprise.DMServerRequestSuccess",
                              DMServerRequestSuccess::REQUEST_ERROR,
                              DMServerRequestSuccess::REQUEST_MAX);
    LOG(WARNING) << "DMServer sent an error response: " << response_code;
  } else {
    // Success with retries_count_ retries.
    UMA_HISTOGRAM_EXACT_LINEAR(
        "Enterprise.DMServerRequestSuccess", retries_count_,
        static_cast<int>(DMServerRequestSuccess::REQUEST_MAX));
  }

  switch (response_code) {
    case kSuccess: {
      em::DeviceManagementResponse response;
      if (!response.ParseFromString(data)) {
        ReportError(DM_STATUS_RESPONSE_DECODING_ERROR);
        return;
      }
      callback_.Run(DM_STATUS_SUCCESS, net::OK, response);
      return;
    }
    case kInvalidArgument:
      ReportError(DM_STATUS_REQUEST_INVALID);
      return;
    case kInvalidAuthCookieOrDMToken:
      ReportError(DM_STATUS_SERVICE_MANAGEMENT_TOKEN_INVALID);
      return;
    case kMissingLicenses:
      ReportError(DM_STATUS_SERVICE_MISSING_LICENSES);
      return;
    case kDeviceManagementNotAllowed:
      ReportError(DM_STATUS_SERVICE_MANAGEMENT_NOT_SUPPORTED);
      return;
    case kPendingApproval:
      ReportError(DM_STATUS_SERVICE_ACTIVATION_PENDING);
      return;
    case kInvalidURL:
    case kInternalServerError:
    case kServiceUnavailable:
      ReportError(DM_STATUS_TEMPORARY_UNAVAILABLE);
      return;
    case kDeviceNotFound:
      ReportError(DM_STATUS_SERVICE_DEVICE_NOT_FOUND);
      return;
    case kPolicyNotFound:
      ReportError(DM_STATUS_SERVICE_POLICY_NOT_FOUND);
      return;
    case kInvalidSerialNumber:
      ReportError(DM_STATUS_SERVICE_INVALID_SERIAL_NUMBER);
      return;
    case kDomainMismatch:
      ReportError(DM_STATUS_SERVICE_DOMAIN_MISMATCH);
      return;
    case kDeprovisioned:
      ReportError(DM_STATUS_SERVICE_DEPROVISIONED);
      return;
    case kDeviceIdConflict:
      ReportError(DM_STATUS_SERVICE_DEVICE_ID_CONFLICT);
      return;
    case kArcDisabled:
      ReportError(DM_STATUS_SERVICE_ARC_DISABLED);
      return;
    default:
      // Handle all unknown 5xx HTTP error codes as temporary and any other
      // unknown error as one that needs more time to recover.
      if (response_code >= 500 && response_code <= 599)
        ReportError(DM_STATUS_TEMPORARY_UNAVAILABLE);
      else
        ReportError(DM_STATUS_HTTP_STATUS_ERROR);
      return;
  }
}

GURL DeviceManagementRequestJobImpl::GetURL(
    const std::string& server_url) {
  std::string result(server_url);
  result += '?';
  ParameterMap current_query_params(query_params_);
  if (last_error_ == 0) {
    // Not a retry.
    current_query_params.push_back(
        std::make_pair(dm_protocol::kParamRetry, "false"));
  } else {
    current_query_params.push_back(
        std::make_pair(dm_protocol::kParamRetry, "true"));
    current_query_params.push_back(std::make_pair(dm_protocol::kParamLastError,
                                                  std::to_string(last_error_)));
  }
  for (ParameterMap::const_iterator entry(current_query_params.begin());
       entry != current_query_params.end(); ++entry) {
    if (entry != current_query_params.begin())
      result += '&';
    result += net::EscapeQueryParamValue(entry->first, true);
    result += '=';
    result += net::EscapeQueryParamValue(entry->second, true);
  }
  return GURL(result);
}

void DeviceManagementRequestJobImpl::ConfigureRequest(
    net::URLFetcher* fetcher) {
  // TODO(dcheng): It might make sense to make this take a const
  // scoped_refptr<T>& too eventually.
  fetcher->SetRequestContext(request_context_.get());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_DISABLE_CACHE |
                        (bypass_proxy_ ? net::LOAD_BYPASS_PROXY : 0));
  std::string payload;
  CHECK(request_.SerializeToString(&payload));
  fetcher->SetUploadData(kPostContentType, payload);
  std::string extra_headers;
  if (!gaia_token_.empty())
    extra_headers += kServiceTokenAuthHeader + gaia_token_ + "\n";
  if (!dm_token_.empty())
    extra_headers += kDMTokenAuthHeader + dm_token_ + "\n";
  if (!enrollment_token_.empty())
    extra_headers += kEnrollmentTokenAuthHeader + enrollment_token_ + "\n";
  fetcher->SetExtraRequestHeaders(extra_headers);
}

DeviceManagementRequestJobImpl::RetryMethod
DeviceManagementRequestJobImpl::ShouldRetry(const net::URLFetcher* fetcher) {
  last_error_ = fetcher->GetStatus().error();
  if (FailedWithProxy(fetcher) && !bypass_proxy_) {
    // Retry the job immediately if it failed due to a broken proxy, by
    // bypassing the proxy on the next try.
    bypass_proxy_ = true;
    return RETRY_IMMEDIATELY;
  }

  // Early device policy fetches on ChromeOS and Auto-Enrollment checks are
  // often interrupted during ChromeOS startup when network is not yet ready.
  // Allowing the fetcher to retry once after that is enough to recover; allow
  // it to retry up to 3 times just in case.
  if (IsConnectionError(fetcher->GetStatus()) && retries_count_ < kMaxRetries) {
    ++retries_count_;
    if (type_ == DeviceManagementRequestJob::TYPE_POLICY_FETCH) {
      // We must not delay when retrying policy fetch, because it is a blocking
      // call when logging in.
      return RETRY_IMMEDIATELY;
    } else {
      return RETRY_WITH_DELAY;
    }
  }

  // The request didn't fail, or the limit of retry attempts has been reached;
  // forward the result to the job owner.
  return NO_RETRY;
}

int DeviceManagementRequestJobImpl::GetRetryDelay(RetryMethod method) {
  switch (method) {
    case RETRY_WITH_DELAY:
      return g_retry_delay_ms << (retries_count_ - 1);
    case RETRY_IMMEDIATELY:
      return 0;
    default:
      NOTREACHED();
      return 0;
  }
}

void DeviceManagementRequestJobImpl::PrepareRetry() {
  if (!retry_callback_.is_null())
    retry_callback_.Run(this);
}

void DeviceManagementRequestJobImpl::ReportError(DeviceManagementStatus code) {
  em::DeviceManagementResponse dummy_response;
  callback_.Run(code, net::OK, dummy_response);
}

DeviceManagementRequestJob::~DeviceManagementRequestJob() {}

void DeviceManagementRequestJob::SetGaiaToken(const std::string& gaia_token) {
  gaia_token_ = gaia_token;
}

void DeviceManagementRequestJob::SetOAuthToken(const std::string& oauth_token) {
  AddParameter(dm_protocol::kParamOAuthToken, oauth_token);
}

void DeviceManagementRequestJob::SetDMToken(const std::string& dm_token) {
  dm_token_ = dm_token;
}

void DeviceManagementRequestJob::SetClientID(const std::string& client_id) {
  AddParameter(dm_protocol::kParamDeviceID, client_id);
}

void DeviceManagementRequestJob::SetEnrollmentToken(const std::string& token) {
  enrollment_token_ = token;
}

void DeviceManagementRequestJob::SetCritical(bool critical) {
  if (critical)
    AddParameter(dm_protocol::kParamCritical, "true");
}

em::DeviceManagementRequest* DeviceManagementRequestJob::GetRequest() {
  return &request_;
}

DeviceManagementRequestJob::DeviceManagementRequestJob(
    JobType type,
    const std::string& agent_parameter,
    const std::string& platform_parameter)
    : type_(type) {
  AddParameter(dm_protocol::kParamRequest, JobTypeToRequestType(type));
  AddParameter(dm_protocol::kParamDeviceType, dm_protocol::kValueDeviceType);
  AddParameter(dm_protocol::kParamAppType, dm_protocol::kValueAppType);
  AddParameter(dm_protocol::kParamAgent, agent_parameter);
  AddParameter(dm_protocol::kParamPlatform, platform_parameter);
}

void DeviceManagementRequestJob::SetRetryCallback(
    const RetryCallback& retry_callback) {
  retry_callback_ = retry_callback;
}

void DeviceManagementRequestJob::Start(const Callback& callback) {
  callback_ = callback;
  Run();
}

void DeviceManagementRequestJob::AddParameter(const std::string& name,
                                              const std::string& value) {
  query_params_.push_back(std::make_pair(name, value));
}

// A random value that other fetchers won't likely use.
const int DeviceManagementService::kURLFetcherID = 0xde71ce1d;

DeviceManagementService::~DeviceManagementService() {
  // All running jobs should have been cancelled by now.
  DCHECK(pending_jobs_.empty());
  DCHECK(queued_jobs_.empty());
}

DeviceManagementRequestJob* DeviceManagementService::CreateJob(
    DeviceManagementRequestJob::JobType type,
    const scoped_refptr<net::URLRequestContextGetter>& request_context) {
  DCHECK(thread_checker_.CalledOnValidThread());

  return new DeviceManagementRequestJobImpl(
      type,
      configuration_->GetAgentParameter(),
      configuration_->GetPlatformParameter(),
      this,
      request_context);
}

void DeviceManagementService::ScheduleInitialization(
    int64_t delay_milliseconds) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (initialized_)
    return;
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&DeviceManagementService::Initialize,
                            weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(delay_milliseconds));
}

void DeviceManagementService::Initialize() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (initialized_)
    return;
  initialized_ = true;

  while (!queued_jobs_.empty()) {
    StartJob(queued_jobs_.front());
    queued_jobs_.pop_front();
  }
}

void DeviceManagementService::Shutdown() {
  DCHECK(thread_checker_.CalledOnValidThread());
  weak_ptr_factory_.InvalidateWeakPtrs();
  for (JobFetcherMap::iterator job(pending_jobs_.begin());
       job != pending_jobs_.end();
       ++job) {
    delete job->first;
    queued_jobs_.push_back(job->second);
  }
  pending_jobs_.clear();
}

DeviceManagementService::DeviceManagementService(
    std::unique_ptr<Configuration> configuration)
    : configuration_(std::move(configuration)),
      initialized_(false),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  DCHECK(configuration_);
}

void DeviceManagementService::StartJob(DeviceManagementRequestJobImpl* job) {
  DCHECK(thread_checker_.CalledOnValidThread());

  GURL url = job->GetURL(GetServerUrl());
  DCHECK(url.is_valid()) << "Maybe invalid --device-management-url was passed?";

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("device_management_service", R"(
        semantics {
          sender: "Cloud Policy"
          description:
            "Communication with the Cloud Policy backend, used to check for "
            "the existence of cloud policy for the signed-in account, and to "
            "load/update cloud policy if it exists."
          trigger:
            "Sign in to Chrome, also periodic refreshes."
          data:
            "During initial signin or device enrollment, auth data is sent up "
            "as part of registration. After initial signin/enrollment, if the "
            "session or device is managed, a unique device or profile ID is "
            "sent with every future request. On Chrome OS, other diagnostic "
            "information can be sent up for managed sessions, including which "
            "users have used the device, device hardware status, connected "
            "networks, CPU usage, etc."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be controlled by Chrome settings, but users "
            "can sign out of Chrome to disable it."
          chrome_policy {
            SigninAllowed {
              policy_options {mode: MANDATORY}
              SigninAllowed: false
            }
          }
        })");
  net::URLFetcher* fetcher =
      net::URLFetcher::Create(kURLFetcherID, std::move(url),
                              net::URLFetcher::POST, this, traffic_annotation)
          .release();
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher, data_use_measurement::DataUseUserData::POLICY);
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_DO_NOT_SEND_COOKIES);
  job->ConfigureRequest(fetcher);
  pending_jobs_[fetcher] = job;
  fetcher->Start();
}

void DeviceManagementService::StartJobAfterDelay(
    base::WeakPtr<DeviceManagementRequestJobImpl> job) {
  // Check if the job still exists (it is possible that it had been canceled
  // while we were waiting for the retry).
  if (job) {
    StartJob(job.get());
  }
}

std::string DeviceManagementService::GetServerUrl() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return configuration_->GetServerUrl();
}

// static
void DeviceManagementService::SetRetryDelayForTesting(long retry_delay_ms) {
  CHECK_GE(retry_delay_ms, 0);
  g_retry_delay_ms = retry_delay_ms;
}

void DeviceManagementService::OnURLFetchComplete(
    const net::URLFetcher* source) {
  JobFetcherMap::iterator entry(pending_jobs_.find(source));
  if (entry == pending_jobs_.end()) {
    NOTREACHED() << "Callback from foreign URL fetcher";
    return;
  }

  DeviceManagementRequestJobImpl* job = entry->second;
  pending_jobs_.erase(entry);

  DeviceManagementRequestJobImpl::RetryMethod retry_method =
      job->ShouldRetry(source);
  if (retry_method != DeviceManagementRequestJobImpl::RetryMethod::NO_RETRY) {
    job->PrepareRetry();
    int delay = job->GetRetryDelay(retry_method);
    LOG(WARNING) << "Dmserver request failed, retrying in " << delay / 1000
                 << "s.";
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&DeviceManagementService::StartJobAfterDelay,
                   weak_ptr_factory_.GetWeakPtr(), job->GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(delay));
  } else {
    std::string data;
    source->GetResponseAsString(&data);
    job->HandleResponse(source->GetStatus(), source->GetResponseCode(), data);
  }
  delete source;
}

void DeviceManagementService::AddJob(DeviceManagementRequestJobImpl* job) {
  if (initialized_)
    StartJob(job);
  else
    queued_jobs_.push_back(job);
}

void DeviceManagementService::RemoveJob(DeviceManagementRequestJobImpl* job) {
  for (JobFetcherMap::iterator entry(pending_jobs_.begin());
       entry != pending_jobs_.end();
       ++entry) {
    if (entry->second == job) {
      delete entry->first;
      pending_jobs_.erase(entry);
      return;
    }
  }

  const JobQueue::iterator elem =
      std::find(queued_jobs_.begin(), queued_jobs_.end(), job);
  if (elem != queued_jobs_.end())
    queued_jobs_.erase(elem);
}

}  // namespace policy
