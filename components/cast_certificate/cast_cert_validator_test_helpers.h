// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CERTIFICATE_CAST_CERT_VALIDATOR_TEST_HELPERS_H_
#define COMPONENTS_CAST_CERTIFICATE_CAST_CERT_VALIDATOR_TEST_HELPERS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "net/cert/internal/trust_store_in_memory.h"

namespace net {
class TrustStoreInMemory;
}

namespace cast_certificate {

namespace testing {

// Reads a PEM file containing certificates to a vector of their DER data.
// |file_name| should be relative to //components/test/data/cast_certificate
std::vector<std::string> ReadCertificateChainFromFile(
    const base::StringPiece& file_name);

// Helper structure that describes a message and its various signatures.
struct SignatureTestData {
  std::string message;

  // RSASSA PKCS#1 v1.5 with SHA1.
  std::string signature_sha1;

  // RSASSA PKCS#1 v1.5 with SHA256.
  std::string signature_sha256;
};

// Reads a PEM file that contains "MESSAGE", "SIGNATURE SHA1" and
// "SIGNATURE SHA256" blocks.
// |file_name| should be relative to //components/test/data/cast_certificate
SignatureTestData ReadSignatureTestData(const base::StringPiece& file_name);

// Reads a file from the test data directory
// (//src/components/test/data/cast_certificate)
std::string ReadTestFileToString(const base::StringPiece& file_name);

// Creates a trust store using the test roots encoded in the PEM file at |path|.
std::unique_ptr<net::TrustStoreInMemory> CreateTrustStoreFromFile(
    const std::string& path);

// Converts uint64_t unix timestamp in seconds to base::Time.
base::Time ConvertUnixTimestampSeconds(uint64_t time);

}  // namespace testing

}  // namespace cast_certificate

#endif  // COMPONENTS_CAST_CERTIFICATE_CAST_CERT_VALIDATOR_TEST_HELPERS_H_
