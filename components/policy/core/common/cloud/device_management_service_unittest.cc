// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/device_management_service.h"

#include <ostream>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/mock_device_management_service.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Mock;
using testing::_;

namespace em = enterprise_management;

namespace policy {

const char kServiceUrl[] = "https://example.com/management_service";

// Encoded empty response messages for testing the error code paths.
const char kResponseEmpty[] = "\x08\x00";

#define PROTO_STRING(name) (std::string(name, arraysize(name) - 1))

// Some helper constants.
const char kGaiaAuthToken[] = "gaia-auth-token";
const char kOAuthToken[] = "oauth-token";
const char kDMToken[] = "device-management-token";
const char kClientID[] = "device-id";
const char kRobotAuthCode[] = "robot-oauth-auth-code";
const char kEnrollmentToken[] = "enrollment_token";

// Unit tests for the device management policy service. The tests are run
// against a TestURLFetcherFactory that is used to short-circuit the request
// without calling into the actual network stack.
class DeviceManagementServiceTestBase : public testing::Test {
 protected:
  DeviceManagementServiceTestBase() {
    // Set retry delay to prevent timeouts.
    policy::DeviceManagementService::SetRetryDelayForTesting(0);

    request_context_ =
        new net::TestURLRequestContextGetter(loop_.task_runner());
    ResetService();
    InitializeService();
  }

