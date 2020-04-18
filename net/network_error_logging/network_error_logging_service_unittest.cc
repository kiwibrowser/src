// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/network_error_logging/network_error_logging_delegate.h"
#include "net/network_error_logging/network_error_logging_service.h"
#include "net/reporting/reporting_policy.h"
#include "net/reporting/reporting_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
namespace {

class TestReportingService : public ReportingService {
 public:
  struct Report {
    Report() = default;

    Report(Report&& other)
        : url(other.url),
          group(other.group),
          type(other.type),
          body(std::move(other.body)),
          depth(other.depth) {}

    Report(const GURL& url,
           const std::string& group,
           const std::string& type,
           std::unique_ptr<const base::Value> body,
           int depth)
        : url(url),
          group(group),
          type(type),
          body(std::move(body)),
          depth(depth) {}

    ~Report() = default;

    GURL url;
    std::string group;
    std::string type;
    std::unique_ptr<const base::Value> body;
    int depth;

   private:
    DISALLOW_COPY(Report);
  };

  TestReportingService() = default;

  const std::vector<Report>& reports() const { return reports_; }

  // ReportingService implementation:

  ~TestReportingService() override = default;

  void QueueReport(const GURL& url,
                   const std::string& group,
                   const std::string& type,
                   std::unique_ptr<const base::Value> body,
                   int depth) override {
    reports_.push_back(Report(url, group, type, std::move(body), depth));
  }

  void ProcessHeader(const GURL& url,
                     const std::string& header_value) override {
    NOTREACHED();
  }

  void RemoveBrowsingData(int data_type_mask,
                          const base::RepeatingCallback<bool(const GURL&)>&
                              origin_filter) override {
    NOTREACHED();
  }

  void RemoveAllBrowsingData(int data_type_mask) override { NOTREACHED(); }

  int GetUploadDepth(const URLRequest& request) override {
    NOTREACHED();
    return 0;
  }

  const ReportingPolicy& GetPolicy() const override {
    NOTREACHED();
    return dummy_policy_;
  }

 private:
  std::vector<Report> reports_;
  ReportingPolicy dummy_policy_;

  DISALLOW_COPY_AND_ASSIGN(TestReportingService);
};

class NetworkErrorLoggingServiceTest : public ::testing::Test {
 protected:
  NetworkErrorLoggingServiceTest() {
    service_ = NetworkErrorLoggingService::Create(
        NetworkErrorLoggingDelegate::Create());
    CreateReportingService();
  }

  void CreateReportingService() {
    DCHECK(!reporting_service_);

    reporting_service_ = std::make_unique<TestReportingService>();
    service_->SetReportingService(reporting_service_.get());
  }

  void DestroyReportingService() {
    DCHECK(reporting_service_);

    service_->SetReportingService(nullptr);
    reporting_service_.reset();
  }

  NetworkErrorLoggingService::RequestDetails
  MakeRequestDetails(GURL url, Error error_type, int status_code = 0) {
    NetworkErrorLoggingService::RequestDetails details;

    details.uri = url;
    details.referrer = kReferrer_;
    details.server_ip = IPAddress::IPv4AllZeros();
    details.status_code = status_code;
    details.elapsed_time = base::TimeDelta::FromSeconds(1);
    details.type = error_type;
    details.reporting_upload_depth = 0;

    return details;
  }

  NetworkErrorLoggingService* service() { return service_.get(); }
  const std::vector<TestReportingService::Report>& reports() {
    return reporting_service_->reports();
  }

  const GURL kUrl_ = GURL("https://example.com/path");
  const GURL kUrlDifferentPort_ = GURL("https://example.com:4433/path");
  const GURL kUrlSubdomain_ = GURL("https://subdomain.example.com/path");
  const GURL kUrlDifferentHost_ = GURL("https://example2.com/path");

  const url::Origin kOrigin_ = url::Origin::Create(kUrl_);
  const url::Origin kOriginDifferentPort_ =
      url::Origin::Create(kUrlDifferentPort_);
  const url::Origin kOriginSubdomain_ = url::Origin::Create(kUrlSubdomain_);
  const url::Origin kOriginDifferentHost_ =
      url::Origin::Create(kUrlDifferentHost_);

