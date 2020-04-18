// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/android/cert/cert_verifier_cache_serializer.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "components/cronet/android/cert/proto/cert_verification.pb.h"
#include "net/base/hash_value.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"

namespace cronet {

namespace {

// Map from each unique certificate to certificate number.
typedef std::map<std::string, size_t> SerializedCertMap;
// Map from certificate number to each unique certificate.
typedef std::map<size_t, std::string> DeserializedCertMap;

// Determine if |cert_handle| was already serialized. If so, simply return a
// reference to that entry. Otherwise, add a new entry to the set of certs to
// be serialized (|serialized_certs|).
size_t SerializeCertHandle(const CRYPTO_BUFFER* cert_handle,
                           SerializedCertMap* serialized_certs) {
  auto result = serialized_certs->insert(
      {std::string(net::x509_util::CryptoBufferAsStringPiece(cert_handle)),
       serialized_certs->size() + 1});
  return result.first->second;
}

// Update |certificate| with certificate number and updates |serialized_certs|
// with DER-encoded representation of certificate if the certicate is not in
// |serialized_certs|. Returns true if data is serialized correctly.
void SerializeCertificate(net::X509Certificate* cert,
                          SerializedCertMap* serialized_certs,
                          cronet_pb::CertVerificationCertificate* certificate) {
  certificate->add_cert_numbers(
      SerializeCertHandle(cert->cert_buffer(), serialized_certs));
  for (const auto& intermediate : cert->intermediate_buffers()) {
    certificate->add_cert_numbers(
        SerializeCertHandle(intermediate.get(), serialized_certs));
  }
}

// Deserializes |certificate| using the certificate database provided in
// |deserialized_certs|. Returns the parsed certificate on success, or nullptr
// if deserialization failed.
scoped_refptr<net::X509Certificate> DeserializeCertificate(
    const cronet_pb::CertVerificationCertificate& certificate,
    const DeserializedCertMap& deserialized_certs) {
  if (0 == certificate.cert_numbers_size())
    return nullptr;
  std::vector<base::StringPiece> der_cert_pieces(
      certificate.cert_numbers_size());
  for (int i = 0; i < certificate.cert_numbers_size(); ++i) {
    size_t cert_number = certificate.cert_numbers(i);
    DeserializedCertMap::const_iterator it =
        deserialized_certs.find(cert_number);
    if (it == deserialized_certs.end())
      return nullptr;
    der_cert_pieces[i] = base::StringPiece(it->second);
  }
  return net::X509Certificate::CreateFromDERCertChain(der_cert_pieces);
}

// Serializes |params| into |request_params|, updating |serialized_certs| with
// the set of raw certificates that will be needed to deserialize the
// certificate in |request_params| via DeserializeCertificate().
void SerializeRequestParams(
    const net::CertVerifier::RequestParams& params,
    SerializedCertMap* serialized_certs,
    cronet_pb::CertVerificationRequestParams* request_params) {
  cronet_pb::CertVerificationCertificate* certificate =
      request_params->mutable_certificate();
  SerializeCertificate(params.certificate().get(), serialized_certs,
                       certificate);
  request_params->set_hostname(params.hostname());
  request_params->set_flags(params.flags());
  request_params->set_ocsp_response(params.ocsp_response());
  for (const auto& cert : params.additional_trust_anchors()) {
    certificate = request_params->add_additional_trust_anchors();
    SerializeCertificate(cert.get(), serialized_certs, certificate);
  }
}

// Serializes |result| into |cached_result|, updating |serialized_certs| with
// the set of raw certificates that will be needed to deserialize the
// certificate in |cached_result| via DeserializeCertificate().
void SerializeCachedResult(
    const net::CertVerifyResult& result,
    SerializedCertMap* serialized_certs,
    cronet_pb::CertVerificationCachedResult* cached_result) {
  cronet_pb::CertVerificationResult* cert_verification_result =
      cached_result->mutable_result();
  cronet_pb::CertVerificationCertificate* certificate =
      cert_verification_result->mutable_verified_cert();
  SerializeCertificate(result.verified_cert.get(), serialized_certs,
                       certificate);
  cert_verification_result->set_cert_status(result.cert_status);
  cert_verification_result->set_has_md2(result.has_md2);
  cert_verification_result->set_has_md4(result.has_md4);
  cert_verification_result->set_has_md5(result.has_md5);
  cert_verification_result->set_has_sha1(result.has_sha1);
  cert_verification_result->set_has_sha1_leaf(result.has_sha1_leaf);
  for (const auto& value : result.public_key_hashes)
    cert_verification_result->add_public_key_hashes(value.ToString());
  cert_verification_result->set_is_issued_by_known_root(
      result.is_issued_by_known_root);
  cert_verification_result->set_is_issued_by_additional_trust_anchor(
      result.is_issued_by_additional_trust_anchor);
}

// Deserializes |cached_result| using the certificate database provided in
// |deserialized_certs|. Returns the parsed net::CertVerifyResult on success, or
// nullptr if deserialization failed.
bool DeserializeCachedResult(
    const cronet_pb::CertVerificationCachedResult& cached_result,
    const DeserializedCertMap& deserialized_certs,
    int* error,
    net::CertVerifyResult* result) {
  if (!cached_result.has_error() || !cached_result.has_result())
    return false;

  const cronet_pb::CertVerificationResult& cert_verification_result =
      cached_result.result();
  if (!cert_verification_result.has_verified_cert() ||
      !cert_verification_result.has_cert_status()) {
    return false;
  }

  *error = cached_result.error();

  result->verified_cert = DeserializeCertificate(
      cert_verification_result.verified_cert(), deserialized_certs);
  if (!result->verified_cert)
    return false;

  for (int i = 0; i < cert_verification_result.public_key_hashes_size(); ++i) {
    const std::string& public_key_hash =
        cert_verification_result.public_key_hashes(i);
    net::HashValue hash;
    if (!hash.FromString(public_key_hash))
      return false;
    result->public_key_hashes.push_back(hash);
  }
  result->cert_status = cert_verification_result.cert_status();
  result->has_md2 = cert_verification_result.has_md2();
  result->has_md4 = cert_verification_result.has_md4();
  result->has_md5 = cert_verification_result.has_md5();
  result->has_sha1 = cert_verification_result.has_sha1();
  result->has_sha1_leaf = cert_verification_result.has_sha1_leaf();
  result->is_issued_by_known_root =
      cert_verification_result.is_issued_by_known_root();
  result->is_issued_by_additional_trust_anchor =
      cert_verification_result.is_issued_by_additional_trust_anchor();
  return true;
}

// Serializes |params|, |error|, |verify_result| and |verification_time| into
// |cert_cache|, updating |serialized_certs| with the set of raw certificates
// that will be needed to deserialize the certificate in |cert_cache| via
// DeserializeCertificate().
void SerializeCachedEntry(const net::CachingCertVerifier::RequestParams& params,
                          int error,
                          const net::CertVerifyResult& verify_result,
                          base::Time verification_time,
                          cronet_pb::CertVerificationCache* cert_cache,
                          SerializedCertMap* serialized_certs) {
  cronet_pb::CertVerificationCacheEntry* cache_entry =
      cert_cache->add_cache_entry();

  cronet_pb::CertVerificationRequestParams* request_params =
      cache_entry->mutable_request_params();
  SerializeRequestParams(params, serialized_certs, request_params);

  cronet_pb::CertVerificationCachedResult* cached_result =
      cache_entry->mutable_cached_result();
  SerializeCachedResult(verify_result, serialized_certs, cached_result);
  cached_result->set_error(error);

  cache_entry->set_verification_time(verification_time.ToInternalValue());
}

class CacheVisitor : public net::CachingCertVerifier::CacheVisitor {
 public:
  CacheVisitor() : failed_to_serialize_(false) {}
  ~CacheVisitor() override {}