  ~DeviceManagementServiceTestBase() override {
    service_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void ResetService() {
    std::unique_ptr<DeviceManagementService::Configuration> configuration(
        new MockDeviceManagementServiceConfiguration(kServiceUrl));
    service_.reset(new DeviceManagementService(std::move(configuration)));
  }

  void InitializeService() {
    service_->ScheduleInitialization(0);
    base::RunLoop().RunUntilIdle();
  }

  net::TestURLFetcher* GetFetcher() {
    return factory_.GetFetcherByID(DeviceManagementService::kURLFetcherID);
  }

  DeviceManagementRequestJob* StartRegistrationJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_REGISTRATION, request_context_.get());
    job->SetGaiaToken(kGaiaAuthToken);
    job->SetOAuthToken(kOAuthToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_register_request();
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartCertBasedRegistrationJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_CERT_BASED_REGISTRATION,
        request_context_.get());
    job->SetGaiaToken(kGaiaAuthToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_register_request();
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartTokenEnrollmentJob() {
    DeviceManagementRequestJob* job =
        service_->CreateJob(DeviceManagementRequestJob::TYPE_TOKEN_ENROLLMENT,
                            request_context_.get());
    job->SetEnrollmentToken(kEnrollmentToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_register_request();
    job->SetRetryCallback(base::BindRepeating(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::BindRepeating(&DeviceManagementServiceTestBase::OnJobDone,
                                   base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartApiAuthCodeFetchJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_API_AUTH_CODE_FETCH,
        request_context_.get());
    job->SetGaiaToken(kGaiaAuthToken);
    job->SetOAuthToken(kOAuthToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_service_api_access_request();
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartUnregistrationJob() {
    DeviceManagementRequestJob* job =
        service_->CreateJob(DeviceManagementRequestJob::TYPE_UNREGISTRATION,
                            request_context_.get());
    job->SetDMToken(kDMToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_unregister_request();
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartPolicyFetchJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_POLICY_FETCH, request_context_.get());
    job->SetGaiaToken(kGaiaAuthToken);
    job->SetOAuthToken(kOAuthToken);
    job->SetClientID(kClientID);
    em::PolicyFetchRequest* fetch_request =
        job->GetRequest()->mutable_policy_request()->add_request();
    fetch_request->set_policy_type(dm_protocol::kChromeUserPolicyType);
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartCriticalPolicyFetchJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_POLICY_FETCH, request_context_.get());
    job->SetGaiaToken(kGaiaAuthToken);
    job->SetOAuthToken(kOAuthToken);
    job->SetClientID(kClientID);
    job->SetCritical(true);
    em::PolicyFetchRequest* fetch_request =
        job->GetRequest()->mutable_policy_request()->add_request();
    fetch_request->set_policy_type(dm_protocol::kChromeUserPolicyType);
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartAutoEnrollmentJob() {
    DeviceManagementRequestJob* job =
        service_->CreateJob(DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT,
                            request_context_.get());
    job->SetClientID(kClientID);
    em::DeviceAutoEnrollmentRequest* request =
        job->GetRequest()->mutable_auto_enrollment_request();
    request->set_modulus(1);
    request->set_remainder(0);
    job->SetRetryCallback(base::Bind(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::Bind(&DeviceManagementServiceTestBase::OnJobDone,
                          base::Unretained(this)));
    return job;
  }

  DeviceManagementRequestJob* StartAppInstallReportJob() {
    DeviceManagementRequestJob* job = service_->CreateJob(
        DeviceManagementRequestJob::TYPE_UPLOAD_APP_INSTALL_REPORT,
        request_context_.get());
    job->SetDMToken(kDMToken);
    job->SetClientID(kClientID);
    job->GetRequest()->mutable_app_install_report_request();
    job->SetRetryCallback(base::BindRepeating(
        &DeviceManagementServiceTestBase::OnJobRetry, base::Unretained(this)));
    job->Start(base::AdaptCallbackForRepeating(base::BindOnce(
        &DeviceManagementServiceTestBase::OnJobDone, base::Unretained(this))));
    return job;
  }

  void SendResponse(net::TestURLFetcher* fetcher,
                    net::Error error,
                    int http_status,
                    const std::string& response) {
    fetcher->set_url(GURL(kServiceUrl));
    fetcher->set_status(net::URLRequestStatus::FromError(error));
    fetcher->set_response_code(http_status);
    fetcher->SetResponseString(response);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  MOCK_METHOD3(OnJobDone, void(DeviceManagementStatus, int,
                               const em::DeviceManagementResponse&));

  MOCK_METHOD1(OnJobRetry, void(DeviceManagementRequestJob*));

  base::MessageLoop loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory factory_;
  std::unique_ptr<DeviceManagementService> service_;
};

struct FailedRequestParams {
  FailedRequestParams(DeviceManagementStatus expected_status,
                      net::Error error,
                      int http_status,
                      const std::string& response)
      : expected_status_(expected_status),
        error_(error),
        http_status_(http_status),
        response_(response) {}

  DeviceManagementStatus expected_status_;
  net::Error error_;
  int http_status_;
  std::string response_;
};

void PrintTo(const FailedRequestParams& params, std::ostream* os) {
  *os << "FailedRequestParams " << params.expected_status_
      << " " << params.error_
      << " " << params.http_status_;
}

// A parameterized test case for erroneous response situations, they're mostly
// the same for all kinds of requests.
class DeviceManagementServiceFailedRequestTest
    : public DeviceManagementServiceTestBase,
      public testing::WithParamInterface<FailedRequestParams> {
};

TEST_P(DeviceManagementServiceFailedRequestTest, RegisterRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, CertBasedRegisterRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartCertBasedRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, TokenEnrollmentRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartTokenEnrollmentJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, ApiAuthCodeFetchRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartApiAuthCodeFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, UnregisterRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartUnregistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, PolicyRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartPolicyFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, AutoEnrollmentRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartAutoEnrollmentJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, AppInstallReportRequest) {
  EXPECT_CALL(*this, OnJobDone(GetParam().expected_status_, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartAppInstallReportJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  SendResponse(fetcher, GetParam().error_, GetParam().http_status_,
               GetParam().response_);
}

INSTANTIATE_TEST_CASE_P(
    DeviceManagementServiceFailedRequestTestInstance,
    DeviceManagementServiceFailedRequestTest,
    testing::Values(
        FailedRequestParams(
            DM_STATUS_REQUEST_FAILED,
            net::ERR_FAILED,
            200,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_HTTP_STATUS_ERROR,
            net::OK,
            666,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_RESPONSE_DECODING_ERROR,
            net::OK,
            200,
            PROTO_STRING("Not a protobuf.")),
        FailedRequestParams(
            DM_STATUS_SERVICE_MANAGEMENT_NOT_SUPPORTED,
            net::OK,
            403,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_INVALID_SERIAL_NUMBER,
            net::OK,
            405,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_DEVICE_ID_CONFLICT,
            net::OK,
            409,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_DEVICE_NOT_FOUND,
            net::OK,
            410,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_MANAGEMENT_TOKEN_INVALID,
            net::OK,
            401,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_REQUEST_INVALID,
            net::OK,
            400,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_TEMPORARY_UNAVAILABLE,
            net::OK,
            404,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_ACTIVATION_PENDING,
            net::OK,
            412,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DM_STATUS_SERVICE_MISSING_LICENSES,
            net::OK,
            402,
            PROTO_STRING(kResponseEmpty))));

// Simple query parameter parser for testing.
class QueryParams {
 public:
  explicit QueryParams(const std::string& query) {
    base::SplitStringIntoKeyValuePairs(query, '=', '&', &params_);
  }

  bool Check(const std::string& name, const std::string& expected_value) {
    bool found = false;
    for (ParamMap::const_iterator i(params_.begin()); i != params_.end(); ++i) {
      std::string unescaped_name(net::UnescapeBinaryURLComponent(
          i->first, net::UnescapeRule::REPLACE_PLUS_WITH_SPACE));
      if (unescaped_name == name) {
        if (found)
          return false;
        found = true;
        std::string unescaped_value(net::UnescapeBinaryURLComponent(
            i->second, net::UnescapeRule::REPLACE_PLUS_WITH_SPACE));
        if (unescaped_value != expected_value)
          return false;
      }
    }
    return found;
  }

 private:
  typedef base::StringPairs ParamMap;
  ParamMap params_;
};

class DeviceManagementServiceTest
    : public DeviceManagementServiceTestBase {
 protected:
  void CheckURLAndQueryParams(const GURL& request_url,
                              const std::string& request_type,
                              const std::string& device_id,
                              const std::string& last_error,
                              bool critical = false) {
    const GURL service_url(kServiceUrl);
    EXPECT_EQ(service_url.scheme(), request_url.scheme());
    EXPECT_EQ(service_url.host(), request_url.host());
    EXPECT_EQ(service_url.port(), request_url.port());
    EXPECT_EQ(service_url.path(), request_url.path());

    QueryParams query_params(request_url.query());
    EXPECT_TRUE(query_params.Check(dm_protocol::kParamRequest, request_type));
    EXPECT_TRUE(query_params.Check(dm_protocol::kParamDeviceID, device_id));
    EXPECT_TRUE(query_params.Check(dm_protocol::kParamDeviceType,
                                   dm_protocol::kValueDeviceType));
    EXPECT_TRUE(query_params.Check(dm_protocol::kParamAppType,
                                   dm_protocol::kValueAppType));
    EXPECT_EQ(critical,
              query_params.Check(dm_protocol::kParamCritical, "true"));
    if (last_error == "") {
      EXPECT_TRUE(query_params.Check(dm_protocol::kParamRetry, "false"));
    } else {
      EXPECT_TRUE(query_params.Check(dm_protocol::kParamRetry, "true"));
      EXPECT_TRUE(query_params.Check(dm_protocol::kParamLastError, last_error));
    }
  }
};

MATCHER_P(MessageEquals, reference, "") {
  std::string reference_data;
  std::string arg_data;
  return arg.SerializeToString(&arg_data) &&
         reference.SerializeToString(&reference_data) &&
         arg_data == reference_data;
}

TEST_F(DeviceManagementServiceTest, RegisterRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_register_response()->
      set_device_management_token(kDMToken);
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_SUCCESS, _,
                               MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestRegister, kClientID, "");

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, CriticalRequest) {
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartCriticalPolicyFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestPolicy, kClientID, "", true);
}

TEST_F(DeviceManagementServiceTest, CertBasedRegisterRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_register_response()->
      set_device_management_token(kDMToken);
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_SUCCESS, _,
                               MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartCertBasedRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestCertBasedRegister, kClientID,
                         "");

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, TokenEnrollmentRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_register_response()->set_device_management_token(
      kDMToken);
  EXPECT_CALL(
      *this, OnJobDone(DM_STATUS_SUCCESS, _, MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartTokenEnrollmentJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestTokenEnrollment, kClientID,
                         "");

  // Make sure request is properly authorized.
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string header;
  ASSERT_TRUE(headers.GetHeader("Authorization", &header));
  EXPECT_EQ("GoogleEnrollmentToken token=enrollment_token", header);

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, ApiAuthCodeFetchRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_service_api_access_response()->set_auth_code(
      kRobotAuthCode);
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_SUCCESS, _,
                               MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartApiAuthCodeFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestApiAuthorization, kClientID,
                         "");

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, UnregisterRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_unregister_response();
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_SUCCESS, _,
                               MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartUnregistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // Check the data the fetcher received.
  const GURL& request_url(fetcher->GetOriginalURL());
  const GURL service_url(kServiceUrl);
  EXPECT_EQ(service_url.scheme(), request_url.scheme());
  EXPECT_EQ(service_url.host(), request_url.host());
  EXPECT_EQ(service_url.port(), request_url.port());
  EXPECT_EQ(service_url.path(), request_url.path());

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestUnregister, kClientID, "");

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, AppInstallReportRequest) {
  em::DeviceManagementResponse expected_response;
  expected_response.mutable_app_install_report_response();
  EXPECT_CALL(
      *this, OnJobDone(DM_STATUS_SUCCESS, _, MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartAppInstallReportJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestAppInstallReport, kClientID,
                         "");

  std::string expected_data;
  ASSERT_TRUE(request_job->GetRequest()->SerializeToString(&expected_data));
  EXPECT_EQ(expected_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, CancelRegisterRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelCertBasedRegisterRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartCertBasedRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelTokenEnrollmentRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartTokenEnrollmentJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelApiAuthCodeFetch) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartApiAuthCodeFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelUnregisterRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartUnregistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelPolicyRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartPolicyFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, CancelAppInstallReportRequest) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartAppInstallReportJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  request_job.reset();
}

TEST_F(DeviceManagementServiceTest, JobQueueing) {
  // Start with a non-initialized service.
  ResetService();

  em::DeviceManagementResponse expected_response;
  expected_response.mutable_register_response()->
      set_device_management_token(kDMToken);
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_SUCCESS, _,
                               MessageEquals(expected_response)));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);

  // Make a request. We should not see any fetchers being created.
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_FALSE(fetcher);

  // Now initialize the service. That should start the job.
  InitializeService();
  fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  factory_.RemoveFetcherFromMap(DeviceManagementService::kURLFetcherID);

  // Check that the request is processed as expected.
  std::string response_data;
  ASSERT_TRUE(expected_response.SerializeToString(&response_data));
  SendResponse(fetcher, net::OK, 200, response_data);
}

TEST_F(DeviceManagementServiceTest, CancelRequestAfterShutdown) {
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartPolicyFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // Shutdown the service and cancel the job afterwards.
  service_->Shutdown();
  request_job.reset();
}

ACTION_P(ResetPointer, pointer) {
  pointer->reset();
}

TEST_F(DeviceManagementServiceTest, CancelDuringCallback) {
  // Make a request.
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  EXPECT_CALL(*this, OnJobDone(_, _, _))
      .WillOnce(ResetPointer(&request_job));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);

  // Generate a callback.
  SendResponse(fetcher, net::OK, 500, std::string());

  // Job should have been reset.
  EXPECT_FALSE(request_job);
}

TEST_F(DeviceManagementServiceTest, RetryOnProxyError) {
  // Make a request.
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_));

  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(0, fetcher->GetLoadFlags() & net::LOAD_BYPASS_PROXY);
  // Not a retry.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestRegister, kClientID, "");
  const std::string upload_data(fetcher->upload_data());

