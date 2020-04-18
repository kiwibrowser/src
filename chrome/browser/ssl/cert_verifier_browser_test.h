// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_CERT_VERIFIER_BROWSER_TEST_H_
#define CHROME_BROWSER_SSL_CERT_VERIFIER_BROWSER_TEST_H_

#include <memory>

#include "chrome/test/base/in_process_browser_test.h"
#include "net/cert/mock_cert_verifier.h"
#include "services/network/public/mojom/network_service_test.mojom.h"

namespace net {
class MockCertVerifier;
}  // namespace net

// CertVerifierBrowserTest allows tests to force certificate
// verification results for requests made with any profile's main
// request context (such as navigations). To do so, tests can use the
// MockCertVerifier exposed via
// CertVerifierBrowserTest::mock_cert_verifier().
class CertVerifierBrowserTest : public InProcessBrowserTest {
 public:
  CertVerifierBrowserTest();
  ~CertVerifierBrowserTest() override;

  // InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void TearDownInProcessBrowserTestFixture() override;

  // Has the same methods as net::MockCertVerifier and updates the network
  // service as well if it's in use. See the documentation of the net class
  // for documentation on the methods.
  // Once all requests use the NetworkContext even when network service is not
  // enabled, we can stop also updating net::MockCertVerifier here and always
  // go through the NetworkServiceTest mojo interface.
  class CertVerifier {
   public:
    explicit CertVerifier(net::MockCertVerifier* verifier);
    ~CertVerifier();
    void set_default_result(int default_result);
    void AddResultForCert(scoped_refptr<net::X509Certificate> cert,
                          const net::CertVerifyResult& verify_result,
                          int rv);
    void AddResultForCertAndHost(scoped_refptr<net::X509Certificate> cert,
                                 const std::string& host_pattern,
                                 const net::CertVerifyResult& verify_result,
                                 int rv);

   private:
    void EnsureNetworkServiceTestInitialized();

    net::MockCertVerifier* verifier_;
    network::mojom::NetworkServiceTestPtr network_service_test_;
  };

  // Returns a pointer to the MockCertVerifier used by all profiles in
  // this test.
  CertVerifier* mock_cert_verifier();

 private:
  std::unique_ptr<net::MockCertVerifier> mock_cert_verifier_;

  CertVerifier cert_verifier_;
};

#endif  // CHROME_BROWSER_SSL_CERT_VERIFIER_BROWSER_TEST_H_
