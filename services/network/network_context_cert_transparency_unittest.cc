// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/test/gtest_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "components/certificate_transparency/features.h"
#include "components/certificate_transparency/single_tree_tracker.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/ct_policy_status.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_tree_head.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/test/cert_test_util.h"
#include "net/test/ct_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/ct_log_info.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/proxy_config.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

mojom::NetworkContextParamsPtr CreateContextParams() {
  mojom::NetworkContextParamsPtr params = mojom::NetworkContextParams::New();

  // Use a fixed proxy config, to avoid dependencies on local network
  // configuration.
  params->initial_proxy_config = net::ProxyConfigWithAnnotation::CreateDirect();

  // Configure Certificate Transparency for the context.
  // TODO(robpercival): https://crbug.com/839612 - Use test logs for
  // integration tests rather than production logs.
  const char kPilotKey[] =
      "\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86"
      "\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x7d\xa8\x4b\x12\x29\x80\xa3"
      "\x3d\xad\xd3\x5a\x77\xb8\xcc\xe2\x88\xb3\xa5\xfd\xf1\xd3\x0c\xcd\x18"
      "\x0c\xe8\x41\x46\xe8\x81\x01\x1b\x15\xe1\x4b\xf1\x1b\x62\xdd\x36\x0a"
      "\x08\x18\xba\xed\x0b\x35\x84\xd0\x9e\x40\x3c\x2d\x9e\x9b\x82\x65\xbd"
      "\x1f\x04\x10\x41\x4c\xa0";
  const char kAviatorKey[] =
      "\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86"
      "\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\xd7\xf4\xcc\x69\xb2\xe4\x0e"
      "\x90\xa3\x8a\xea\x5a\x70\x09\x4f\xef\x13\x62\xd0\x8d\x49\x60\xff\x1b"
      "\x40\x50\x07\x0c\x6d\x71\x86\xda\x25\x49\x8d\x65\xe1\x08\x0d\x47\x34"
      "\x6b\xbd\x27\xbc\x96\x21\x3e\x34\xf5\x87\x76\x31\xb1\x7f\x1d\xc9\x85"
      "\x3b\x0d\xf7\x1f\x3f\xe9";
  const char kDigiCertKey[] =
      "\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86"
      "\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x02\x46\xc5\xbe\x1b\xbb\x82"
      "\x40\x16\xe8\xc1\xd2\xac\x19\x69\x13\x59\xf8\xf8\x70\x85\x46\x40\xb9"
      "\x38\xb0\x23\x82\xa8\x64\x4c\x7f\xbf\xbb\x34\x9f\x4a\x5f\x28\x8a\xcf"
      "\x19\xc4\x00\xf6\x36\x06\x93\x65\xed\x4c\xf5\xa9\x21\x62\x5a\xd8\x91"
      "\xeb\x38\x24\x40\xac\xe8";
  params->ct_logs.push_back(network::mojom::CTLogInfo::New(
      std::string(kPilotKey, base::size(kPilotKey) - 1), "Google 'Pilot' Log",
      "pilot.ct.invalid"));
  params->ct_logs.push_back(network::mojom::CTLogInfo::New(
      std::string(kAviatorKey, base::size(kAviatorKey) - 1),
      "Google 'Aviator' Log", "aviator.ct.invalid"));
  params->ct_logs.push_back(network::mojom::CTLogInfo::New(
      std::string(kDigiCertKey, base::size(kDigiCertKey) - 1),
      "DigiCert Log Server", "digicert.ct.invalid"));

  return params;
}