  // Generate a callback with a proxy failure.
  SendResponse(fetcher, net::ERR_PROXY_CONNECTION_FAILED, 200, std::string());
  base::RunLoop().RunUntilIdle();

  // Verify that a new URLFetcher was started that bypasses the proxy.
  fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_TRUE(fetcher->GetLoadFlags() & net::LOAD_BYPASS_PROXY);
  EXPECT_EQ(upload_data, fetcher->upload_data());
  // Retry with last error net::ERR_PROXY_CONNECTION_FAILED.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestRegister, kClientID,
                         std::to_string(net::ERR_PROXY_CONNECTION_FAILED));
}

TEST_F(DeviceManagementServiceTest, RetryOnBadResponseFromProxy) {
  // Make a request.
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_));

  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(0, fetcher->GetLoadFlags() & net::LOAD_BYPASS_PROXY);
  const GURL original_url(fetcher->GetOriginalURL());
  const std::string upload_data(fetcher->upload_data());
  fetcher->set_was_fetched_via_proxy(true);
  scoped_refptr<net::HttpResponseHeaders> headers;
  headers = new net::HttpResponseHeaders(
      "HTTP/1.1 200 OK\0Content-type: bad/type\0\0");
  fetcher->set_response_headers(headers);

  // Generate a callback with a valid http response, that was generated by
  // a bad/wrong proxy.
  SendResponse(fetcher, net::OK, 200, std::string());
  base::RunLoop().RunUntilIdle();

  // Verify that a new URLFetcher was started that bypasses the proxy.
  fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_NE(0, fetcher->GetLoadFlags() & net::LOAD_BYPASS_PROXY);
  EXPECT_EQ(original_url, fetcher->GetOriginalURL());
  EXPECT_EQ(upload_data, fetcher->upload_data());
}

