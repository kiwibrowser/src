// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_CERTIFICATE_HELPER_H_
#define CHROMEOS_NETWORK_CERTIFICATE_HELPER_H_

#include <cert.h>
#include <string>

#include "chromeos/chromeos_export.h"
#include "net/cert/cert_type.h"

namespace chromeos {
namespace certificate {

// Selected functions from chrome/common/net/x509_certificate_model.cc

// Decodes the certificate type from |cert|.
CHROMEOS_EXPORT net::CertType GetCertType(CERTCertificate* cert_handle);

// Extracts the token name from |cert|->slot if it exists or returns an empty
// string.
CHROMEOS_EXPORT std::string GetCertTokenName(CERTCertificate* cert_handle);

// Returns a name that can be used to represent the issuer of |cert_handle|.
// It tries in this order: CN, O and OU and returns the first non-empty one
// found.
CHROMEOS_EXPORT std::string GetIssuerDisplayName(CERTCertificate* cert_handle);

// Returns the common name for |cert_handle|->subject converted to unicode,
// or |cert_handle|->nickname if the subject name is unavailable or empty.
// NOTE: Unlike x509_certificate_model::GetCertNameOrNickname in src/chrome,
// the raw unformated name or nickname will not be included in the returned
// value (see GetCertAsciiNameOrNickname instead).
CHROMEOS_EXPORT std::string GetCertNameOrNickname(CERTCertificate* cert_handle);

// Returns the unformated ASCII common name for |cert_handle|->subject. Returns
// an empty string if the subject name is unavailable or empty.
CHROMEOS_EXPORT std::string GetCertAsciiSubjectCommonName(
    CERTCertificate* cert_handle);

// Returns the unformatted ASCII common name for |cert_handle|->subject or for
// |cert_handle|->nickname if the subject name is unavailable or empty. If both
// are not available, returns an empty string.
CHROMEOS_EXPORT std::string GetCertAsciiNameOrNickname(
    CERTCertificate* cert_handle);

}  // namespace certificate
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_CERTIFICATE_HELPER_H_
