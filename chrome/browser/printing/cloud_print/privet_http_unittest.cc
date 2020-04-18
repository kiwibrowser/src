// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_http.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/printing/cloud_print/privet_http_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "printing/buildflags/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/printing/pwg_raster_converter.h"
#include "printing/pwg_raster_settings.h"
#endif

namespace cloud_print {

namespace {

using testing::NiceMock;
using testing::StrictMock;
using testing::TestWithParam;
using testing::ValuesIn;

using content::BrowserThread;
using net::EmbeddedTestServer;

const char kSampleInfoResponse[] =
    R"({
         "version": "1.0",
         "name": "Common printer",
         "description": "Printer connected through Chrome connector",
         "url": "https://www.google.com/cloudprint",
         "type": [ "printer" ],
         "id": "",
         "device_state": "idle",
         "connection_state": "online",
         "manufacturer": "Google",
         "model": "Google Chrome",
         "serial_number": "1111-22222-33333-4444",
         "firmware": "24.0.1312.52",
         "uptime": 600,
         "setup_url": "http://support.google.com/",
         "support_url": "http://support.google.com/cloudprint/?hl=en",
         "update_url": "http://support.google.com/cloudprint/?hl=en",
         "x-privet-token": "SampleTokenForTesting",
         "api": [
           "/privet/accesstoken",
           "/privet/capabilities",
           "/privet/printer/submitdoc",
         ]
       })";

const char kSampleInfoResponseRegistered[] =
    R"({
         "version": "1.0",
         "name": "Common printer",
         "description": "Printer connected through Chrome connector",
         "url": "https://www.google.com/cloudprint",
         "type": [ "printer" ],
         "id": "MyDeviceID",
         "device_state": "idle",
         "connection_state": "online",
         "manufacturer": "Google",
         "model": "Google Chrome",
         "serial_number": "1111-22222-33333-4444",
         "firmware": "24.0.1312.52",
         "uptime": 600,
         "setup_url": "http://support.google.com/",
         "support_url": "http://support.google.com/cloudprint/?hl=en",
         "update_url": "http://support.google.com/cloudprint/?hl=en",
         "x-privet-token": "SampleTokenForTesting",
         "api": [
           "/privet/accesstoken",
           "/privet/capabilities",
           "/privet/printer/submitdoc",
         ]
       })";

const char kSampleRegisterStartResponse[] =
    R"({
         "user": "example@google.com",
         "action": "start"
       })";

const char kSampleRegisterGetClaimTokenResponse[] =
    R"({
         "action": "getClaimToken",
         "user": "example@google.com",
         "token": "MySampleToken",
         "claim_url": "https://domain.com/SoMeUrL"
       })";

const char kSampleRegisterCompleteResponse[] =
    R"({
         "user": "example@google.com",
         "action": "complete",
         "device_id": "MyDeviceID"
       })";

const char kSampleXPrivetErrorResponse[] =
    R"({ "error": "invalid_x_privet_token" })";

const char kSampleRegisterErrorTransient[] =
    R"({ "error": "device_busy", "timeout": 1})";

const char kSampleRegisterErrorPermanent[] =
    R"({ "error": "user_cancel" })";

const char kSampleInfoResponseBadJson[] = "{";

const char kSampleRegisterCancelResponse[] =
    R"({
         "user": "example@google.com",
         "action": "cancel"
       })";

const char kSampleCapabilitiesResponse[] =
    R"({
         "version" : "1.0",
         "printer" : {
           "supported_content_type" : [
             { "content_type" : "application/pdf" },
             { "content_type" : "image/pwg-raster" }
           ]
         }
       })";

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
const char kSampleInfoResponseWithCreatejob[] =
    R"({
         "version": "1.0",
         "name": "Common printer",
         "description": "Printer connected through Chrome connector",
         "url": "https://www.google.com/cloudprint",
         "type": [ "printer" ],
         "id": "",
         "device_state": "idle",
         "connection_state": "online",
         "manufacturer": "Google",
         "model": "Google Chrome",
         "serial_number": "1111-22222-33333-4444",
         "firmware": "24.0.1312.52",
         "uptime": 600,
         "setup_url": "http://support.google.com/",
         "support_url": "http://support.google.com/cloudprint/?hl=en",
         "update_url": "http://support.google.com/cloudprint/?hl=en",
         "x-privet-token": "SampleTokenForTesting",
         "api": [
           "/privet/accesstoken",
           "/privet/capabilities",
           "/privet/printer/createjob",
           "/privet/printer/submitdoc",
         ]
       })";