TEST_F(DeviceManagementServiceTest, RetryOnNetworkChanges) {
  // Make a request.
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_));

  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  // Not a retry.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestRegister, kClientID, "");
  const std::string original_upload_data(fetcher->upload_data());

  // Make it fail with ERR_NETWORK_CHANGED.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->set_url(GURL(kServiceUrl));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  base::RunLoop().RunUntilIdle();

  // Verify that a new URLFetcher was started that retries this job, after
  // having called OnJobRetry.
  Mock::VerifyAndClearExpectations(this);
  fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(original_upload_data, fetcher->upload_data());
  EXPECT_EQ(net::URLRequestStatus::SUCCESS, fetcher->GetStatus().status());
  // Retry with last error net::ERR_NETWORK_CHANGED.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestRegister, kClientID,
                         std::to_string(net::ERR_NETWORK_CHANGED));
}

TEST_F(DeviceManagementServiceTest, PolicyFetchRetryImmediately) {
  // We must not wait before a policy fetch retry, so this must not time out.
  policy::DeviceManagementService::SetRetryDelayForTesting(60000);

  // Make a request.
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_));

  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartPolicyFetchJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  // Not a retry.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestPolicy, kClientID, "");
  const std::string original_upload_data(fetcher->upload_data());

  // Make it fail with ERR_NETWORK_CHANGED.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->set_url(GURL(kServiceUrl));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  base::RunLoop().RunUntilIdle();

  // Verify that a new URLFetcher was started that retries this job, after
  // having called OnJobRetry.
  Mock::VerifyAndClearExpectations(this);
  fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(original_upload_data, fetcher->upload_data());
  EXPECT_EQ(net::URLRequestStatus::SUCCESS, fetcher->GetStatus().status());
  // Retry with last error net::ERR_NETWORK_CHANGED.
  CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                         dm_protocol::kValueRequestPolicy, kClientID,
                         std::to_string(net::ERR_NETWORK_CHANGED));
}

