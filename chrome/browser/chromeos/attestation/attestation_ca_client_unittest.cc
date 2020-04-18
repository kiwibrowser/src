// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/attestation/attestation_ca_client.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace attestation {

class AttestationCAClientTest : public ::testing::Test {
 public:
  AttestationCAClientTest() : num_invocations_(0), result_(false) {}

  ~AttestationCAClientTest() override {}

  void DataCallback(bool result, const std::string& data) {
    ++num_invocations_;
    result_ = result;
    data_ = data;
  }

  void DeleteClientDataCallback(AttestationCAClient* client,
                                bool result,
                                const std::string& data) {
    delete client;
    DataCallback(result, data);
  }

 protected:
  void CheckURLAndSendResponse(GURL expected_url,
                               net::Error error,
                               int response_code) {
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    CHECK(fetcher);
    EXPECT_EQ(expected_url, fetcher->GetOriginalURL());
    SendFetcherResponse(fetcher, error, response_code);
  }

  void SendResponse(net::Error error, int response_code) {
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    CHECK(fetcher);
    SendFetcherResponse(fetcher, error, response_code);
  }

  void SendFetcherResponse(net::TestURLFetcher* fetcher,
                           net::Error error,
                           int response_code) {
    fetcher->set_status(net::URLRequestStatus::FromError(error));
    fetcher->set_response_code(response_code);
    fetcher->SetResponseString(fetcher->upload_data() + "_response");
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  net::TestURLFetcherFactory url_fetcher_factory_;

  // For use with DataCallback.
  int num_invocations_;
  bool result_;
  std::string data_;
};

TEST_F(AttestationCAClientTest, EnrollRequest) {
  AttestationCAClient client;
  client.SendEnrollRequest(
      "enroll",
      base::Bind(&AttestationCAClientTest::DataCallback,
                 base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://chromeos-ca.gstatic.com/enroll"),
                          net::OK, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("enroll_response", data_);
}

TEST_F(AttestationCAClientTest, CertificateRequest) {
  AttestationCAClient client;
  client.SendCertificateRequest(
      "certificate",
      base::Bind(&AttestationCAClientTest::DataCallback,
                 base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://chromeos-ca.gstatic.com/sign"), net::OK,
                          net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("certificate_response", data_);
}

TEST_F(AttestationCAClientTest, CertificateRequestNetworkFailure) {
  AttestationCAClient client;
  client.SendCertificateRequest(
      "certificate",
      base::Bind(&AttestationCAClientTest::DataCallback,
                 base::Unretained(this)));
  SendResponse(net::ERR_FAILED, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_FALSE(result_);
  EXPECT_EQ("", data_);
}

TEST_F(AttestationCAClientTest, CertificateRequestHttpError) {
  AttestationCAClient client;
  client.SendCertificateRequest(
      "certificate",
      base::Bind(&AttestationCAClientTest::DataCallback,
                 base::Unretained(this)));
  SendResponse(net::OK, net::HTTP_NOT_FOUND);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_FALSE(result_);
  EXPECT_EQ("", data_);
}

TEST_F(AttestationCAClientTest, DeleteOnCallback) {
  AttestationCAClient* client = new AttestationCAClient();
  client->SendCertificateRequest(
      "certificate",
      base::Bind(&AttestationCAClientTest::DeleteClientDataCallback,
                 base::Unretained(this),
                 client));
  SendResponse(net::OK, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("certificate_response", data_);
}

class AttestationCAClientAttestationServerTest
    : public AttestationCAClientTest {};

TEST_F(AttestationCAClientAttestationServerTest, DefaultEnrollRequest) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      chromeos::switches::kAttestationServer, "default");
  AttestationCAClient client;
  client.SendEnrollRequest("enroll",
                           base::Bind(&AttestationCAClientTest::DataCallback,
                                      base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://chromeos-ca.gstatic.com/enroll"),
                          net::OK, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("enroll_response", data_);
}

TEST_F(AttestationCAClientAttestationServerTest, DefaultCertificateRequest) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      chromeos::switches::kAttestationServer, "default");
  AttestationCAClient client;
  client.SendCertificateRequest(
      "certificate", base::Bind(&AttestationCAClientTest::DataCallback,
                                base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://chromeos-ca.gstatic.com/sign"), net::OK,
                          net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("certificate_response", data_);
}

TEST_F(AttestationCAClientAttestationServerTest, TestEnrollRequest) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      chromeos::switches::kAttestationServer, "test");
  AttestationCAClient client;
  client.SendEnrollRequest("enroll",
                           base::Bind(&AttestationCAClientTest::DataCallback,
                                      base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://asbestos-qa.corp.google.com/enroll"),
                          net::OK, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("enroll_response", data_);
}

TEST_F(AttestationCAClientAttestationServerTest, TestCertificateRequest) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      chromeos::switches::kAttestationServer, "test");
  AttestationCAClient client;
  client.SendCertificateRequest(
      "certificate", base::Bind(&AttestationCAClientTest::DataCallback,
                                base::Unretained(this)));
  CheckURLAndSendResponse(GURL("https://asbestos-qa.corp.google.com/sign"),
                          net::OK, net::HTTP_OK);

  EXPECT_EQ(1, num_invocations_);
  EXPECT_TRUE(result_);
  EXPECT_EQ("certificate_response", data_);
}

}  // namespace attestation
}  // namespace chromeos