const char kSampleLocalPrintResponse[] =
    R"({
         "job_id": "123",
         "expires_in": 500,
         "job_type": "application/pdf",
         "job_size": 16,
         "job_name": "Sample job name",
       })";

const char kSampleCapabilitiesResponsePWGOnly[] =
    R"({
         "version" : "1.0",
         "printer" : {
           "supported_content_type" : [
              { "content_type" : "image/pwg-raster" }
           ]
         }
       })";

const char kSampleErrorResponsePrinterBusy[] =
    R"({
         "error": "invalid_print_job",
         "timeout": 1
       })";

const char kSampleInvalidDocumentTypeResponse[] =
    R"({ "error" : "invalid_document_type" })";

const char kSampleCreatejobResponse[] = R"({ "job_id": "1234" })";

const char kSampleCapabilitiesResponseWithAnyMimetype[] =
    R"({
         "version" : "1.0",
         "printer" : {
           "supported_content_type" : [
             { "content_type" : "*/*" },
             { "content_type" : "image/pwg-raster" }
           ]
         }
       })";

const char kSampleCJT[] = R"({ "version" : "1.0" })";

const char kSampleCapabilitiesResponsePWGSettings[] =
    R"({
         "version" : "1.0",
         "printer" : {
           "pwg_raster_config" : {
             "document_sheet_back" : "MANUAL_TUMBLE",
             "reverse_order_streaming": true
           },
           "supported_content_type" : [
             { "content_type" : "image/pwg-raster" }
           ]
         }
       })";

const char kSampleCapabilitiesResponsePWGSettingsMono[] =
    R"({
         "version": "1.0",
         "printer": {
           "pwg_raster_config": {
             "document_type_supported": [ "SGRAY_8" ],
             "document_sheet_back": "ROTATED"
           }
         }
       })";

const char kSampleCJTDuplex[] =
    R"({
         "version" : "1.0",
         "print": { "duplex": {"type": "SHORT_EDGE"} }
       })";

const char kSampleCJTMono[] =
    R"({
         "version" : "1.0",
         "print": { "color": {"type": "STANDARD_MONOCHROME"} }
       })";
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

const char* const kTestParams[] = {"8.8.4.4", "2001:4860:4860::8888"};

// Return the representation of the given JSON that would be outputted by
// JSONWriter. This ensures the same JSON values are represented by the same
// string.
std::string NormalizeJson(const std::string& json) {
  std::string result = json;
  std::unique_ptr<base::Value> value = base::JSONReader::Read(result);
  DCHECK(value) << result;
  base::JSONWriter::Write(*value, &result);
  return result;
}

class MockTestURLFetcherFactoryDelegate
    : public net::TestURLFetcher::DelegateForTests {
 public:
  // Callback issued correspondingly to the call to the |Start()| method.
  MOCK_METHOD1(OnRequestStart, void(int fetcher_id));

  // Callback issued correspondingly to the call to |AppendChunkToUpload|.
  // Uploaded chunks can be retrieved with the |upload_chunks()| getter.
  MOCK_METHOD1(OnChunkUpload, void(int fetcher_id));

  // Callback issued correspondingly to the destructor.
  MOCK_METHOD1(OnRequestEnd, void(int fetcher_id));
};

class PrivetHTTPTest : public TestWithParam<const char*> {
 public:
  PrivetHTTPTest() {
    PrivetURLFetcher::ResetTokenMapForTest();

    request_context_ = base::MakeRefCounted<net::TestURLRequestContextGetter>(
        base::ThreadTaskRunnerHandle::Get());
    privet_client_ = PrivetV1HTTPClient::CreateDefault(
        std::make_unique<PrivetHTTPClientImpl>(
            "sampleDevice._privet._tcp.local",
            net::HostPortPair(GetParam(), 6006), request_context_.get()));
    fetcher_factory_.SetDelegateForTests(&fetcher_delegate_);
  }

  GURL GetUrl(const std::string& path) const {
    std::string host = GetParam();
    if (host.find(":") != std::string::npos)
      host = "[" + host + "]";
    return GURL("http://" + host + ":6006" + path);
  }

  bool SuccessfulResponseToURL(const GURL& url,
                               const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher) {
      ADD_FAILURE();
      return false;
    }

