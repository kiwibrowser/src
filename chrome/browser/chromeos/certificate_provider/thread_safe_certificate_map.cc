// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/certificate_provider/thread_safe_certificate_map.h"

#include "net/base/hash_value.h"
#include "net/cert/x509_certificate.h"

namespace chromeos {
namespace certificate_provider {
namespace {

void BuildFingerprintsMap(
    const std::map<std::string, certificate_provider::CertificateInfoList>&
        extension_to_certificates,
    ThreadSafeCertificateMap::FingerprintToCertAndExtensionMap*
        fingerprint_to_cert) {
  for (const auto& entry : extension_to_certificates) {
    const std::string& extension_id = entry.first;
    for (const CertificateInfo& cert_info : entry.second) {
      const net::SHA256HashValue fingerprint =
          net::X509Certificate::CalculateFingerprint256(
              cert_info.certificate->cert_buffer());
      fingerprint_to_cert->insert(std::make_pair(
          fingerprint, std::make_unique<ThreadSafeCertificateMap::MapValue>(
                           cert_info, extension_id)));
    }
  }
}

}  // namespace

ThreadSafeCertificateMap::MapValue::MapValue(const CertificateInfo& cert_info,
                                             const std::string& extension_id)
    : cert_info(cert_info), extension_id(extension_id) {}

ThreadSafeCertificateMap::MapValue::~MapValue() {}

ThreadSafeCertificateMap::ThreadSafeCertificateMap() {}

ThreadSafeCertificateMap::~ThreadSafeCertificateMap() {}

void ThreadSafeCertificateMap::Update(
    const std::map<std::string, certificate_provider::CertificateInfoList>&
        extension_to_certificates) {
  FingerprintToCertAndExtensionMap new_fingerprint_map;
  BuildFingerprintsMap(extension_to_certificates, &new_fingerprint_map);

  base::AutoLock auto_lock(lock_);
  // Keep all old fingerprints from |fingerprint_to_cert_and_extension_| but
  // remove the association to any extension.
  for (const auto& entry : fingerprint_to_cert_and_extension_) {
    const net::SHA256HashValue& fingerprint = entry.first;
    // This doesn't modify the map if it already contains the key |fingerprint|.
    new_fingerprint_map.insert(std::make_pair(fingerprint, nullptr));
  }
  fingerprint_to_cert_and_extension_.swap(new_fingerprint_map);
}

bool ThreadSafeCertificateMap::LookUpCertificate(
    const net::X509Certificate& cert,
    bool* is_currently_provided,
    CertificateInfo* info,
    std::string* extension_id) {
  *is_currently_provided = false;
  const net::SHA256HashValue fingerprint =
      net::X509Certificate::CalculateFingerprint256(cert.cert_buffer());

  base::AutoLock auto_lock(lock_);
  const auto it = fingerprint_to_cert_and_extension_.find(fingerprint);
  if (it == fingerprint_to_cert_and_extension_.end())
    return false;

  MapValue* const value = it->second.get();
  if (value) {
    *is_currently_provided = true;
    *info = value->cert_info;
    *extension_id = value->extension_id;
  }
  return true;
}

void ThreadSafeCertificateMap::RemoveExtension(
    const std::string& extension_id) {
  base::AutoLock auto_lock(lock_);
  for (auto& entry : fingerprint_to_cert_and_extension_) {
    MapValue* const value = entry.second.get();
    // Only remove the association of the fingerprint to the extension, but keep
    // the fingerprint.
    if (value && value->extension_id == extension_id)
      fingerprint_to_cert_and_extension_[entry.first] = nullptr;
  }
}

}  // namespace certificate_provider
}  // namespace chromeos