  bool VisitEntry(const net::CachingCertVerifier::RequestParams& params,
                  int error,
                  const net::CertVerifyResult& verify_result,
                  base::Time verification_time,
                  base::Time expiration_time) override {
    SerializeCachedEntry(params, error, verify_result, verification_time,
                         &cert_cache_, &serialized_certs_);
    return true;
  }

  void Reset() {
    cert_cache_ = cronet_pb::CertVerificationCache();
    failed_to_serialize_ = true;
  }

  void SerializeCerts() {
    for (const auto& cert : serialized_certs_) {
      cronet_pb::CertVerificationCertificateData* cert_entry =
          cert_cache_.add_cert_entry();
      cert_entry->set_cert(cert.first);
      cert_entry->set_cert_number(cert.second);
    }
  }

  const cronet_pb::CertVerificationCache& cert_cache() const {
    return cert_cache_;
  }

  bool failed_to_serialize() const { return failed_to_serialize_; }

  cronet_pb::CertVerificationCache cert_cache_;
  SerializedCertMap serialized_certs_;
  bool failed_to_serialize_;
};

struct CertVerifierCacheEntry {
  CertVerifierCacheEntry(const net::CertVerifier::RequestParams& params,
                         int error,
                         const net::CertVerifyResult& result,
                         base::Time verification_time)
      : params(params),
        error(error),
        result(result),
        verification_time(verification_time) {}