    EXPECT_EQ(url, fetcher->GetOriginalURL());
    if (url != fetcher->GetOriginalURL())
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                              net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

  bool SuccessfulResponseToURLAndData(const GURL& url,
                                      const std::string& data,
                                      const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher) {
      ADD_FAILURE();
      return false;
    }

    EXPECT_EQ(url, fetcher->GetOriginalURL());

    EXPECT_EQ(data, fetcher->upload_data());
    if (data != fetcher->upload_data())
      return false;

    return SuccessfulResponseToURL(url, response);
  }

  bool SuccessfulResponseToURLAndJSONData(const GURL& url,
                                          const std::string& data,
                                          const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher) {
      ADD_FAILURE();
      return false;
    }

    EXPECT_EQ(url, fetcher->GetOriginalURL());

    std::string normalized_data = NormalizeJson(data);
    std::string normalized_upload_data = NormalizeJson(fetcher->upload_data());
    EXPECT_EQ(normalized_data, normalized_upload_data);
    if (normalized_data != normalized_upload_data)
      return false;

    return SuccessfulResponseToURL(url, response);
  }

  bool SuccessfulResponseToURLAndFileData(const GURL& url,
                                          const std::string& file_data,
                                          const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher) {
      ADD_FAILURE();
      return false;
    }

    EXPECT_EQ(url, fetcher->GetOriginalURL());

    EXPECT_EQ(file_data, fetcher->upload_data());
    if (file_data != fetcher->upload_data())
      return false;

    return SuccessfulResponseToURL(url, response);
  }


  void RunFor(base::TimeDelta time_period) {
    base::CancelableCallback<void()> callback(base::Bind(
        &PrivetHTTPTest::Stop, base::Unretained(this)));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, callback.callback(), time_period);

    base::RunLoop().Run();
    callback.Cancel();
  }

  void Stop() { base::RunLoop::QuitCurrentWhenIdleDeprecated(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  std::unique_ptr<PrivetV1HTTPClient> privet_client_;
  NiceMock<MockTestURLFetcherFactoryDelegate> fetcher_delegate_;
};

class MockJSONCallback{
 public:
  void OnPrivetJSONDone(const base::DictionaryValue* value) {
    value_.reset(value ? value->DeepCopy() : nullptr);
    OnPrivetJSONDoneInternal();
  }

  MOCK_METHOD0(OnPrivetJSONDoneInternal, void());

  const base::DictionaryValue* value() { return value_.get(); }
  PrivetJSONOperation::ResultCallback callback() {
    return base::Bind(&MockJSONCallback::OnPrivetJSONDone,
                      base::Unretained(this));
  }
 protected:
  std::unique_ptr<base::DictionaryValue> value_;
};

class MockRegisterDelegate : public PrivetRegisterOperation::Delegate {
 public:
  void OnPrivetRegisterClaimToken(
      PrivetRegisterOperation* operation,
      const std::string& token,
      const GURL& url) override {
    OnPrivetRegisterClaimTokenInternal(token, url);
  }

  MOCK_METHOD2(OnPrivetRegisterClaimTokenInternal, void(
      const std::string& token,
      const GURL& url));

  void OnPrivetRegisterError(
      PrivetRegisterOperation* operation,
      const std::string& action,
      PrivetRegisterOperation::FailureReason reason,
      int printer_http_code,
      const base::DictionaryValue* json) override {
    // TODO(noamsml): Save and test for JSON?
    OnPrivetRegisterErrorInternal(action, reason, printer_http_code);
  }

  MOCK_METHOD3(OnPrivetRegisterErrorInternal,
               void(const std::string& action,
                    PrivetRegisterOperation::FailureReason reason,
                    int printer_http_code));

  void OnPrivetRegisterDone(
      PrivetRegisterOperation* operation,
      const std::string& device_id) override {
    OnPrivetRegisterDoneInternal(device_id);
  }

  MOCK_METHOD1(OnPrivetRegisterDoneInternal,
               void(const std::string& device_id));
};

class MockLocalPrintDelegate : public PrivetLocalPrintOperation::Delegate {
 public:
  void OnPrivetPrintingDone(
      const PrivetLocalPrintOperation* print_operation) override {
    OnPrivetPrintingDoneInternal();
  }

  MOCK_METHOD0(OnPrivetPrintingDoneInternal, void());

  void OnPrivetPrintingError(const PrivetLocalPrintOperation* print_operation,
                             int http_code) override {
    OnPrivetPrintingErrorInternal(http_code);
  }

  MOCK_METHOD1(OnPrivetPrintingErrorInternal, void(int http_code));
};

class PrivetInfoTest : public PrivetHTTPTest {
 public:
  void SetUp() override {
    info_operation_ = privet_client_->CreateInfoOperation(
        info_callback_.callback());
  }

 protected:
  std::unique_ptr<PrivetJSONOperation> info_operation_;
  StrictMock<MockJSONCallback> info_callback_;
};

INSTANTIATE_TEST_CASE_P(PrivetTests, PrivetInfoTest, ValuesIn(kTestParams));

TEST_P(PrivetInfoTest, SuccessfulInfo) {
  info_operation_->Start();

  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GetUrl("/privet/info"), fetcher->GetOriginalURL());

  fetcher->SetResponseString(kSampleInfoResponse);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                            net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(info_callback_, OnPrivetJSONDoneInternal());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

TEST_P(PrivetInfoTest, InfoFailureHTTP) {
  info_operation_->Start();

  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                            net::OK));
  fetcher->set_response_code(404);

  EXPECT_CALL(info_callback_, OnPrivetJSONDoneInternal());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