// TODO(robpercival): https://crbug.com/839612 - Make it easier to use a test
// cert that is not so tightly-coupled to production logs and STHs.
scoped_refptr<net::X509Certificate> GetCTCertForTesting() {
  base::ScopedAllowBlockingForTesting allow_blocking_for_loading_cert;
  return net::CreateCertificateChainFromFile(
      net::GetTestCertsDirectory(), "comodo-chain.pem",
      net::X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
}

// The number of valid SCTs in |GetCTCertForTesting| from logs configured in
// |CreateContextParams()|.
const int kNumSCTs = 3;

// Decodes a base64-encoded "DigitallySigned" TLS struct into |*sig_out|.
// See https://tools.ietf.org/html/rfc5246#section-4.7.
// |sig_out| must not be null.
bool DecodeDigitallySigned(base::StringPiece base64_data,
                           net::ct::DigitallySigned* sig_out) {
  std::string data;
  if (!base::Base64Decode(base64_data, &data))
    return false;

  base::StringPiece data_ptr = data;
  if (!net::ct::DecodeDigitallySigned(&data_ptr, sig_out))
    return false;

  return true;
}

// Populates |*sth_out| with the given information.
// |sth_out| must not be null.
bool BuildSignedTreeHead(base::Time timestamp,
                         uint64_t tree_size,
                         base::StringPiece root_hash_base64,
                         base::StringPiece signature_base64,
                         base::StringPiece log_id_base64,
                         net::ct::SignedTreeHead* sth_out) {
  sth_out->version = net::ct::SignedTreeHead::V1;
  sth_out->timestamp = timestamp;
  sth_out->tree_size = tree_size;

  std::string root_hash;
  if (!base::Base64Decode(root_hash_base64, &root_hash)) {
    return false;
  }
  root_hash.copy(sth_out->sha256_root_hash, net::ct::kSthRootHashLength);

  return DecodeDigitallySigned(signature_base64, &sth_out->signature) &&
         base::Base64Decode(log_id_base64, &sth_out->log_id);
}

TEST(NetworkContextCertTransparencyAuditingDisabledTest,
     SCTsAreNotCheckedForInclusion) {
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      certificate_transparency::kCTLogAuditing);

  std::unique_ptr<NetworkService> network_service =
      NetworkService::CreateForTesting();

  // Override the CertVerifier, so that a 'real' cert can be simulated being
  // returned by the net::TestServer. This should be done before creating the
  // context.
  net::MockCertVerifier mock_cert_verifier;
  NetworkContext::SetCertVerifierForTesting(&mock_cert_verifier);
  base::ScopedClosureRunner cleanup(base::BindOnce(
      [] { NetworkContext::SetCertVerifierForTesting(nullptr); }));

  mojom::NetworkContextParamsPtr context_params = CreateContextParams();
  mojom::NetworkContextPtr network_context_ptr;
  std::unique_ptr<NetworkContext> network_context =
      std::make_unique<NetworkContext>(network_service.get(),
                                       mojo::MakeRequest(&network_context_ptr),
                                       std::move(context_params));

  // Certificate Transparency should be configured, but there should be
  // nothing listening for SCTs (such as the NetworkContext's ct_tree_tracker_).
  ASSERT_TRUE(
      network_context->url_request_context()->cert_transparency_verifier());
  EXPECT_FALSE(network_context->url_request_context()
                   ->cert_transparency_verifier()
                   ->GetObserver());

  // Provide an STH from Google's Pilot log that can be used to prove
  // inclusion for an SCT later in the test.
  net::ct::SignedTreeHead pilot_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1512419914170), 181871752,
      "bvgljSy3Yg32Y6J8qL5WmUA3jn2WnOrEFDqxD0AxUvs=",
      "BAMARjBEAiAwEXve2RBk3XkUR+6nACSETTgzKFaEeginxuj5U9BI/"
      "wIgBPuQS5ACxsro6TtpY4bQyE6WlMdcSMiMd/SSGraOBOg=",
      "pLkJkLQYWBSHuxOizGdwCjw1mAT5G9+443fNDsgN3BA=", &pilot_sth));
  network_service->UpdateSignedTreeHead(pilot_sth);

  // Provide an STH from Google's Aviator log that is not recent enough to
  // prove inclusion for an SCT later in the test.
  net::ct::SignedTreeHead aviator_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1442652106945), 8502329,
      "bfG+gWZcHl9fqtNo0Z/uggs8E5YqGOtJQ0Z5zVZDRxI=",
      "BAMARjBEAiA6elcNQoShmKLHj/"
      "IA649UIbaQtWJEpj0Eot0q7G6fEgIgYChb7U6Reuvt0nO5PionH+3UciOxKV3Cy8/"
      "eq59lSYY=",
      "aPaY+B9kgr46jO65KB1M/HFRXWeT1ETRCmesu09P+8Q=", &aviator_sth));
  network_service->UpdateSignedTreeHead(aviator_sth);

  // Start a test server on "localhost" and configure connections to it to
  // simulate using a real certificate.
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  ASSERT_TRUE(https_server.Start());

  // Configure "localhost" to be treated as if it went through DNS. This
  // modifies the HostCache directly to simulate it being cached, rather than
  // indirecting through a scoped HostResolverProc, as queries that use
  // HostResolverProcs are treated as SOURCE_UNKNOWN, rather than SOURCE_DNS.
  net::AddressList address_list;
  ASSERT_TRUE(https_server.GetAddressList(&address_list));

  net::HostCache* host_cache =
      network_context->url_request_context()->host_resolver()->GetHostCache();
  ASSERT_TRUE(host_cache);
  host_cache->Set(
      net::HostCache::Key("localhost", net::ADDRESS_FAMILY_UNSPECIFIED, 0),
      net::HostCache::Entry(net::OK, address_list,
                            net::HostCache::Entry::SOURCE_DNS),
      base::TimeTicks::Now(), base::TimeDelta());

  // This certificate contains 3 SCTs and fulfills the Chrome CT policy.
  // Simulate it being trusted by a known root, as otherwise CT is skipped for
  // private roots.
  net::CertVerifyResult verify_result;
  verify_result.is_issued_by_known_root = true;
  verify_result.cert_status = 0;
  verify_result.verified_cert = GetCTCertForTesting();
  ASSERT_TRUE(verify_result.verified_cert);
  mock_cert_verifier.AddResultForCert(https_server.GetCertificate(),
                                      verify_result, net::OK);

  ResourceRequest request;
  request.url = https_server.GetURL("localhost", "/");

  mojom::URLLoaderFactoryPtr loader_factory;
  auto url_loader_factory_params =
      network::mojom::URLLoaderFactoryParams::New();
  url_loader_factory_params->process_id = network::mojom::kBrowserProcessId;
  url_loader_factory_params->is_corb_enabled = false;
  network_context->CreateURLLoaderFactory(mojo::MakeRequest(&loader_factory),
                                          std::move(url_loader_factory_params));

  base::HistogramTester histograms;
  mojom::URLLoaderPtr loader;
  TestURLLoaderClient client;
  loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 0 /* routing_id */, 0 /* request_id */,
      0 /* options */, request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));

  client.RunUntilResponseReceived();
  EXPECT_TRUE(client.has_received_response());
  EXPECT_TRUE(client.has_received_completion());

  // Expect only a single connection.
  ASSERT_EQ(histograms.GetBucketCount("Net.SSL_Connection_Error", net::OK), 1);

  // Expect 3 SCTs in this connection.
  EXPECT_THAT(histograms.GetBucketCount(
                  "Net.CertificateTransparency.SCTsPerConnection", kNumSCTs),
              1);

  // Expect that the SCTs were embedded in the certificate.
  EXPECT_THAT(
      histograms.GetBucketCount(
          "Net.CertificateTransparency.SCTOrigin",
          static_cast<int>(net::ct::SignedCertificateTimestamp::SCT_EMBEDDED)),
      kNumSCTs);

  // No SCTs should be eligible for inclusion checking, as inclusion checking
  // is disabled.
  histograms.ExpectTotalCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT", 0);
}

