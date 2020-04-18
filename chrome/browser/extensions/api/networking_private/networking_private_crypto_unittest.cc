// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/extensions/api/networking_private/networking_private_crypto.h"

#include <stdint.h>

#include "base/base64.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

}  // namespace

// Tests of networking_private_crypto support for Networking Private API.
class NetworkingPrivateCryptoTest : public testing::Test {
 protected:
  // Verify that decryption of |encrypted| data using |private_key_pem| matches
  // |plain| data.
  bool VerifyByteString(const std::string& private_key_pem,
                        const std::string& plain,
                        const std::vector<uint8_t>& encrypted) {
    std::string decrypted;
    if (networking_private_crypto::DecryptByteString(
            private_key_pem, encrypted, &decrypted))
      return decrypted == plain;
    return false;
  }
};

// Test that networking_private_crypto::VerifyCredentials behaves as expected.
TEST_F(NetworkingPrivateCryptoTest, VerifyCredentials) {
  // This certificate chain and signature are duplicated from:
  //
  //   components/test/data/cast_certificate/certificates/chromecast_gen1.pem
  //   components/test/data/cast_certificate/signeddata/2ZZBG9_FA8FCA3EF91A.pem
  //
  // TODO(eroman): Avoid duplicating the data.
  static const char kCertData[] =
      "-----BEGIN CERTIFICATE-----"
      "MIIDrDCCApSgAwIBAgIEU8xPLDANBgkqhkiG9w0BAQUFADB9MQswCQYDVQQGEwJV"
      "UzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNTW91bnRhaW4gVmlldzET"
      "MBEGA1UECgwKR29vZ2xlIEluYzESMBAGA1UECwwJR29vZ2xlIFRWMRgwFgYDVQQD"
      "DA9FdXJla2EgR2VuMSBJQ0EwHhcNMTQwNzIwMjMyMjIwWhcNMzQwNzE1MjMyMjIw"
      "WjCBgTELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExEzARBgNVBAoT"
      "Ckdvb2dsZSBJbmMxFjAUBgNVBAcTDU1vdW50YWluIFZpZXcxEjAQBgNVBAsTCUdv"
      "b2dsZSBUVjEcMBoGA1UEAxMTMlpaQkc5IEZBOEZDQTNFRjkxQTCCASIwDQYJKoZI"
      "hvcNAQEBBQADggEPADCCAQoCggEBAKV56Srec2ePlqDP6cqFPuwU4MOs7MOcGDrv"
      "da6qy6tWC7BmsqipMA/hn77iUiBZsw3TbUQnVfmM4ZQ2RENzcrAJ68cmc+lPxmRr"
      "8x1Xu5FzZ+kcyU8glLLqdiXYEKRboFhC7BM05O1XOLvzCls4zuZuMrGNFBW+YoBm"
      "FiXFYWBhapZC3RhhlSEZFuQWbb/MUSDzwr/CRbn4tKHMv4Fkw5HAnhLa+yXfgCGw"
      "qOd9GejqUKsO/aajAHkM7lIHmvkthI4MVk0Koc+Ih487pgsOt18LqubZVEkbjCqp"
      "Rpx1CGbErWnw2ptPvMCEC6e7mrYHcYgmuzQ7m+eUlhthEUiTYC0CAwEAAaMvMC0w"
      "CQYDVR0TBAIwADALBgNVHQ8EBAMCB4AwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDQYJ"
      "KoZIhvcNAQEFBQADggEBAGuKgGXHJXQ1M7P4uXB8wPPuT2h6g29YJ62rUvZ7BrlW"
      "TknJT0Owaw68zepLhFQ4ydIzbVV3hA2InCmP3U24ZMxMJcA/9qNPAqPrtE1ZIQNI"
      "Qh6slAdZa0qM6Us30/5fpUL6lgAfD1RIJxA4RWYZKP78SjJz1Lybx3Zbt0Jist9G"
      "tvaJGZjZrdPncnJKayGaIln8gzHd6MVEGZp7aIQZ2h4NDlnrwyhMFTjg1WvnmQJ6"
      "3bEvjSyjMGhY0JOUaDp/UMxnExn+1+cYAW9LrosZXtRDNJTl1zX4auAnNMHkt8uC"
      "F8Jhy80X2wU0fj85oYbRsm+jBMtRayznY1TR0WoPBAo="
      "-----END CERTIFICATE-----";

  static const char kICAData[] =
      "-----BEGIN CERTIFICATE-----"
      "MIIDhzCCAm+gAwIBAgIBATANBgkqhkiG9w0BAQUFADB8MQswCQYDVQQGEwJVUzET"
      "MBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNTW91bnRhaW4gVmlldzETMBEG"
      "A1UECgwKR29vZ2xlIEluYzESMBAGA1UECwwJR29vZ2xlIFRWMRcwFQYDVQQDDA5F"
      "dXJla2EgUm9vdCBDQTAeFw0xMjEyMTkwMDQ3MTJaFw0zMjEyMTQwMDQ3MTJaMH0x"
      "CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1Nb3Vu"
      "dGFpbiBWaWV3MRMwEQYDVQQKDApHb29nbGUgSW5jMRIwEAYDVQQLDAlHb29nbGUg"
      "VFYxGDAWBgNVBAMMD0V1cmVrYSBHZW4xIElDQTCCASIwDQYJKoZIhvcNAQEBBQAD"
      "ggEPADCCAQoCggEBALwigL2A9johADuudl41fz3DZFxVlIY0LwWHKM33aYwXs1Cn"
      "uIL638dDLdZ+q6BvtxNygKRHFcEgmVDN7BRiCVukmM3SQbY2Tv/oLjIwSoGoQqNs"
      "mzNuyrL1U2bgJ1OGGoUepzk/SneO+1RmZvtYVMBeOcf1UAYL4IrUzuFqVR+LFwDm"
      "aaMn5gglaTwSnY0FLNYuojHetFJQ1iBJ3nGg+a0gQBLx3SXr1ea4NvTWj3/KQ9zX"
      "EFvmP1GKhbPz//YDLcsjT5ytGOeTBYysUpr3TOmZer5ufk0K48YcqZP6OqWRXRy9"
      "ZuvMYNyGdMrP+JIcmH1X+mFHnquAt+RIgCqSxRsCAwEAAaMTMBEwDwYDVR0TBAgw"
      "BgEB/wIBATANBgkqhkiG9w0BAQUFAAOCAQEAi9Shsc9dzXtsSEpBH1MvGC0yRf+e"
      "q9NzPh8i1+r6AeZzAw8rxiW7pe7F9UXLJBIqrcJdBfR69cKbEBZa0QpzxRY5oBDK"
      "0WiFnvueJoOOWPN3oE7l25e+LQBf9ZTbsZ1la/3w0QRR38ySppktcfVN1SP+Mxyp"
      "tKvFvxq40YDvicniH5xMSDui+gIK3IQBiocC+1nup0wEfXSZh2olRK0WquxONRt8"
      "e4TJsT/hgnDlDefZbfqVtsXkHugRm9iy86T9E/ODT/cHFCC7IqWmj9a126l0eOKT"
      "DeUjLwUX4LKXZzRND5x2Q3umIUpWBfYqfPJ/EpSCJikH8AtsbHkUsHTVbA=="
      "-----END CERTIFICATE-----";

  unsigned char kData[] = {0x53, 0x54, 0x52, 0x49, 0x4e, 0x47};

  unsigned char kSignature[] = {
      0x0a, 0xda, 0xb5, 0x40, 0x5c, 0x8e, 0x53, 0x89, 0xda, 0x67, 0x47, 0x28,
      0xab, 0x64, 0x0d, 0xec, 0xb8, 0x1f, 0xd6, 0x75, 0x28, 0x97, 0x5f, 0xe0,
      0x11, 0x51, 0x35, 0x2a, 0x70, 0xd8, 0xf6, 0x4d, 0xe8, 0xd0, 0x2e, 0xe0,
      0x79, 0x75, 0x3a, 0x25, 0xbf, 0x40, 0x0f, 0x6d, 0xd1, 0x20, 0xe3, 0x82,
      0xbd, 0x05, 0x87, 0x57, 0x01, 0x1e, 0x76, 0xb7, 0xf4, 0xd7, 0xb3, 0x10,
      0x4a, 0x6c, 0x8a, 0xf9, 0x3d, 0xe7, 0xeb, 0x62, 0xe9, 0x5f, 0x73, 0xab,
      0x6e, 0x22, 0xf5, 0x59, 0x4d, 0xc4, 0xa3, 0x95, 0xc3, 0xbe, 0x7b, 0x04,
      0x5a, 0x36, 0x67, 0xee, 0x71, 0xb2, 0xe8, 0x60, 0xbe, 0xaa, 0x2c, 0x90,
      0x36, 0xd7, 0xf0, 0x42, 0x28, 0xd4, 0x29, 0x9f, 0x30, 0xaa, 0x10, 0x4f,
      0x2a, 0xe1, 0x72, 0x67, 0xcc, 0xb5, 0x44, 0x7b, 0x7f, 0x89, 0x45, 0x9f,
      0xc3, 0x9d, 0x6a, 0xf0, 0x78, 0x77, 0x6d, 0x9f, 0x13, 0x58, 0x35, 0x09,
      0x8c, 0x71, 0xaf, 0x34, 0x4b, 0x18, 0xc7, 0x07, 0xd2, 0xf2, 0x03, 0x48,
      0xe2, 0x40, 0x75, 0x3b, 0xeb, 0x33, 0x74, 0x8d, 0x33, 0xb4, 0x45, 0xe2,
      0x59, 0x56, 0x8b, 0xc7, 0x4e, 0x60, 0xc7, 0xec, 0xc8, 0xd3, 0x32, 0x16,
      0x20, 0xb0, 0xc7, 0x0d, 0x14, 0x4b, 0x68, 0xbf, 0x79, 0xad, 0x7e, 0x47,
      0x5d, 0x5d, 0xb5, 0x8c, 0xb6, 0xc3, 0x27, 0xb9, 0xd8, 0x25, 0x70, 0xc0,
      0x8d, 0x12, 0x26, 0x51, 0xe8, 0xad, 0xde, 0xf8, 0xe8, 0x3e, 0x47, 0xd0,
      0xdf, 0x11, 0x7d, 0x34, 0x50, 0xa8, 0x89, 0x89, 0x59, 0x93, 0x8a, 0x3d,
      0x88, 0xaf, 0xd5, 0x1e, 0xe8, 0x34, 0x2e, 0x98, 0x62, 0x39, 0xc1, 0x22,
      0x06, 0xf7, 0x3e, 0x98, 0xfd, 0x6f, 0x3a, 0x45, 0xd0, 0xb7, 0x3a, 0xe5,
      0xaa, 0x38, 0x35, 0x2c, 0xe9, 0x78, 0x71, 0xe2, 0xf0, 0x6f, 0x60, 0x95,
      0xc0, 0x60, 0x5f, 0xc3,
  };

  static const char kHotspotBssid[] = "FA:8F:CA:3E:F9:1A";

  static const char kBadCertData[] = "not a certificate";
  static const char kBadHotspotBssid[] = "bad bssid";

  // April 1, 2016
  base::Time::Exploded time_exploded = {0};
  time_exploded.year = 2016;
  time_exploded.month = 4;
  time_exploded.day_of_month = 1;
  base::Time time;
  ASSERT_TRUE(base::Time::FromUTCExploded(time_exploded, &time));

  // September 1, 2035
  base::Time::Exploded expired_time_exploded = {0};
  expired_time_exploded.year = 2035;
  expired_time_exploded.month = 9;
  expired_time_exploded.day_of_month = 1;
  base::Time expired_time;
  ASSERT_TRUE(
      base::Time::FromUTCExploded(expired_time_exploded, &expired_time));

  std::string unsigned_data = std::string(std::begin(kData), std::end(kData));
  std::string signed_data =
      std::string(std::begin(kSignature), std::end(kSignature));

  // Check that verification fails when the intermediaries are not provided.
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, std::vector<std::string>(), signed_data, unsigned_data,
      kHotspotBssid, time));

  // Checking basic verification operation.
  std::vector<std::string> icas;
  icas.push_back(kICAData);

  EXPECT_TRUE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, icas, signed_data, unsigned_data, kHotspotBssid, time));

  // Checking that verification fails when the certificate is expired.
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, icas, signed_data, unsigned_data, kHotspotBssid,
      expired_time));

  // Checking that verification fails when certificate has invalid format.
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kBadCertData, icas, signed_data, unsigned_data, kHotspotBssid, time));

  // Checking that verification fails if we supply a bad ICA.
  std::vector<std::string> bad_icas;
  bad_icas.push_back(kCertData);
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, bad_icas, signed_data, unsigned_data, kHotspotBssid, time));

  // Checking that verification fails when Hotspot Bssid does not match the
  // certificate's common name.
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, icas, signed_data, unsigned_data, kBadHotspotBssid, time));

  // Checking that verification fails when the signature is wrong.
  unsigned_data = "bad data";
  EXPECT_FALSE(networking_private_crypto::VerifyCredentialsAtTime(
      kCertData, icas, signed_data, unsigned_data, kHotspotBssid, time));
}