class PrivetRegisterTest : public PrivetHTTPTest {
 public:
  void SetUp() override {
    info_operation_ = privet_client_->CreateInfoOperation(
        info_callback_.callback());
    register_operation_ =
        privet_client_->CreateRegisterOperation("example@google.com",
                                                &register_delegate_);
  }

 protected:
  bool SuccessfulResponseToURL(const GURL& url,
                               const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher) {
      ADD_FAILURE();
      return false;
    }

    EXPECT_EQ(url, fetcher->GetOriginalURL());
    if (url != fetcher->GetOriginalURL())
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                              net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

  std::unique_ptr<PrivetJSONOperation> info_operation_;
  NiceMock<MockJSONCallback> info_callback_;
  std::unique_ptr<PrivetRegisterOperation> register_operation_;
  StrictMock<MockRegisterDelegate> register_delegate_;
};

INSTANTIATE_TEST_CASE_P(PrivetTests, PrivetRegisterTest, ValuesIn(kTestParams));

TEST_P(PrivetRegisterTest, RegisterSuccessSimple) {
  register_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterStartResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterClaimTokenInternal(
      "MySampleToken",
      GURL("https://domain.com/SoMeUrL")));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GetUrl("/privet/register?"
             "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterGetClaimTokenResponse));

  register_operation_->CompleteRegistration();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GetUrl("/privet/register?"
             "action=complete&user=example%40google.com"),
      kSampleRegisterCompleteResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterDoneInternal(
      "MyDeviceID"));

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseRegistered));
}

TEST_P(PrivetRegisterTest, RegisterXSRFFailure) {
  register_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterStartResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GetUrl("/privet/register?"
             "action=getClaimToken&user=example%40google.com"),
      kSampleXPrivetErrorResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterClaimTokenInternal(
      "MySampleToken", GURL("https://domain.com/SoMeUrL")));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GetUrl("/privet/register?"
             "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterGetClaimTokenResponse));
}

TEST_P(PrivetRegisterTest, TransientFailure) {
  register_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterErrorTransient));

  EXPECT_CALL(fetcher_delegate_, OnRequestStart(0));

  RunFor(base::TimeDelta::FromSeconds(2));

  testing::Mock::VerifyAndClearExpectations(&fetcher_delegate_);

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterStartResponse));
}

TEST_P(PrivetRegisterTest, PermanentFailure) {
  register_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterStartResponse));

  EXPECT_CALL(register_delegate_,
              OnPrivetRegisterErrorInternal(
                  "getClaimToken",
                  PrivetRegisterOperation::FAILURE_JSON_ERROR,
                  200));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GetUrl("/privet/register?"
             "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterErrorPermanent));
}

TEST_P(PrivetRegisterTest, InfoFailure) {
  register_operation_->Start();

  EXPECT_CALL(register_delegate_,
              OnPrivetRegisterErrorInternal(
                  "start",
                  PrivetRegisterOperation::FAILURE_TOKEN,
                  -1));

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseBadJson));
}

