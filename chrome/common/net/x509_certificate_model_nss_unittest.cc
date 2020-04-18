// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/net/x509_certificate_model_nss.h"

#include <stddef.h>

#include "base/files/file_path.h"
#include "crypto/scoped_test_nss_db.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/scoped_nss_types.h"
#include "net/cert/x509_util_nss.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(X509CertificateModelTest, GetCertNameOrNicknameAndGetTitle) {
  net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "root_ca_cert.pem"));
  ASSERT_TRUE(cert.get());
  EXPECT_EQ("Test Root CA",
            x509_certificate_model::GetCertNameOrNickname(cert.get()));

  net::ScopedCERTCertificate punycode_cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "punycodetest.pem"));
  ASSERT_TRUE(punycode_cert.get());
  EXPECT_EQ("xn--wgv71a119e.com (日本語.com)",
            x509_certificate_model::GetCertNameOrNickname(punycode_cert.get()));

  net::ScopedCERTCertificate no_cn_cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "no_subject_common_name_cert.pem"));
  ASSERT_TRUE(no_cn_cert.get());
  // Temp cert has no nickname.
  EXPECT_EQ("",
            x509_certificate_model::GetCertNameOrNickname(no_cn_cert.get()));

  EXPECT_EQ("xn--wgv71a119e.com",
            x509_certificate_model::GetTitle(punycode_cert.get()));

  EXPECT_EQ("E=wtc@google.com",
            x509_certificate_model::GetTitle(no_cn_cert.get()));

  net::ScopedCERTCertificate no_cn_cert2(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "ct-test-embedded-cert.pem"));
  ASSERT_TRUE(no_cn_cert2.get());
  EXPECT_EQ("L=Erw Wen,ST=Wales,O=Certificate Transparency,C=GB",
            x509_certificate_model::GetTitle(no_cn_cert2.get()));
}

