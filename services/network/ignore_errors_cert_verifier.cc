// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/ignore_errors_cert_verifier.h"

#include <iterator>
#include <utility>

#include "base/base64.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "crypto/sha2.h"
#include "net/base/completion_callback.h"
#include "net/base/hash_value.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "services/network/public/cpp/network_switches.h"

using ::net::CertVerifier;
using ::net::CompletionCallback;
using ::net::HashValue;
using ::net::SHA256HashValue;
using ::net::X509Certificate;

namespace network {

// static
std::unique_ptr<CertVerifier> IgnoreErrorsCertVerifier::MaybeWrapCertVerifier(
    const base::CommandLine& command_line,
    const char* user_data_dir_switch,
    std::unique_ptr<CertVerifier> verifier) {
  if ((user_data_dir_switch && !command_line.HasSwitch(user_data_dir_switch)) ||
      !command_line.HasSwitch(switches::kIgnoreCertificateErrorsSPKIList)) {
    return verifier;
  }
  auto spki_list =
      base::SplitString(command_line.GetSwitchValueASCII(
                            switches::kIgnoreCertificateErrorsSPKIList),
                        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  return std::make_unique<IgnoreErrorsCertVerifier>(
      std::move(verifier), IgnoreErrorsCertVerifier::MakeWhitelist(spki_list));
}

// static
IgnoreErrorsCertVerifier::SPKIHashSet IgnoreErrorsCertVerifier::MakeWhitelist(
    const std::vector<std::string>& fingerprints) {
  IgnoreErrorsCertVerifier::SPKIHashSet whitelist;
  for (const std::string& fingerprint : fingerprints) {
    HashValue hash;
    if (!hash.FromString("sha256/" + fingerprint)) {
      LOG(ERROR) << "Invalid SPKI: " << fingerprint;
      continue;
    }
    SHA256HashValue sha256;
    DCHECK_EQ(hash.size(), sizeof(sha256));
    memcpy(&sha256, hash.data(), sizeof(sha256));
    whitelist.insert(sha256);
  }
  return whitelist;
}

IgnoreErrorsCertVerifier::IgnoreErrorsCertVerifier(
    std::unique_ptr<CertVerifier> verifier,
    IgnoreErrorsCertVerifier::SPKIHashSet whitelist)
    : verifier_(std::move(verifier)), whitelist_(std::move(whitelist)) {}

IgnoreErrorsCertVerifier::~IgnoreErrorsCertVerifier() {}

int IgnoreErrorsCertVerifier::Verify(const RequestParams& params,
                                     net::CRLSet* crl_set,
                                     net::CertVerifyResult* verify_result,
                                     const net::CompletionCallback& callback,
                                     std::unique_ptr<Request>* out_req,
                                     const net::NetLogWithSource& net_log) {
  SPKIHashSet spki_fingerprints;
  base::StringPiece cert_spki;
  SHA256HashValue hash;
  if (net::asn1::ExtractSPKIFromDERCert(
          net::x509_util::CryptoBufferAsStringPiece(
              params.certificate()->cert_buffer()),
          &cert_spki)) {
    crypto::SHA256HashString(cert_spki, &hash, sizeof(SHA256HashValue));
    spki_fingerprints.insert(hash);
  }
  for (const auto& intermediate :
       params.certificate()->intermediate_buffers()) {
    if (net::asn1::ExtractSPKIFromDERCert(
            net::x509_util::CryptoBufferAsStringPiece(intermediate.get()),
            &cert_spki)) {
      crypto::SHA256HashString(cert_spki, &hash, sizeof(SHA256HashValue));
      spki_fingerprints.insert(hash);
    }
  }

  // Intersect SPKI hashes from the chain with the whitelist.
  auto whitelist_begin = whitelist_.begin();
  auto whitelist_end = whitelist_.end();
  auto fingerprints_begin = spki_fingerprints.begin();
  auto fingerprints_end = spki_fingerprints.end();
  bool ignore_errors = false;
  while (whitelist_begin != whitelist_end &&
         fingerprints_begin != fingerprints_end) {
    if (*whitelist_begin < *fingerprints_begin) {
      ++whitelist_begin;
    } else if (*fingerprints_begin < *whitelist_begin) {
      ++fingerprints_begin;
    } else {
      ignore_errors = true;
      break;
    }
  }

  if (ignore_errors) {
    verify_result->Reset();
    verify_result->verified_cert = params.certificate();
    std::transform(spki_fingerprints.begin(), spki_fingerprints.end(),
                   std::back_inserter(verify_result->public_key_hashes),
                   [](const SHA256HashValue& v) { return HashValue(v); });
    if (!params.ocsp_response().empty()) {
      verify_result->ocsp_result.response_status =
          net::OCSPVerifyResult::PROVIDED;
      verify_result->ocsp_result.revocation_status =
          net::OCSPRevocationStatus::GOOD;
    }
    return net::OK;
  }

  return verifier_->Verify(params, crl_set, verify_result, callback, out_req,
                           net_log);
}

void IgnoreErrorsCertVerifier::set_whitelist(const SPKIHashSet& whitelist) {
  whitelist_ = whitelist;
}

}  // namespace network
