// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/certificate_pattern.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/values.h"
#include "components/onc/onc_constants.h"

namespace chromeos {

namespace {

bool GetAsListOfStrings(const base::Value& value,
                        std::vector<std::string>* result) {
  const base::ListValue* list = NULL;
  if (!value.GetAsList(&list))
    return false;
  result->clear();
  result->reserve(list->GetSize());
  for (size_t i = 0; i < list->GetSize(); i++) {
    std::string item;
    if (!list->GetString(i, &item))
      return false;
    result->push_back(item);
  }
  return true;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// IssuerSubjectPattern
IssuerSubjectPattern::IssuerSubjectPattern(
    const std::string& common_name,
    const std::string& locality,
    const std::string& organization,
    const std::string& organizational_unit)
    : common_name_(common_name),
      locality_(locality),
      organization_(organization),
      organizational_unit_(organizational_unit) {
}

IssuerSubjectPattern::IssuerSubjectPattern() = default;

IssuerSubjectPattern::IssuerSubjectPattern(const IssuerSubjectPattern& other) =
    default;

IssuerSubjectPattern::~IssuerSubjectPattern() = default;

bool IssuerSubjectPattern::Empty() const {
  return common_name_.empty() && locality_.empty() && organization_.empty() &&
         organizational_unit_.empty();
}

void IssuerSubjectPattern::Clear() {
  common_name_.clear();
  locality_.clear();
  organization_.clear();
  organizational_unit_.clear();
}

void IssuerSubjectPattern::ReadFromONCDictionary(
    const base::DictionaryValue& dict) {
  Clear();

  dict.GetStringWithoutPathExpansion(onc::client_cert::kCommonName,
                                     &common_name_);
  dict.GetStringWithoutPathExpansion(onc::client_cert::kLocality, &locality_);
  dict.GetStringWithoutPathExpansion(onc::client_cert::kOrganization,
                                     &organization_);
  dict.GetStringWithoutPathExpansion(onc::client_cert::kOrganizationalUnit,
                                     &organizational_unit_);
}

////////////////////////////////////////////////////////////////////////////////
// CertificatePattern

CertificatePattern::CertificatePattern() = default;

CertificatePattern::CertificatePattern(const CertificatePattern& other) =
    default;

CertificatePattern::~CertificatePattern() = default;

bool CertificatePattern::Empty() const {
  return issuer_ca_pems_.empty() && issuer_.Empty() && subject_.Empty();
}

void CertificatePattern::Clear() {
  issuer_ca_pems_.clear();
  issuer_.Clear();
  subject_.Clear();
  enrollment_uri_list_.clear();
}

bool CertificatePattern::ReadFromONCDictionary(
    const base::DictionaryValue& dict) {
  Clear();

  const base::DictionaryValue* child_dict = NULL;
  const base::ListValue* child_list = NULL;

  // All of these are optional.
  if (dict.GetListWithoutPathExpansion(onc::client_cert::kIssuerCAPEMs,
                                       &child_list) &&
      child_list) {
    if (!GetAsListOfStrings(*child_list, &issuer_ca_pems_))
      return false;
  }
  if (dict.GetDictionaryWithoutPathExpansion(onc::client_cert::kIssuer,
                                             &child_dict) &&
      child_dict) {
    issuer_.ReadFromONCDictionary(*child_dict);
  }
  child_dict = NULL;
  if (dict.GetDictionaryWithoutPathExpansion(onc::client_cert::kSubject,
                                             &child_dict) &&
      child_dict) {
    subject_.ReadFromONCDictionary(*child_dict);
  }
  child_list = NULL;
  if (dict.GetListWithoutPathExpansion(onc::client_cert::kEnrollmentURI,
                                       &child_list) &&
      child_list) {
    if (!GetAsListOfStrings(*child_list, &enrollment_uri_list_))
      return false;
  }

  return true;
}

}  // namespace chromeos
