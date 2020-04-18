// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_CERTIFICATE_PATTERN_H_
#define CHROMEOS_NETWORK_CERTIFICATE_PATTERN_H_

#include <memory>
#include <string>
#include <vector>

#include "chromeos/chromeos_export.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

// Class to represent the DER fields of an issuer or a subject in a
// certificate and compare them.
class CHROMEOS_EXPORT IssuerSubjectPattern {
 public:
  IssuerSubjectPattern();
  IssuerSubjectPattern(const std::string& common_name,
                       const std::string& locality,
                       const std::string& organization,
                       const std::string& organizational_unit);
  IssuerSubjectPattern(const IssuerSubjectPattern& other);
  ~IssuerSubjectPattern();

  // Returns true if all fields in the pattern are empty.
  bool Empty() const;

  // Clears out all values in this pattern.
  void Clear();

  const std::string& common_name() const {
    return common_name_;
  }
  const std::string& locality() const {
    return locality_;
  }
  const std::string& organization() const {
    return organization_;
  }
  const std::string& organizational_unit() const {
    return organizational_unit_;
  }

  // Replaces the content of this object with the values of |dictionary|.
  // |dictionary| should be a valid ONC IssuerSubjectPattern dictionary.
  void ReadFromONCDictionary(const base::DictionaryValue& dictionary);

 private:
  std::string common_name_;
  std::string locality_;
  std::string organization_;
  std::string organizational_unit_;
};

// A class to contain a certificate pattern and find existing matches to the
// pattern in the certificate database.
class CHROMEOS_EXPORT CertificatePattern {
 public:
  CertificatePattern();
  CertificatePattern(const CertificatePattern& other);
  ~CertificatePattern();

  // Returns true if this pattern has nothing set (and so would match
  // all certs).  Ignores enrollment_uri_;
  bool Empty() const;

  const IssuerSubjectPattern& issuer() const {
    return issuer_;
  }
  const IssuerSubjectPattern& subject() const {
    return subject_;
  }
  const std::vector<std::string>& issuer_ca_pems() const {
    return issuer_ca_pems_;
  }
  const std::vector<std::string>& enrollment_uri_list() const {
    return enrollment_uri_list_;
  }

  // Replaces the content of this object with the values of |dictionary|.
  // |dictionary| should be a valid ONC CertificatePattern dictionary. Returns
  // whether all required fields were present.
  bool ReadFromONCDictionary(const base::DictionaryValue& dictionary);

 private:
  // Clears out all the values in this pattern.
  void Clear();

  std::vector<std::string> issuer_ca_pems_;
  IssuerSubjectPattern issuer_;
  IssuerSubjectPattern subject_;
  std::vector<std::string> enrollment_uri_list_;
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_CERTIFICATE_PATTERN_H_