TEST_P(PrivetRegisterTest, RegisterCancel) {
  register_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=start&user=example%40google.com"),
                              kSampleRegisterStartResponse));

  register_operation_->Cancel();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/register?"
                                     "action=cancel&user=example%40google.com"),
                              kSampleRegisterCancelResponse));

  // Must keep mocks alive for 3 seconds so the cancelation object can be
  // deleted.
  RunFor(base::TimeDelta::FromSeconds(3));
}

class PrivetCapabilitiesTest : public PrivetHTTPTest {
 public:
  void SetUp() override {
    capabilities_operation_ = privet_client_->CreateCapabilitiesOperation(
        capabilities_callback_.callback());
  }

 protected:
  std::unique_ptr<PrivetJSONOperation> capabilities_operation_;
  StrictMock<MockJSONCallback> capabilities_callback_;
};

INSTANTIATE_TEST_CASE_P(PrivetTests,
                        PrivetCapabilitiesTest,
                        ValuesIn(kTestParams));

TEST_P(PrivetCapabilitiesTest, SuccessfulCapabilities) {
  capabilities_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/capabilities"),
                                      kSampleCapabilitiesResponse));

  std::string version;
  EXPECT_TRUE(capabilities_callback_.value()->GetString("version", &version));
  EXPECT_EQ("1.0", version);
}

TEST_P(PrivetCapabilitiesTest, CacheToken) {
  capabilities_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/capabilities"),
                                      kSampleCapabilitiesResponse));

  capabilities_operation_ = privet_client_->CreateCapabilitiesOperation(
      capabilities_callback_.callback());

  capabilities_operation_->Start();

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/capabilities"),
                                      kSampleCapabilitiesResponse));
}

TEST_P(PrivetCapabilitiesTest, BadToken) {
  capabilities_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/capabilities"),
                                      kSampleXPrivetErrorResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/capabilities"),
                                      kSampleCapabilitiesResponse));
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
// A note on PWG raster conversion: The fake PWG raster converter simply returns
// the input as the converted data. The output isn't checked anyway.
// converts strings to file paths based on them by appending "test.pdf", since
// it's easier to test that way. Instead of using a mock, we simply check if the
// request is uploading a file that is based on this pattern.
class FakePwgRasterConverter : public printing::PwgRasterConverter {
 public:
  void Start(const base::RefCountedMemory* data,
             const printing::PdfRenderSettings& conversion_settings,
             const printing::PwgRasterSettings& bitmap_settings,
             ResultCallback callback) override {
    base::MappedReadOnlyRegion memory =
        base::ReadOnlySharedMemoryRegion::Create(data->size());
    if (!memory.IsValid()) {
      ADD_FAILURE() << "Failed to create pwg raster shared memory.";
      std::move(callback).Run(base::ReadOnlySharedMemoryRegion());
      return;
    }

    memcpy(memory.mapping.memory(), data->front(), data->size());
    bitmap_settings_ = bitmap_settings;
    std::move(callback).Run(std::move(memory.region));
  }

  const printing::PwgRasterSettings& bitmap_settings() {
    return bitmap_settings_;
  }

 private:
  printing::PwgRasterSettings bitmap_settings_;
};

class PrivetLocalPrintTest : public PrivetHTTPTest {
 public:
  void SetUp() override {
    PrivetURLFetcher::ResetTokenMapForTest();

    local_print_operation_ = privet_client_->CreateLocalPrintOperation(
        &local_print_delegate_);

    auto pwg_converter = std::make_unique<FakePwgRasterConverter>();
    pwg_converter_ = pwg_converter.get();
    local_print_operation_->SetPwgRasterConverterForTesting(
        std::move(pwg_converter));
  }

  scoped_refptr<base::RefCountedBytes> RefCountedBytesFromString(
      base::StringPiece str) {
    std::vector<unsigned char> str_vec;
    str_vec.insert(str_vec.begin(), str.begin(), str.end());
    return base::RefCountedBytes::TakeVector(&str_vec);
  }

 protected:
  std::unique_ptr<PrivetLocalPrintOperation> local_print_operation_;
  StrictMock<MockLocalPrintDelegate> local_print_delegate_;
  FakePwgRasterConverter* pwg_converter_;
};

INSTANTIATE_TEST_CASE_P(PrivetTests,
                        PrivetLocalPrintTest,
                        ValuesIn(kTestParams));

TEST_P(PrivetLocalPrintTest, SuccessfulLocalPrint) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString(
      "Sample print data"));
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponse);
  local_print_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name"),
      "Sample print data", kSampleLocalPrintResponse));
}

