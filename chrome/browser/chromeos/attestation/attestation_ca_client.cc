// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/attestation/attestation_ca_client.h"

#include <string>

#include "base/command_line.h"
#include "chrome/browser/browser_process.h"
#include "chromeos/chromeos_switches.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace {
// Values for the attestation server switch.
const char kAttestationServerDefault[] = "default";
const char kAttestationServerTest[] = "test";

// Endpoints for the default Google Privacy CA operations.
const char kDefaultEnrollRequestURL[] =
    "https://chromeos-ca.gstatic.com/enroll";
const char kDefaultCertificateRequestURL[] =
    "https://chromeos-ca.gstatic.com/sign";

// Endpoints for the test Google Privacy CA operations.
const char kTestEnrollRequestURL[] =
    "https://asbestos-qa.corp.google.com/enroll";
const char kTestCertificateRequestURL[] =
    "https://asbestos-qa.corp.google.com/sign";

const char kMimeContentType[] = "application/octet-stream";

}  // namespace

namespace chromeos {
namespace attestation {

static PrivacyCAType GetAttestationServerType() {
  std::string value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          chromeos::switches::kAttestationServer);
  if (value.empty() || value == kAttestationServerDefault) {
    return DEFAULT_PCA;
  }
  if (value == kAttestationServerTest) {
    return TEST_PCA;
  }
  LOG(WARNING) << "Invalid attestation server value: " << value
               << ". Using default.";
  return DEFAULT_PCA;
}

AttestationCAClient::AttestationCAClient() {
  pca_type_ = GetAttestationServerType();
}

AttestationCAClient::~AttestationCAClient() {}

void AttestationCAClient::SendEnrollRequest(const std::string& request,
                                            const DataCallback& on_response) {
  FetchURL(
      GetType() == TEST_PCA ? kTestEnrollRequestURL : kDefaultEnrollRequestURL,
      request, on_response);
}

void AttestationCAClient::SendCertificateRequest(
    const std::string& request,
    const DataCallback& on_response) {
  FetchURL(GetType() == TEST_PCA ? kTestCertificateRequestURL
                                 : kDefaultCertificateRequestURL,
           request, on_response);
}

void AttestationCAClient::OnURLFetchComplete(const net::URLFetcher* source) {
  FetcherCallbackMap::iterator iter = pending_requests_.find(source);
  if (iter == pending_requests_.end()) {
    LOG(WARNING) << "Callback from unknown source.";
    return;
  }

  DataCallback callback = iter->second;
  pending_requests_.erase(iter);
  std::unique_ptr<const net::URLFetcher> scoped_source(source);

  if (source->GetStatus().status() != net::URLRequestStatus::SUCCESS) {
    LOG(ERROR) << "Attestation CA request failed, status: "
               << source->GetStatus().status() << ", error: "
               << source->GetStatus().error();
    callback.Run(false, "");
    return;
  }

  if (source->GetResponseCode() != net::HTTP_OK) {
    LOG(ERROR) << "Attestation CA sent an error response: "
               << source->GetResponseCode();
    callback.Run(false, "");
    return;
  }

  std::string response;
  bool result = source->GetResponseAsString(&response);
  DCHECK(result) << "Invalid fetcher setting.";

  // Run the callback last because it may delete |this|.
  callback.Run(true, response);
}

void AttestationCAClient::FetchURL(const std::string& url,
                                   const std::string& request,
                                   const DataCallback& on_response) {
  const net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("attestation_ca_client", R"(
        semantics {
          sender: "Attestation CA client"
          description:
            "Sends requests to the Attestation CA as part of the remote "
            "attestation feature, such as enrolling for remote attestation or "
            "to obtain an attestation certificate."
          trigger:
            "Device enrollment, content protection or get an attestation "
            "certificate for a hardware-protected key."
          data:
            "The data from AttestationCertificateRequest or from "
            "AttestationEnrollmentRequest message from "
            "cryptohome/attestation.proto. Some of the important data being "
            "encrypted endorsement certificate, attestation identity public "
            "key, PCR0 and PCR1 TPM values."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "The device setting DeviceAttestationEnabled can disable the "
            "attestation requests and AttestationForContentProtectionEnabled "
            "can disable the attestation for content protection. But they "
            "cannot be controlled by a policy or through settings."
          policy_exception_justification: "Not implemented."
        })");
  // The first argument allows the use of TestURLFetcherFactory in tests.
  net::URLFetcher* fetcher =
      net::URLFetcher::Create(0, GURL(url), net::URLFetcher::POST, this,
                              traffic_annotation)
          .release();
  fetcher->SetRequestContext(g_browser_process->system_request_context());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_DISABLE_CACHE);
  fetcher->SetUploadData(kMimeContentType, request);
  pending_requests_[fetcher] = on_response;
  fetcher->Start();
}

PrivacyCAType AttestationCAClient::GetType() {
  return pca_type_;
}

}  // namespace attestation
}  // namespace chromeos