TEST_F(DeviceManagementServiceTest, RetryLimit) {
  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());

  // Simulate 3 failed network requests.
  for (int i = 0; i < 3; ++i) {
    // Make the current fetcher fail with ERR_NETWORK_CHANGED.
    net::TestURLFetcher* fetcher = GetFetcher();
    ASSERT_TRUE(fetcher);
    EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
    EXPECT_CALL(*this, OnJobRetry(_));
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                              net::ERR_NETWORK_CHANGED));
    fetcher->set_url(GURL(kServiceUrl));
    if (i == 0) {
      // Not a retry.
      CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                             dm_protocol::kValueRequestRegister, kClientID, "");
    } else {
      // Retry with last error net::ERR_NETWORK_CHANGED.
      CheckURLAndQueryParams(fetcher->GetOriginalURL(),
                             dm_protocol::kValueRequestRegister, kClientID,
                             std::to_string(net::ERR_NETWORK_CHANGED));
    }
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    base::RunLoop().RunUntilIdle();
    Mock::VerifyAndClearExpectations(this);
  }

  // At the next failure the DeviceManagementService should give up retrying and
  // pass the error code to the job's owner.
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);
  EXPECT_CALL(*this, OnJobDone(DM_STATUS_REQUEST_FAILED, _, _));
  EXPECT_CALL(*this, OnJobRetry(_)).Times(0);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->set_url(GURL(kServiceUrl));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClearExpectations(this);
}

TEST_F(DeviceManagementServiceTest, CancelDuringRetry) {
  // Make a request.
  EXPECT_CALL(*this, OnJobDone(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnJobRetry(_));

  std::unique_ptr<DeviceManagementRequestJob> request_job(
      StartRegistrationJob());
  net::TestURLFetcher* fetcher = GetFetcher();
  ASSERT_TRUE(fetcher);

  // Make it fail with ERR_NETWORK_CHANGED.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->set_url(GURL(kServiceUrl));
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Before we retry, cancel the job
  request_job.reset();

  // We must not crash
  base::RunLoop().RunUntilIdle();
}

}  // namespace policy