TEST_P(PrivetLocalPrintTest, SuccessfulLocalPrintWithAnyMimetype) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->SetCapabilities(
      kSampleCapabilitiesResponseWithAnyMimetype);
  local_print_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name"),
      "Sample print data", kSampleLocalPrintResponse));
}

TEST_P(PrivetLocalPrintTest, SuccessfulPWGLocalPrint) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString("foobar"));
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponsePWGOnly);
  local_print_operation_->Start();

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFileData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com"
             "&job_name=Sample+job+name"),
      "foobar", kSampleLocalPrintResponse));

  EXPECT_EQ(printing::TRANSFORM_NORMAL,
            pwg_converter_->bitmap_settings().odd_page_transform);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().rotate_all_pages);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().reverse_page_order);

  // Defaults to true when the color is not specified.
  EXPECT_TRUE(pwg_converter_->bitmap_settings().use_color);
}

TEST_P(PrivetLocalPrintTest, SuccessfulPWGLocalPrintDuplex) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString("foobar"));
  local_print_operation_->SetTicket(kSampleCJTDuplex);
  local_print_operation_->SetCapabilities(
      kSampleCapabilitiesResponsePWGSettings);
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndJSONData(
      GetUrl("/privet/printer/createjob"), kSampleCJTDuplex,
      kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFileData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com"
             "&job_name=Sample+job+name&job_id=1234"),
      "foobar", kSampleLocalPrintResponse));

  EXPECT_EQ(printing::TRANSFORM_ROTATE_180,
            pwg_converter_->bitmap_settings().odd_page_transform);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().rotate_all_pages);
  EXPECT_TRUE(pwg_converter_->bitmap_settings().reverse_page_order);

  // Defaults to true when the color is not specified.
  EXPECT_TRUE(pwg_converter_->bitmap_settings().use_color);
}

TEST_P(PrivetLocalPrintTest, SuccessfulPWGLocalPrintMono) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString("foobar"));
  local_print_operation_->SetTicket(kSampleCJTMono);
  local_print_operation_->SetCapabilities(
      kSampleCapabilitiesResponsePWGSettings);
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndJSONData(
      GetUrl("/privet/printer/createjob"), kSampleCJTMono,
      kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFileData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com"
             "&job_name=Sample+job+name&job_id=1234"),
      "foobar", kSampleLocalPrintResponse));

  EXPECT_EQ(printing::TRANSFORM_NORMAL,
            pwg_converter_->bitmap_settings().odd_page_transform);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().rotate_all_pages);
  EXPECT_TRUE(pwg_converter_->bitmap_settings().reverse_page_order);

  // Ticket specified mono, but no SGRAY_8 color capability.
  EXPECT_TRUE(pwg_converter_->bitmap_settings().use_color);
}

TEST_P(PrivetLocalPrintTest, SuccessfulPWGLocalPrintMonoToGRAY8Printer) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString("foobar"));
  local_print_operation_->SetTicket(kSampleCJTMono);
  local_print_operation_->SetCapabilities(
      kSampleCapabilitiesResponsePWGSettingsMono);
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndJSONData(
      GetUrl("/privet/printer/createjob"), kSampleCJTMono,
      kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFileData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com"
             "&job_name=Sample+job+name&job_id=1234"),
      "foobar", kSampleLocalPrintResponse));

  EXPECT_EQ(printing::TRANSFORM_NORMAL,
            pwg_converter_->bitmap_settings().odd_page_transform);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().rotate_all_pages);
  EXPECT_FALSE(pwg_converter_->bitmap_settings().reverse_page_order);

  // Ticket specified mono, and SGRAY_8 color capability exists.
  EXPECT_FALSE(pwg_converter_->bitmap_settings().use_color);
}

TEST_P(PrivetLocalPrintTest, SuccessfulLocalPrintWithCreatejob) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket(kSampleCJT);
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponse);
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURLAndJSONData(GetUrl("/privet/printer/createjob"),
                                         kSampleCJT, kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name&job_id=1234"),
      "Sample print data", kSampleLocalPrintResponse));
}

