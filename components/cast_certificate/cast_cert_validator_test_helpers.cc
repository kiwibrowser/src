// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_certificate/cast_cert_validator_test_helpers.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "net/cert/internal/cert_errors.h"
#include "net/cert/pem_tokenizer.h"
#include "net/cert/x509_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cast_certificate {

namespace testing {

std::string ReadTestFileToString(const base::StringPiece& file_name) {
  base::FilePath filepath;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &filepath);
  filepath = filepath.Append(FILE_PATH_LITERAL("components"));
  filepath = filepath.Append(FILE_PATH_LITERAL("test"));
  filepath = filepath.Append(FILE_PATH_LITERAL("data"));
  filepath = filepath.Append(FILE_PATH_LITERAL("cast_certificate"));
  filepath = filepath.AppendASCII(file_name);

  // Read the full contents of the file.
  std::string file_data;
  if (!base::ReadFileToString(filepath, &file_data)) {
    ADD_FAILURE() << "Couldn't read file: " << filepath.value();
    return std::string();
  }

  return file_data;
}

std::vector<std::string> ReadCertificateChainFromFile(
    const base::StringPiece& file_name) {
  std::string file_data = ReadTestFileToString(file_name);

  std::vector<std::string> pem_headers;
  pem_headers.push_back("CERTIFICATE");

  std::vector<std::string> certs;
  net::PEMTokenizer pem_tokenizer(file_data, pem_headers);
  while (pem_tokenizer.GetNext())
    certs.push_back(pem_tokenizer.data());

  EXPECT_FALSE(certs.empty());
  return certs;
}

SignatureTestData ReadSignatureTestData(const base::StringPiece& file_name) {
  SignatureTestData result;

  std::string file_data = ReadTestFileToString(file_name);
  EXPECT_FALSE(file_data.empty());

  std::vector<std::string> pem_headers;
  pem_headers.push_back("MESSAGE");
  pem_headers.push_back("SIGNATURE SHA1");
  pem_headers.push_back("SIGNATURE SHA256");

  net::PEMTokenizer pem_tokenizer(file_data, pem_headers);
  while (pem_tokenizer.GetNext()) {
    const std::string& type = pem_tokenizer.block_type();
    const std::string& value = pem_tokenizer.data();

    if (type == "MESSAGE") {
      result.message = value;
    } else if (type == "SIGNATURE SHA1") {
      result.signature_sha1 = value;
    } else if (type == "SIGNATURE SHA256") {
      result.signature_sha256 = value;
    }
  }

  EXPECT_FALSE(result.message.empty());
  EXPECT_FALSE(result.signature_sha1.empty());
  EXPECT_FALSE(result.signature_sha256.empty());

  return result;
}

std::unique_ptr<net::TrustStoreInMemory> CreateTrustStoreFromFile(
    const std::string& path) {
  std::unique_ptr<net::TrustStoreInMemory> trust_store(
      new net::TrustStoreInMemory());
  const auto trusted_test_roots =
      cast_certificate::testing::ReadCertificateChainFromFile(path);
  for (const auto& trusted_root : trusted_test_roots) {
    net::CertErrors errors;
    scoped_refptr<net::ParsedCertificate> cert(net::ParsedCertificate::Create(
        net::x509_util::CreateCryptoBuffer(trusted_root), {}, &errors));
    EXPECT_TRUE(cert) << errors.ToDebugString();
    trust_store->AddTrustAnchorWithConstraints(cert);
  }
  return trust_store;
}

base::Time ConvertUnixTimestampSeconds(uint64_t time) {
  return base::Time::UnixEpoch() + base::TimeDelta::FromSeconds(time);
}

}  // namespace testing

}  // namespace cast_certificate