TEST(X509CertificateModelTest, GetExtensions) {
  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "root_ca_cert.pem"));
    ASSERT_TRUE(cert.get());

    x509_certificate_model::Extensions extensions;
    x509_certificate_model::GetExtensions("critical", "notcrit", cert.get(),
                                          &extensions);
    ASSERT_EQ(3U, extensions.size());

    EXPECT_EQ("Certificate Basic Constraints", extensions[0].name);
    EXPECT_EQ(
        "critical\nIs a Certification Authority\n"
        "Maximum number of intermediate CAs: unlimited",
        extensions[0].value);

    EXPECT_EQ("Certificate Subject Key ID", extensions[1].name);
    EXPECT_EQ(
        "notcrit\nKey ID: 9B 26 0B 8A 98 A9 BB 1D B9 1F 1C E3 1A 40 33 ED\n8E "
        "17 88 AB",
        extensions[1].value);

    EXPECT_EQ("Certificate Key Usage", extensions[2].name);
    EXPECT_EQ("critical\nCertificate Signer\nCRL Signer", extensions[2].value);
  }

  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "subjectAltName_sanity_check.pem"));
    ASSERT_TRUE(cert.get());

    x509_certificate_model::Extensions extensions;
    x509_certificate_model::GetExtensions("critical", "notcrit", cert.get(),
                                          &extensions);
    ASSERT_EQ(2U, extensions.size());
    EXPECT_EQ("Certificate Subject Alternative Name", extensions[1].name);
    EXPECT_EQ(
        "notcrit\nIP Address: 127.0.0.2\nIP Address: fe80::1\nDNS Name: "
        "test.example\nEmail Address: test@test.example\nOID.1.2.3.4: 0C 09 69 "
        "67 6E 6F 72 65 20 6D 65\nX.500 Name: CN = 127.0.0.3\n\n",
        extensions[1].value);
  }

  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "foaf.me.chromium-test-cert.der"));
    ASSERT_TRUE(cert.get());

    x509_certificate_model::Extensions extensions;
    x509_certificate_model::GetExtensions("critical", "notcrit", cert.get(),
                                          &extensions);
    ASSERT_EQ(5U, extensions.size());
    EXPECT_EQ("Netscape Certificate Comment", extensions[1].name);
    EXPECT_EQ("notcrit\nOpenSSL Generated Certificate", extensions[1].value);
  }

  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "2029_globalsign_com_cert.pem"));
    ASSERT_TRUE(cert.get());

    x509_certificate_model::Extensions extensions;
    x509_certificate_model::GetExtensions("critical", "notcrit", cert.get(),
                                          &extensions);
    ASSERT_EQ(9U, extensions.size());

    EXPECT_EQ("Certificate Subject Key ID", extensions[0].name);
    EXPECT_EQ(
        "notcrit\nKey ID: 59 BC D9 69 F7 B0 65 BB C8 34 C5 D2 C2 EF 17 78\nA6 "
        "47 1E 8B",
        extensions[0].value);

    EXPECT_EQ("Certification Authority Key ID", extensions[1].name);
    EXPECT_EQ(
        "notcrit\nKey ID: 8A FC 14 1B 3D A3 59 67 A5 3B E1 73 92 A6 62 91\n7F "
        "E4 78 30\n",
        extensions[1].value);

    EXPECT_EQ("Authority Information Access", extensions[2].name);
    EXPECT_EQ(
        "notcrit\nCA Issuers: "
        "URI: http://secure.globalsign.net/cacert/SHA256extendval1.crt\n",
        extensions[2].value);

    EXPECT_EQ("CRL Distribution Points", extensions[3].name);
    EXPECT_EQ("notcrit\nURI: http://crl.globalsign.net/SHA256ExtendVal1.crl\n",
              extensions[3].value);

    EXPECT_EQ("Certificate Basic Constraints", extensions[4].name);
    EXPECT_EQ("notcrit\nIs not a Certification Authority\n",
              extensions[4].value);

    EXPECT_EQ("Certificate Key Usage", extensions[5].name);
    EXPECT_EQ(
        "critical\nSigning\nNon-repudiation\nKey Encipherment\n"
        "Data Encipherment",
        extensions[5].value);

    EXPECT_EQ("Extended Key Usage", extensions[6].name);
    EXPECT_EQ(
        "notcrit\nTLS WWW Server Authentication (OID.1.3.6.1.5.5.7.3.1)\n"
        "TLS WWW Client Authentication (OID.1.3.6.1.5.5.7.3.2)\n",
        extensions[6].value);

    EXPECT_EQ("Certificate Policies", extensions[7].name);
    EXPECT_EQ(
        "notcrit\nOID.1.3.6.1.4.1.4146.1.1:\n"
        "  Certification Practice Statement Pointer:"
        "    http://www.globalsign.net/repository/\n",
        extensions[7].value);

    EXPECT_EQ("Netscape Certificate Type", extensions[8].name);
    EXPECT_EQ("notcrit\nSSL Client Certificate\nSSL Server Certificate",
              extensions[8].value);
  }

  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "diginotar_public_ca_2025.pem"));
    ASSERT_TRUE(cert.get());

    x509_certificate_model::Extensions extensions;
    x509_certificate_model::GetExtensions("critical", "notcrit", cert.get(),
                                          &extensions);
    ASSERT_EQ(7U, extensions.size());

    EXPECT_EQ("Authority Information Access", extensions[0].name);
    EXPECT_EQ(
        "notcrit\nOCSP Responder: "
        "URI: http://validation.diginotar.nl\n",
        extensions[0].value);

    EXPECT_EQ("Certificate Basic Constraints", extensions[2].name);
    EXPECT_EQ(
        "critical\nIs a Certification Authority\n"
        "Maximum number of intermediate CAs: 0",
        extensions[2].value);
    EXPECT_EQ("Certificate Policies", extensions[3].name);
    EXPECT_EQ(
        "notcrit\nOID.2.16.528.1.1001.1.1.1.1.5.2.6.4:\n"
        "  Certification Practice Statement Pointer:"
        "    http://www.diginotar.nl/cps\n"
        "  User Notice:\n"
        "    Conditions, as mentioned on our website (www.diginotar.nl), are "
        "applicable to all our products and services.\n",
        extensions[3].value);
  }
}

TEST(X509CertificateModelTest, GetTypeCA) {
  net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "root_ca_cert.pem"));
  ASSERT_TRUE(cert.get());

  EXPECT_EQ(net::CA_CERT, x509_certificate_model::GetType(cert.get()));

  crypto::ScopedTestNSSDB test_nssdb;
  net::NSSCertDatabase db(crypto::ScopedPK11Slot(PK11_ReferenceSlot(
                              test_nssdb.slot())) /* public slot */,
                          crypto::ScopedPK11Slot(PK11_ReferenceSlot(
                              test_nssdb.slot())) /* private slot */);

  // Test that explicitly distrusted CA certs are still returned as CA_CERT
  // type. See http://crbug.com/96654.
  EXPECT_TRUE(db.SetCertTrust(cert.get(), net::CA_CERT,
                              net::NSSCertDatabase::DISTRUSTED_SSL));

  EXPECT_EQ(net::CA_CERT, x509_certificate_model::GetType(cert.get()));
}