// Test that networking_private_crypto::EncryptByteString behaves as expected.
TEST_F(NetworkingPrivateCryptoTest, EncryptByteString) {
  static const char kPublicKey[] =
      "MIGJAoGBANTjeoILNkSKHVkd3my/rSwNi+9t473vPJU0lkM8nn9C7+gmaPvEWg4ZNkMd12aI"
      "XDXVHrjgjcS80bPE0ykhN9J7EYkJ+43oulJMrEnyDy5KQo7U3MKBdjaKFTS+OPyohHpI8GqH"
      "KM8UMkLPVtAKu1BXgGTSDvEaBAuoVT2PM4XNAgMBAAE=";
  static const char kPrivateKey[] =
      "-----BEGIN PRIVATE KEY-----"
      "MIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBANTjeoILNkSKHVkd"
      "3my/rSwNi+9t473vPJU0lkM8nn9C7+gmaPvEWg4ZNkMd12aIXDXVHrjgjcS80bPE"
      "0ykhN9J7EYkJ+43oulJMrEnyDy5KQo7U3MKBdjaKFTS+OPyohHpI8GqHKM8UMkLP"
      "VtAKu1BXgGTSDvEaBAuoVT2PM4XNAgMBAAECgYEAt91H/2zjj8qhkkhDxDS/wd5p"
      "T37fRTmMX2ktpiCC23LadOxHm7p39Nk9jjYFxV5cFXpdsFrw1kwl6VdC8LDp3eGu"
      "Ku1GCqj5H2fpnkmL2goD01HRkPR3ro4uBHPtTXDbCIz0qp+NGlGG4gPUysMXxHSb"
      "E5FIWeUx6gcPvidwrpkCQQD40FXY46KDJT8JVYJMqY6nFQZvptFl+9BGWfheVVSF"
      "KBlTQBx/QA+XcC/W9Q/I+NEhdGcxLlkEMUpihSpYffKbAkEA2wmFfccdheTtoOuY"
      "8oTurbnFHsS7gLtcR2IbRJKXw80CJxTQA/LMWz0YuFOAYJNl/9ILMfp6MQiI4L9F"
      "l6pbtwJAJqkAXcXo72WvKL0flNfXsYBj0p9h8+2vi+7Y15d8nYAAh13zz5XdllM5"
      "K7ZCMKDwpbkXe53O+QbLnwk/7iYLtwJAERT6AygfJk0HNzCIeglh78x4EgE3uj9i"
      "X/LHu55PFacMTu3xlw09YLQwFFf2wBFeuAeyddBZ7S8ENbrU+5H+mwJBAO2E6gwG"
      "e5ZqY4RmsQmv6K0rn5k+UT4qlPeVp1e6LnvO/PcKWOaUvDK59qFZoX4vN+iFUAbk"
      "IuvhmL9u/uPWWck="
      "-----END PRIVATE KEY-----";
  static const std::vector<uint8_t> kBadKeyData(5, 111);
  static const char kTestData[] = "disco boy";
  static const char kEmptyData[] = "";

  std::string public_key_string;
  base::Base64Decode(kPublicKey, &public_key_string);
  std::vector<uint8_t> public_key(public_key_string.begin(),
                                  public_key_string.end());
  std::string plain;
  std::vector<uint8_t> encrypted_output;

  // Checking basic encryption operation.
  plain = kTestData;
  EXPECT_TRUE(networking_private_crypto::EncryptByteString(
      public_key, plain, &encrypted_output));
  EXPECT_TRUE(VerifyByteString(kPrivateKey, plain, encrypted_output));

  // Checking that we can encrypt the empty string.
  plain = kEmptyData;
  EXPECT_TRUE(networking_private_crypto::EncryptByteString(
      public_key, plain, &encrypted_output));

  // Checking graceful fail for too much data to encrypt.
  EXPECT_FALSE(networking_private_crypto::EncryptByteString(
      public_key, std::string(500, 'x'), &encrypted_output));

  // Checking graceful fail for a bad key format.
  EXPECT_FALSE(networking_private_crypto::EncryptByteString(
      kBadKeyData, kTestData, &encrypted_output));
}