  const std::string kHeader_ = "{\"report-to\":\"group\",\"max-age\":86400}";
  const std::string kHeaderIncludeSubdomains_ =
      "{\"report-to\":\"group\",\"max-age\":86400,\"include-subdomains\":true}";
  const std::string kHeaderMaxAge0_ = "{\"max-age\":0}";
  const std::string kHeaderTooLong_ =
      "{\"report-to\":\"group\",\"max-age\":86400,\"junk\":\"" +
      std::string(32 * 1024, 'a') + "\"}";
  const std::string kHeaderTooDeep_ =
      "{\"report-to\":\"group\",\"max-age\":86400,\"junk\":[[[[[[[[[[]]]]]]]]]]"
      "}";

  const std::string kGroup_ = "group";

  const std::string kType_ = NetworkErrorLoggingService::kReportType;

  const GURL kReferrer_ = GURL("https://referrer.com/");

 private:
  std::unique_ptr<NetworkErrorLoggingService> service_;
  std::unique_ptr<TestReportingService> reporting_service_;
};

void ExpectDictDoubleValue(double expected_value,
                           const base::DictionaryValue& value,
                           const std::string& key) {
  double double_value = 0.0;
  EXPECT_TRUE(value.GetDouble(key, &double_value)) << key;
  EXPECT_DOUBLE_EQ(expected_value, double_value) << key;
}

TEST_F(NetworkErrorLoggingServiceTest, CreateService) {
  // Service is created by default in the test fixture..
  EXPECT_TRUE(service());
}

TEST_F(NetworkErrorLoggingServiceTest, NoReportingService) {
  DestroyReportingService();

  service()->OnHeader(kOrigin_, kHeader_);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));
}