TEST(X509CertificateModelTest, GetTypeServer) {
  net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "google.single.der"));
  ASSERT_TRUE(cert.get());

  // Test mozilla_security_manager::GetCertType with server certs and default
  // trust.  Currently this doesn't work.
  // TODO(mattm): make mozilla_security_manager::GetCertType smarter so we can
  // tell server certs even if they have no trust bits set.
  EXPECT_EQ(net::OTHER_CERT, x509_certificate_model::GetType(cert.get()));

  crypto::ScopedTestNSSDB test_nssdb;
  net::NSSCertDatabase db(crypto::ScopedPK11Slot(PK11_ReferenceSlot(
                              test_nssdb.slot())) /* public slot */,
                          crypto::ScopedPK11Slot(PK11_ReferenceSlot(
                              test_nssdb.slot())) /* private slot */);

  // Test GetCertType with server certs and explicit trust.
  EXPECT_TRUE(db.SetCertTrust(cert.get(), net::SERVER_CERT,
                              net::NSSCertDatabase::TRUSTED_SSL));

  EXPECT_EQ(net::SERVER_CERT, x509_certificate_model::GetType(cert.get()));

  // Test GetCertType with server certs and explicit distrust.
  EXPECT_TRUE(db.SetCertTrust(cert.get(), net::SERVER_CERT,
                              net::NSSCertDatabase::DISTRUSTED_SSL));

  EXPECT_EQ(net::SERVER_CERT, x509_certificate_model::GetType(cert.get()));
}

// An X.509 v1 certificate with the version field omitted should get
// the default value v1.
TEST(X509CertificateModelTest, GetVersionOmitted) {
  net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "ndn.ca.crt"));
  ASSERT_TRUE(cert.get());

  EXPECT_EQ("1", x509_certificate_model::GetVersion(cert.get()));
}

TEST(X509CertificateModelTest, GetCMSString) {
  net::ScopedCERTCertificateList certs = CreateCERTCertificateListFromFile(
      net::GetTestCertsDirectory(), "multi-root-chain1.pem",
      net::X509Certificate::FORMAT_AUTO);

  {
    // Write the full chain.
    std::string pkcs7_string =
        x509_certificate_model::GetCMSString(certs, 0, certs.size());

    ASSERT_FALSE(pkcs7_string.empty());

    net::ScopedCERTCertificateList decoded_certs =
        net::x509_util::CreateCERTCertificateListFromBytes(
            pkcs7_string.data(), pkcs7_string.size(),
            net::X509Certificate::FORMAT_PKCS7);

    ASSERT_EQ(certs.size(), decoded_certs.size());

    // NSS sorts the certs before writing the file.
    EXPECT_TRUE(net::x509_util::IsSameCertificate(certs[0].get(),
                                                  decoded_certs.back().get()));
    for (size_t i = 1; i < certs.size(); ++i)
      EXPECT_TRUE(net::x509_util::IsSameCertificate(
          certs[i].get(), decoded_certs[i - 1].get()));
  }

  {
    // Write only the first cert.
    std::string pkcs7_string =
        x509_certificate_model::GetCMSString(certs, 0, 1);

    net::ScopedCERTCertificateList decoded_certs =
        net::x509_util::CreateCERTCertificateListFromBytes(
            pkcs7_string.data(), pkcs7_string.size(),
            net::X509Certificate::FORMAT_PKCS7);

    ASSERT_EQ(1U, decoded_certs.size());
    EXPECT_TRUE(net::x509_util::IsSameCertificate(certs[0].get(),
                                                  decoded_certs[0].get()));
  }
}

TEST(X509CertificateModelTest, ProcessSecAlgorithms) {
  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "root_ca_cert.pem"));
    ASSERT_TRUE(cert.get());

    EXPECT_EQ("PKCS #1 SHA-256 With RSA Encryption",
              x509_certificate_model::ProcessSecAlgorithmSignature(cert.get()));
    EXPECT_EQ(
        "PKCS #1 SHA-256 With RSA Encryption",
        x509_certificate_model::ProcessSecAlgorithmSignatureWrap(cert.get()));
    EXPECT_EQ("PKCS #1 RSA Encryption",
              x509_certificate_model::ProcessSecAlgorithmSubjectPublicKey(
                  cert.get()));
  }
  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "weak_digest_md5_root.pem"));
    ASSERT_TRUE(cert.get());

    EXPECT_EQ("PKCS #1 MD5 With RSA Encryption",
              x509_certificate_model::ProcessSecAlgorithmSignature(cert.get()));
    EXPECT_EQ(
        "PKCS #1 MD5 With RSA Encryption",
        x509_certificate_model::ProcessSecAlgorithmSignatureWrap(cert.get()));
    EXPECT_EQ("PKCS #1 RSA Encryption",
              x509_certificate_model::ProcessSecAlgorithmSubjectPublicKey(
                  cert.get()));
  }
}

