// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Make sure stdint.h includes SIZE_MAX. (See C89, p259, footnote 221.)
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include "components/domain_reliability/config.h"

#include <stdint.h>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"

namespace {

bool ConvertURL(const base::Value* value, GURL* url) {
  std::string url_string;
  if (!value->GetAsString(&url_string))
    return false;
  *url = GURL(url_string);
  return url->is_valid();
}

bool ConvertOrigin(const base::Value* value, GURL* url) {
  return ConvertURL(value, url) && !url->has_username() &&
         !url->has_password() && url->SchemeIs("https") &&
         url->path_piece() == "/" && !url->has_query() && !url->has_ref();
}

bool IsValidSampleRate(double p) {
  return p >= 0.0 && p <= 1.0;
}

}  // namespace

namespace domain_reliability {

DomainReliabilityConfig::DomainReliabilityConfig()
    : include_subdomains(false),
      success_sample_rate(-1.0),
      failure_sample_rate(-1.0) {
}
DomainReliabilityConfig::~DomainReliabilityConfig() {}

// static
std::unique_ptr<const DomainReliabilityConfig>
DomainReliabilityConfig::FromJSON(const base::StringPiece& json) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
  base::JSONValueConverter<DomainReliabilityConfig> converter;
  std::unique_ptr<DomainReliabilityConfig> config(
      new DomainReliabilityConfig());

  // If we can parse and convert the JSON into a valid config, return that.
  if (value && converter.Convert(*value, config.get()) && config->IsValid())
    return std::move(config);
  return std::unique_ptr<const DomainReliabilityConfig>();
}

bool DomainReliabilityConfig::IsValid() const {
  if (!origin.is_valid() || collectors.empty() ||
      !IsValidSampleRate(success_sample_rate) ||
      !IsValidSampleRate(failure_sample_rate)) {
    return false;
  }

  for (const auto& url : collectors) {
    if (!url->is_valid())
      return false;
  }

  return true;
}

bool DomainReliabilityConfig::Equals(const DomainReliabilityConfig& other)
    const {
  if (include_subdomains != other.include_subdomains ||
      collectors.size() != other.collectors.size() ||
      success_sample_rate != other.success_sample_rate ||
      failure_sample_rate != other.failure_sample_rate ||
      path_prefixes.size() != other.path_prefixes.size()) {
    return false;
  }

  for (size_t i = 0; i < collectors.size(); ++i)
    if (*collectors[i] != *other.collectors[i])
      return false;

  for (size_t i = 0; i < path_prefixes.size(); ++i)
    if (*path_prefixes[i] != *other.path_prefixes[i])
      return false;

  return true;
}

double DomainReliabilityConfig::GetSampleRate(bool request_successful) const {
  return request_successful ? success_sample_rate : failure_sample_rate;
}

// static
void DomainReliabilityConfig::RegisterJSONConverter(
    base::JSONValueConverter<DomainReliabilityConfig>* converter) {
  converter->RegisterCustomValueField<GURL>(
      "origin", &DomainReliabilityConfig::origin, &ConvertOrigin);
  converter->RegisterBoolField("include_subdomains",
                               &DomainReliabilityConfig::include_subdomains);
  converter->RegisterRepeatedCustomValue(
      "collectors", &DomainReliabilityConfig::collectors, &ConvertURL);
  converter->RegisterRepeatedString("path_prefixes",
                                    &DomainReliabilityConfig::path_prefixes);
  converter->RegisterDoubleField("success_sample_rate",
                                 &DomainReliabilityConfig::success_sample_rate);
  converter->RegisterDoubleField("failure_sample_rate",
                                 &DomainReliabilityConfig::failure_sample_rate);
}

}  // namespace domain_reliability