  net::CertVerifier::RequestParams params;
  int error;
  net::CertVerifyResult result;
  base::Time verification_time;
};

}  // namespace

cronet_pb::CertVerificationCache SerializeCertVerifierCache(
    const net::CachingCertVerifier& verifier) {
  CacheVisitor visitor;
  verifier.VisitEntries(&visitor);

  if (!visitor.failed_to_serialize())
    visitor.SerializeCerts();
  return visitor.cert_cache();
}

bool DeserializeCertVerifierCache(
    const cronet_pb::CertVerificationCache& cert_cache,
    net::CachingCertVerifier* verifier) {
  DeserializedCertMap deserialized_certs;

  if (cert_cache.cert_entry_size() == 0u ||
      cert_cache.cache_entry_size() == 0u) {
    return false;
  }

  // Build |deserialized_certs|'s certificate map.
  for (int i = 0; i < cert_cache.cert_entry_size(); ++i) {
    const cronet_pb::CertVerificationCertificateData& cert_entry =
        cert_cache.cert_entry(i);
    if (!cert_entry.has_cert() || !cert_entry.has_cert_number())
      return false;
    deserialized_certs.insert({cert_entry.cert_number(), cert_entry.cert()});
  }

  std::vector<std::unique_ptr<CertVerifierCacheEntry>>
      cert_verifier_cache_entries;
  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    const cronet_pb::CertVerificationCacheEntry& cache_entry =
        cert_cache.cache_entry(i);

    // Verify |cache_entry|'s data.
    if (!cache_entry.has_request_params() ||
        !cache_entry.has_verification_time() ||
        !cache_entry.has_cached_result()) {
      return false;
    }

    const cronet_pb::CertVerificationRequestParams& request_params =
        cache_entry.request_params();

    // Verify |request_params|'s data.
    if (!request_params.has_certificate() || !request_params.has_hostname() ||
        request_params.hostname().empty() || !request_params.has_flags() ||
        !request_params.has_ocsp_response()) {
      return false;
    }

    // Deserialize |request_params|'s certificate using the certificate database
    // provided in |deserialized_certs|.
    scoped_refptr<net::X509Certificate> certificate = DeserializeCertificate(
        request_params.certificate(), deserialized_certs);
    if (!certificate)
      return false;

    // Deserialize |request_params|'s trust anchor certificates using the
    // certificate database provided in |deserialized_certs|.
    net::CertificateList additional_trust_anchors;
    for (int i = 0; i < request_params.additional_trust_anchors_size(); ++i) {
      const cronet_pb::CertVerificationCertificate& certificate =
          request_params.additional_trust_anchors(i);
      scoped_refptr<net::X509Certificate> cert =
          DeserializeCertificate(certificate, deserialized_certs);
      if (!cert)
        return false;
      additional_trust_anchors.push_back(cert);
    }

    net::CertVerifier::RequestParams params(
        certificate, request_params.hostname(), request_params.flags(),
        request_params.ocsp_response(), additional_trust_anchors);

    // Deserialize |cached_result| into |result|.
    const cronet_pb::CertVerificationCachedResult& cached_result =
        cache_entry.cached_result();
    net::CertVerifyResult result;
    int error;
    if (!DeserializeCachedResult(cached_result, deserialized_certs, &error,
                                 &result)) {
      return false;
    }

    base::Time verification_time =
        base::Time::FromInternalValue(cache_entry.verification_time());
    // We are deserializing the data that was persisted in the past and thus
    // |verification_time| can not be in the future.
    if (verification_time.is_null() || verification_time >= base::Time::Now())
      return false;

    std::unique_ptr<CertVerifierCacheEntry> cert_verifier_cache_entry(
        new CertVerifierCacheEntry(params, error, result, verification_time));
    cert_verifier_cache_entries.push_back(std::move(cert_verifier_cache_entry));
  }

  for (const auto& entry : cert_verifier_cache_entries) {
    verifier->AddEntry(entry->params, entry->error, entry->result,
                       entry->verification_time);
  }
  return true;
}

}  // namespace cronet