TEST_P(PrivetLocalPrintTest, SuccessfulLocalPrintWithOverlongName) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname(
      "123456789:123456789:123456789:123456789:123456789:123456789:123456789:");
  local_print_operation_->SetTicket(kSampleCJT);
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponse);
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURLAndJSONData(GetUrl("/privet/printer/createjob"),
                                         kSampleCJT, kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=123456789%3A123456789%3A123456789%3A1...123456789"
             "%3A123456789%3A123456789%3A&job_id=1234"),
      "Sample print data", kSampleLocalPrintResponse));
}

TEST_P(PrivetLocalPrintTest, PDFPrintInvalidDocumentTypeRetry) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket(kSampleCJT);
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponse);
  local_print_operation_->SetData(RefCountedBytesFromString("sample_data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURLAndJSONData(GetUrl("/privet/printer/createjob"),
                                         kSampleCJT, kSampleCreatejobResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name&job_id=1234"),
      "sample_data", kSampleInvalidDocumentTypeResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFileData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name&job_id=1234"),
      "sample_data", kSampleLocalPrintResponse));
}

TEST_P(PrivetLocalPrintTest, LocalPrintRetryOnInvalidJobID) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket(kSampleCJT);
  local_print_operation_->SetCapabilities(kSampleCapabilitiesResponse);
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/info"),
                                      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(
      SuccessfulResponseToURL(GetUrl("/privet/info"), kSampleInfoResponse));

  EXPECT_TRUE(
      SuccessfulResponseToURLAndJSONData(GetUrl("/privet/printer/createjob"),
                                         kSampleCJT, kSampleCreatejobResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GetUrl("/privet/printer/submitdoc?"
             "client_name=Chrome&user_name=sample%40gmail.com&"
             "job_name=Sample+job+name&job_id=1234"),
      "Sample print data", kSampleErrorResponsePrinterBusy));

  RunFor(base::TimeDelta::FromSeconds(3));

  EXPECT_TRUE(SuccessfulResponseToURL(GetUrl("/privet/printer/createjob"),
                                      kSampleCreatejobResponse));
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

class PrivetHttpWithServerTest : public ::testing::Test,
                                 public PrivetURLFetcher::Delegate {
 protected:
  PrivetHttpWithServerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::REAL_IO_THREAD) {}

  void SetUp() override {
    context_getter_ = base::MakeRefCounted<net::TestURLRequestContextGetter>(
        BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));

    server_ =
        std::make_unique<EmbeddedTestServer>(EmbeddedTestServer::TYPE_HTTP);

    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir));
    server_->ServeFilesFromDirectory(
        test_data_dir.Append(FILE_PATH_LITERAL("chrome/test/data")));
    ASSERT_TRUE(server_->Start());

    client_ = std::make_unique<PrivetHTTPClientImpl>(
        "test", server_->host_port_pair(), context_getter_);
  }

  void OnNeedPrivetToken(PrivetURLFetcher::TokenCallback callback) override {
    std::move(callback).Run("abc");
  }

  void OnError(int response_code, PrivetURLFetcher::ErrorType error) override {
    done_ = true;
    success_ = false;
    error_ = error;

    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, quit_);
  }

  void OnParsedJson(int response_code,
                    const base::DictionaryValue& value,
                    bool has_error) override {
    NOTREACHED();
  }

  bool OnRawData(bool response_is_file,
                 const std::string& data_string,
                 const base::FilePath& data_file) override {
    done_ = true;
    success_ = true;

    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, quit_);
    return true;
  }

  bool Run() {
    success_ = false;
    done_ = false;

    base::RunLoop run_loop;
    quit_ = run_loop.QuitClosure();

    std::unique_ptr<PrivetURLFetcher> fetcher = client_->CreateURLFetcher(
        server_->GetURL("/simple.html"), net::URLFetcher::GET, this);

    fetcher->SetMaxRetriesForTest(1);
    fetcher->Start();

    run_loop.Run();

    EXPECT_TRUE(done_);
    return success_;
  }

  bool success_ = false;
  bool done_ = false;
  PrivetURLFetcher::ErrorType error_ = PrivetURLFetcher::ErrorType();
  content::TestBrowserThreadBundle thread_bundle_;
  scoped_refptr<net::TestURLRequestContextGetter> context_getter_;
  std::unique_ptr<EmbeddedTestServer> server_;
  std::unique_ptr<PrivetHTTPClientImpl> client_;

  base::Closure quit_;
};

TEST_F(PrivetHttpWithServerTest, HttpServer) {
  EXPECT_TRUE(Run());
}

}  // namespace

}  // namespace cloud_print