TEST_F(NetworkErrorLoggingServiceTest, OriginInsecure) {
  const GURL kInsecureUrl("http://insecure.com/");
  const url::Origin kInsecureOrigin = url::Origin::Create(kInsecureUrl);

  service()->OnHeader(kInsecureOrigin, kHeader_);

  service()->OnRequest(
      MakeRequestDetails(kInsecureUrl, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, NoPolicyForOrigin) {
  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, JsonTooLong) {
  service()->OnHeader(kOrigin_, kHeaderTooLong_);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, JsonTooDeep) {
  service()->OnHeader(kOrigin_, kHeaderTooDeep_);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, SuccessReportQueued) {
  static const std::string kHeaderSuccessFraction1 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"success-fraction\":1.0}";
  service()->OnHeader(kOrigin_, kHeaderSuccessFraction1);

  service()->OnRequest(MakeRequestDetails(kUrl_, OK));

  ASSERT_EQ(1u, reports().size());
  EXPECT_EQ(kUrl_, reports()[0].url);
  EXPECT_EQ(kGroup_, reports()[0].group);
  EXPECT_EQ(kType_, reports()[0].type);
  EXPECT_EQ(0, reports()[0].depth);

  const base::DictionaryValue* body;
  ASSERT_TRUE(reports()[0].body->GetAsDictionary(&body));
  base::ExpectDictStringValue(kUrl_.spec(), *body,
                              NetworkErrorLoggingService::kUriKey);
  base::ExpectDictStringValue(kReferrer_.spec(), *body,
                              NetworkErrorLoggingService::kReferrerKey);
  // TODO(juliatuttle): Extract these constants.
  ExpectDictDoubleValue(1.0, *body,
                        NetworkErrorLoggingService::kSamplingFractionKey);
  base::ExpectDictStringValue("0.0.0.0", *body,
                              NetworkErrorLoggingService::kServerIpKey);
  base::ExpectDictStringValue("", *body,
                              NetworkErrorLoggingService::kProtocolKey);
  base::ExpectDictIntegerValue(0, *body,
                               NetworkErrorLoggingService::kStatusCodeKey);
  base::ExpectDictIntegerValue(1000, *body,
                               NetworkErrorLoggingService::kElapsedTimeKey);
  base::ExpectDictStringValue("ok", *body,
                              NetworkErrorLoggingService::kTypeKey);
}

TEST_F(NetworkErrorLoggingServiceTest, FailureReportQueued) {
  static const std::string kHeaderFailureFraction1 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"failure-fraction\":1.0}";
  service()->OnHeader(kOrigin_, kHeaderFailureFraction1);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  ASSERT_EQ(1u, reports().size());
  EXPECT_EQ(kUrl_, reports()[0].url);
  EXPECT_EQ(kGroup_, reports()[0].group);
  EXPECT_EQ(kType_, reports()[0].type);
  EXPECT_EQ(0, reports()[0].depth);

  const base::DictionaryValue* body;
  ASSERT_TRUE(reports()[0].body->GetAsDictionary(&body));
  base::ExpectDictStringValue(kUrl_.spec(), *body,
                              NetworkErrorLoggingService::kUriKey);
  base::ExpectDictStringValue(kReferrer_.spec(), *body,
                              NetworkErrorLoggingService::kReferrerKey);
  // TODO(juliatuttle): Extract these constants.
  ExpectDictDoubleValue(1.0, *body,
                        NetworkErrorLoggingService::kSamplingFractionKey);
  base::ExpectDictStringValue("0.0.0.0", *body,
                              NetworkErrorLoggingService::kServerIpKey);
  base::ExpectDictStringValue("", *body,
                              NetworkErrorLoggingService::kProtocolKey);
  base::ExpectDictIntegerValue(0, *body,
                               NetworkErrorLoggingService::kStatusCodeKey);
  base::ExpectDictIntegerValue(1000, *body,
                               NetworkErrorLoggingService::kElapsedTimeKey);
  base::ExpectDictStringValue("tcp.refused", *body,
                              NetworkErrorLoggingService::kTypeKey);
}

TEST_F(NetworkErrorLoggingServiceTest, HttpErrorReportQueued) {
  static const std::string kHeaderFailureFraction1 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"failure-fraction\":1.0}";
  service()->OnHeader(kOrigin_, kHeaderFailureFraction1);

  service()->OnRequest(MakeRequestDetails(kUrl_, OK, 504));

  ASSERT_EQ(1u, reports().size());
  EXPECT_EQ(kUrl_, reports()[0].url);
  EXPECT_EQ(kGroup_, reports()[0].group);
  EXPECT_EQ(kType_, reports()[0].type);
  EXPECT_EQ(0, reports()[0].depth);

  const base::DictionaryValue* body;
  ASSERT_TRUE(reports()[0].body->GetAsDictionary(&body));
  base::ExpectDictStringValue(kUrl_.spec(), *body,
                              NetworkErrorLoggingService::kUriKey);
  base::ExpectDictStringValue(kReferrer_.spec(), *body,
                              NetworkErrorLoggingService::kReferrerKey);
  // TODO(juliatuttle): Extract these constants.
  ExpectDictDoubleValue(1.0, *body,
                        NetworkErrorLoggingService::kSamplingFractionKey);
  base::ExpectDictStringValue("0.0.0.0", *body,
                              NetworkErrorLoggingService::kServerIpKey);
  base::ExpectDictStringValue("", *body,
                              NetworkErrorLoggingService::kProtocolKey);
  base::ExpectDictIntegerValue(504, *body,
                               NetworkErrorLoggingService::kStatusCodeKey);
  base::ExpectDictIntegerValue(1000, *body,
                               NetworkErrorLoggingService::kElapsedTimeKey);
  base::ExpectDictStringValue("http.error", *body,
                              NetworkErrorLoggingService::kTypeKey);
}

TEST_F(NetworkErrorLoggingServiceTest, MaxAge0) {
  service()->OnHeader(kOrigin_, kHeader_);

  service()->OnHeader(kOrigin_, kHeaderMaxAge0_);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, SuccessFraction0) {
  static const std::string kHeaderSuccessFraction0 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"success-fraction\":0.0}";
  service()->OnHeader(kOrigin_, kHeaderSuccessFraction0);

  // Each network error has a 0% chance of being reported.  Fire off several and
  // verify that no reports are produced.
  constexpr size_t kReportCount = 100;
  for (size_t i = 0; i < kReportCount; ++i)
    service()->OnRequest(MakeRequestDetails(kUrl_, OK));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, SuccessFractionHalf) {
  // Include a different value for failure-fraction to ensure that we copy the
  // right value into sampling-fraction.
  static const std::string kHeaderSuccessFractionHalf =
      "{\"report-to\":\"group\",\"max-age\":86400,\"success-fraction\":0.5,"
      "\"failure-fraction\":0.25}";
  service()->OnHeader(kOrigin_, kHeaderSuccessFractionHalf);

  // Each network error has a 50% chance of being reported.  Fire off several
  // and verify that some requests were reported and some weren't.  (We can't
  // verify exact counts because each decision is made randomly.)
  constexpr size_t kReportCount = 100;
  for (size_t i = 0; i < kReportCount; ++i)
    service()->OnRequest(MakeRequestDetails(kUrl_, OK));

  // If our random selection logic is correct, there is a 2^-100 chance that
  // every single report above was skipped.  If this check fails, it's much more
  // likely that our code is wrong.
  EXPECT_FALSE(reports().empty());

  // There's also a 2^-100 chance that every single report was logged.  Same as
  // above, that's much more likely to be a code error.
  EXPECT_GT(kReportCount, reports().size());

  for (const auto& report : reports()) {
    const base::DictionaryValue* body;
    ASSERT_TRUE(report.body->GetAsDictionary(&body));
    // Our header includes a different value for failure-fraction, so that this
    // check verifies that we copy the correct fraction into sampling-fraction.
    ExpectDictDoubleValue(0.5, *body,
                          NetworkErrorLoggingService::kSamplingFractionKey);
  }
}

TEST_F(NetworkErrorLoggingServiceTest, FailureFraction0) {
  static const std::string kHeaderFailureFraction0 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"failure-fraction\":0.0}";
  service()->OnHeader(kOrigin_, kHeaderFailureFraction0);

  // Each network error has a 0% chance of being reported.  Fire off several and
  // verify that no reports are produced.
  constexpr size_t kReportCount = 100;
  for (size_t i = 0; i < kReportCount; ++i)
    service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, FailureFractionHalf) {
  // Include a different value for success-fraction to ensure that we copy the
  // right value into sampling-fraction.
  static const std::string kHeaderFailureFractionHalf =
      "{\"report-to\":\"group\",\"max-age\":86400,\"failure-fraction\":0.5,"
      "\"success-fraction\":0.25}";
  service()->OnHeader(kOrigin_, kHeaderFailureFractionHalf);

  // Each network error has a 50% chance of being reported.  Fire off several
  // and verify that some requests were reported and some weren't.  (We can't
  // verify exact counts because each decision is made randomly.)
  constexpr size_t kReportCount = 100;
  for (size_t i = 0; i < kReportCount; ++i)
    service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  // If our random selection logic is correct, there is a 2^-100 chance that
  // every single report above was skipped.  If this check fails, it's much more
  // likely that our code is wrong.
  EXPECT_FALSE(reports().empty());

  // There's also a 2^-100 chance that every single report was logged.  Same as
  // above, that's much more likely to be a code error.
  EXPECT_GT(kReportCount, reports().size());

  for (const auto& report : reports()) {
    const base::DictionaryValue* body;
    ASSERT_TRUE(report.body->GetAsDictionary(&body));
    ExpectDictDoubleValue(0.5, *body,
                          NetworkErrorLoggingService::kSamplingFractionKey);
  }
}

TEST_F(NetworkErrorLoggingServiceTest,
       ExcludeSubdomainsDoesntMatchDifferentPort) {
  service()->OnHeader(kOrigin_, kHeader_);

  service()->OnRequest(
      MakeRequestDetails(kUrlDifferentPort_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, ExcludeSubdomainsDoesntMatchSubdomain) {
  service()->OnHeader(kOrigin_, kHeader_);

  service()->OnRequest(
      MakeRequestDetails(kUrlSubdomain_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, IncludeSubdomainsMatchesDifferentPort) {
  service()->OnHeader(kOrigin_, kHeaderIncludeSubdomains_);

  service()->OnRequest(
      MakeRequestDetails(kUrlDifferentPort_, ERR_CONNECTION_REFUSED));

  ASSERT_EQ(1u, reports().size());
  EXPECT_EQ(kUrlDifferentPort_, reports()[0].url);
}

TEST_F(NetworkErrorLoggingServiceTest, IncludeSubdomainsMatchesSubdomain) {
  service()->OnHeader(kOrigin_, kHeaderIncludeSubdomains_);

  service()->OnRequest(
      MakeRequestDetails(kUrlSubdomain_, ERR_CONNECTION_REFUSED));

  ASSERT_EQ(1u, reports().size());
}

TEST_F(NetworkErrorLoggingServiceTest,
       IncludeSubdomainsDoesntMatchSuperdomain) {
  service()->OnHeader(kOriginSubdomain_, kHeaderIncludeSubdomains_);

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, RemoveAllBrowsingData) {
  service()->OnHeader(kOrigin_, kHeader_);

  service()->RemoveAllBrowsingData();

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, RemoveSomeBrowsingData) {
  service()->OnHeader(kOrigin_, kHeader_);
  service()->OnHeader(kOriginDifferentHost_, kHeader_);

  service()->RemoveBrowsingData(
      base::BindRepeating([](const GURL& origin) -> bool {
        return origin.host() == "example.com";
      }));

  service()->OnRequest(MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED));

  EXPECT_TRUE(reports().empty());

  service()->OnRequest(
      MakeRequestDetails(kUrlDifferentHost_, ERR_CONNECTION_REFUSED));

  ASSERT_EQ(1u, reports().size());
}

TEST_F(NetworkErrorLoggingServiceTest, Nested) {
  service()->OnHeader(kOrigin_, kHeader_);

  NetworkErrorLoggingService::RequestDetails details =
      MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED);
  details.reporting_upload_depth =
      NetworkErrorLoggingService::kMaxNestedReportDepth;
  service()->OnRequest(details);

  ASSERT_EQ(1u, reports().size());
  EXPECT_EQ(NetworkErrorLoggingService::kMaxNestedReportDepth,
            reports()[0].depth);
}

TEST_F(NetworkErrorLoggingServiceTest, NestedTooDeep) {
  service()->OnHeader(kOrigin_, kHeader_);

  NetworkErrorLoggingService::RequestDetails details =
      MakeRequestDetails(kUrl_, ERR_CONNECTION_REFUSED);
  details.reporting_upload_depth =
      NetworkErrorLoggingService::kMaxNestedReportDepth + 1;
  service()->OnRequest(details);

  EXPECT_TRUE(reports().empty());
}

TEST_F(NetworkErrorLoggingServiceTest, StatusAsValue) {
  base::SimpleTestTickClock clock;
  service()->SetTickClockForTesting(&clock);

  static const std::string kHeaderSuccessFraction1 =
      "{\"report-to\":\"group\",\"max-age\":86400,\"success-fraction\":1.0}";
  service()->OnHeader(kOrigin_, kHeaderSuccessFraction1);
  service()->OnHeader(kOriginDifferentHost_, kHeader_);
  service()->OnHeader(kOriginSubdomain_, kHeaderIncludeSubdomains_);

  base::Value actual = service()->StatusAsValue();
  std::unique_ptr<base::Value> expected = base::test::ParseJson(R"json(
      {
        "originPolicies": [
          {
            "origin": "https://example.com",
            "includeSubdomains": false,
            "expires": "86400000",
            "reportTo": "group",
            "successFraction": 1.0,
            "failureFraction": 1.0,
          },
          {
            "origin": "https://example2.com",
            "includeSubdomains": false,
            "expires": "86400000",
            "reportTo": "group",
            "successFraction": 0.0,
            "failureFraction": 1.0,
          },
          {
            "origin": "https://subdomain.example.com",
            "includeSubdomains": true,
            "expires": "86400000",
            "reportTo": "group",
            "successFraction": 0.0,
            "failureFraction": 1.0,
          },
        ]
      }
      )json");
  EXPECT_EQ(*expected, actual);
}

}  // namespace
}  // namespace net