TEST(X509CertificateModelTest, ProcessSubjectPublicKeyInfo) {
  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "root_ca_cert.pem"));
    ASSERT_TRUE(cert.get());

    EXPECT_EQ(
        "Modulus (2048 bits):\n"
        "  C6 81 1F 92 73 B6 58 85 D9 8D AC B7 20 FD C7 BF\n"
        "40 B2 EA FA E5 0B 52 01 8F 9A C1 EB 7A 80 C1 F3\n"
        "89 A4 3E D5 1B 61 CC B5 CF 80 B1 1A DB BB 25 E0\n"
        "18 BF 92 69 26 50 CD E7 3F FF 0D 3C B4 1F 14 12\n"
        "AB 67 37 DE 07 03 6C 12 74 82 36 AC C3 D4 D3 64\n"
        "9F 91 ED 5B F6 A9 7A A4 9C 98 E8 65 6C 94 E1 CB\n"
        "55 73 AE F8 1D 50 B0 78 E5 74 FF B1 37 2C CB 19\n"
        "3D A4 8C E7 76 4E 86 5C 3F DF B3 ED 45 23 4F 54\n"
        "9B 33 C6 89 5E 13 1D DD 7D 59 A5 07 34 28 86 27\n"
        "1F FA 9E 53 4F 2A B6 42 AD 37 12 62 F5 72 36 B6\n"
        "02 12 40 44 FE C7 9E 95 89 43 51 5E B4 6E C7 67\n"
        "80 58 43 BE CC 07 28 BD 59 FF 1C 4C 8D 90 42 F4\n"
        "CF FD 54 00 4F 48 72 2B E1 67 3C 84 17 68 95 BF\n"
        "CA 07 7B DF 86 9D 56 E3 32 E3 70 87 B7 F8 3A F7\n"
        "E3 6E 65 14 7C BB 76 B7 17 F1 42 8C 6F 2A 34 64\n"
        "10 35 14 8C 85 F6 57 BF F3 5C 55 9D AD 03 10 F3\n"
        "\n"
        "  Public Exponent (24 bits):\n"
        "  01 00 01",
        x509_certificate_model::ProcessSubjectPublicKeyInfo(cert.get()));
  }
  {
    net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
        net::GetTestCertsDirectory(), "prime256v1-ecdsa-intermediate.pem"));
    ASSERT_TRUE(cert.get());

    EXPECT_EQ(
        "04 D5 C1 4A 32 95 95 C5 88 FA 01 FA C5 9E DC E2\n"
        "99 62 EB 13 E5 35 42 B3 7A FC 46 C0 FA 29 12 C8\n"
        "2D EA 30 0F D2 9A 47 97 2C 7E 89 E6 EF 49 55 06\n"
        "C9 37 C7 99 56 16 B2 2B C9 7C 69 8E 10 7A DD 1F\n"
        "42",
        x509_certificate_model::ProcessSubjectPublicKeyInfo(cert.get()));
  }
}

TEST(X509CertificateModelTest, ProcessRawBitsSignatureWrap) {
  net::ScopedCERTCertificate cert(net::ImportCERTCertificateFromFile(
      net::GetTestCertsDirectory(), "root_ca_cert.pem"));
  ASSERT_TRUE(cert.get());

  EXPECT_EQ(
      "5B 53 FF 6D D5 0A 43 A5 0F D4 7D C6 5D 88 E3 98\n"
      "9D 67 EB 32 82 B3 0F F5 C1 78 F8 05 4A BF BC 21\n"
      "05 EE 21 08 2C B2 15 A1 B8 B2 F6 A3 15 61 E4 C1\n"
      "AD 84 A4 A7 40 0C 87 09 5F 2B 1B F9 4D 6C 92 7D\n"
      "CB 7E 2B B0 01 0A ED 40 E5 4E AF 1A F1 0D EC 1D\n"
      "9E 96 C7 D4 61 64 39 23 FA 5F 29 C4 2A 3A B8 ED\n"
      "8A 72 50 6A AC 45 04 76 09 A8 3D 57 D7 F0 4B AE\n"
      "46 B4 83 C1 14 50 2A 19 59 53 B2 4D AE FC 2F 40\n"
      "49 C8 AD 4D 9D C8 22 8D 8C 01 DB 31 33 5A F4 BC\n"
      "4C 9B ED D7 E3 43 D9 E8 1D 53 8B 30 D8 81 9E 72\n"
      "AB 9E CE B8 F5 83 93 F2 72 DB DE CD B0 52 9A 45\n"
      "4D CF E7 21 D8 CE 16 64 8F 42 AF C1 87 A8 F9 D5\n"
      "E2 03 DD BA 6B 1B 7C 7D A0 38 33 61 39 B4 DD 5C\n"
      "69 17 79 02 3A EC 1D 6F 5E BB 13 FB A6 82 5D 07\n"
      "20 FC 86 FE 6E 8B AC E1 C2 18 A2 FE 3F 95 66 D3\n"
      "69 8A 00 06 2C 56 37 34 B9 B6 31 DE 0F F6 44 39",
      x509_certificate_model::ProcessRawBitsSignatureWrap(cert.get()));
}