TEST(NetworkContextCertTransparencyAuditingEnabledTest,
     SCTsAreCheckedForInclusion) {
  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      certificate_transparency::kCTLogAuditing);

  std::unique_ptr<NetworkService> network_service =
      NetworkService::CreateForTesting();

  // Override the CertVerifier, so that a 'real' cert can be simulated being
  // returned by the net::TestServer. This should be done before creating the
  // context.
  net::MockCertVerifier mock_cert_verifier;
  NetworkContext::SetCertVerifierForTesting(&mock_cert_verifier);
  base::ScopedClosureRunner cleanup(base::BindOnce(
      [] { NetworkContext::SetCertVerifierForTesting(nullptr); }));

  mojom::NetworkContextParamsPtr context_params = CreateContextParams();
  mojom::NetworkContextPtr network_context_ptr;
  std::unique_ptr<NetworkContext> network_context =
      std::make_unique<NetworkContext>(network_service.get(),
                                       mojo::MakeRequest(&network_context_ptr),
                                       std::move(context_params));

  // Certificate Transparency should be configured, but there should be
  // nothing listening for SCTs (such as the NetworkContext's ct_tree_tracker_).
  ASSERT_TRUE(
      network_context->url_request_context()->cert_transparency_verifier());
  EXPECT_TRUE(network_context->url_request_context()
                  ->cert_transparency_verifier()
                  ->GetObserver());

  // Provide an STH from Google's Pilot log that can be used to prove
  // inclusion for an SCT later in the test.
  net::ct::SignedTreeHead pilot_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1512419914170), 181871752,
      "bvgljSy3Yg32Y6J8qL5WmUA3jn2WnOrEFDqxD0AxUvs=",
      "BAMARjBEAiAwEXve2RBk3XkUR+6nACSETTgzKFaEeginxuj5U9BI/"
      "wIgBPuQS5ACxsro6TtpY4bQyE6WlMdcSMiMd/SSGraOBOg=",
      "pLkJkLQYWBSHuxOizGdwCjw1mAT5G9+443fNDsgN3BA=", &pilot_sth));
  network_service->UpdateSignedTreeHead(pilot_sth);

  // Provide an STH from Google's Aviator log that is not recent enough to
  // prove inclusion for an SCT later in the test.
  net::ct::SignedTreeHead aviator_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1442652106945), 8502329,
      "bfG+gWZcHl9fqtNo0Z/uggs8E5YqGOtJQ0Z5zVZDRxI=",
      "BAMARjBEAiA6elcNQoShmKLHj/"
      "IA649UIbaQtWJEpj0Eot0q7G6fEgIgYChb7U6Reuvt0nO5PionH+3UciOxKV3Cy8/"
      "eq59lSYY=",
      "aPaY+B9kgr46jO65KB1M/HFRXWeT1ETRCmesu09P+8Q=", &aviator_sth));
  network_service->UpdateSignedTreeHead(aviator_sth);

  // Start a test server on "localhost" and configure connections to it to
  // simulate using a real certificate.
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  ASSERT_TRUE(https_server.Start());

  // Configure "localhost" to be treated as if it went through DNS. This
  // modifies the HostCache directly to simulate it being cached, rather than
  // indirecting through a scoped HostResolverProc, as queries that use
  // HostResolverProcs are treated as SOURCE_UNKNOWN, rather than SOURCE_DNS.
  net::AddressList address_list;
  ASSERT_TRUE(https_server.GetAddressList(&address_list));

  net::HostCache* host_cache =
      network_context->url_request_context()->host_resolver()->GetHostCache();
  ASSERT_TRUE(host_cache);
  host_cache->Set(
      net::HostCache::Key("localhost", net::ADDRESS_FAMILY_UNSPECIFIED, 0),
      net::HostCache::Entry(net::OK, address_list,
                            net::HostCache::Entry::SOURCE_DNS),
      base::TimeTicks::Now(), base::TimeDelta());

  // This certificate contains 3 SCTs and fulfills the Chrome CT policy.
  // Simulate it being trusted by a known root, as otherwise CT is skipped for
  // private roots.
  net::CertVerifyResult verify_result;
  verify_result.is_issued_by_known_root = true;
  verify_result.cert_status = 0;
  verify_result.verified_cert = GetCTCertForTesting();
  ASSERT_TRUE(verify_result.verified_cert);
  mock_cert_verifier.AddResultForCert(https_server.GetCertificate(),
                                      verify_result, net::OK);

  base::HistogramTester histograms;

  ResourceRequest request;
  request.url = https_server.GetURL("localhost", "/");

  mojom::URLLoaderFactoryPtr loader_factory;
  auto url_loader_factory_params =
      network::mojom::URLLoaderFactoryParams::New();
  url_loader_factory_params->process_id = network::mojom::kBrowserProcessId;
  url_loader_factory_params->is_corb_enabled = false;
  network_context->CreateURLLoaderFactory(mojo::MakeRequest(&loader_factory),
                                          std::move(url_loader_factory_params));

  mojom::URLLoaderPtr loader;
  TestURLLoaderClient client;
  loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 0 /* routing_id */, 0 /* request_id */,
      0 /* options */, request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));

  client.RunUntilResponseReceived();
  EXPECT_TRUE(client.has_received_response());
  EXPECT_TRUE(client.has_received_completion());

  // Expect only a single connection.
  ASSERT_EQ(histograms.GetBucketCount("Net.SSL_Connection_Error", net::OK), 1);

  // Expect 3 SCTs in this connection.
  EXPECT_THAT(histograms.GetBucketCount(
                  "Net.CertificateTransparency.SCTsPerConnection", kNumSCTs),
              1);

  // Expect that the SCTs were embedded in the certificate.
  EXPECT_THAT(
      histograms.GetBucketCount(
          "Net.CertificateTransparency.SCTOrigin",
          static_cast<int>(net::ct::SignedCertificateTimestamp::SCT_EMBEDDED)),
      kNumSCTs);

  // The Pilot SCT should be eligible for inclusion checking, because a recent
  // enough Pilot STH is available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::CAN_BE_CHECKED, 1);
  // The Aviator SCT should not be eligible for inclusion checking, because
  // there is not a recent enough Aviator STH available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::NEWER_STH_REQUIRED, 1);
  // The DigiCert SCT should not be eligible for inclusion checking, because
  // there is no DigiCert STH available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::VALID_STH_REQUIRED, 1);
}

}  // namespace

}  // namespace network
