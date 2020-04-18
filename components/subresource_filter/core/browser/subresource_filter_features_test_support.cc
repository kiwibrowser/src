// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"

#include <ostream>
#include <utility>

#include "base/json/json_writer.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event_argument.h"
#include "base/values.h"

namespace subresource_filter {
namespace testing {

// ScopedSubresourceFilterConfigurator ----------------------------------------

ScopedSubresourceFilterConfigurator::ScopedSubresourceFilterConfigurator(
    scoped_refptr<ConfigurationList> configs_list)
    : original_config_(GetAndSetActivateConfigurations(configs_list)) {}

ScopedSubresourceFilterConfigurator::ScopedSubresourceFilterConfigurator(
    Configuration config)
    : ScopedSubresourceFilterConfigurator(
          std::vector<Configuration>(1, std::move(config))) {}

ScopedSubresourceFilterConfigurator::ScopedSubresourceFilterConfigurator(
    std::vector<Configuration> configs)
    : ScopedSubresourceFilterConfigurator(
          base::MakeRefCounted<ConfigurationList>(std::move(configs))) {}

ScopedSubresourceFilterConfigurator::~ScopedSubresourceFilterConfigurator() {
  GetAndSetActivateConfigurations(std::move(original_config_));
}

void ScopedSubresourceFilterConfigurator::ResetConfiguration(
    scoped_refptr<ConfigurationList> configs_list) {
  GetAndSetActivateConfigurations(configs_list);
}

void ScopedSubresourceFilterConfigurator::ResetConfiguration(
    Configuration config) {
  ResetConfiguration(std::vector<Configuration>(1, std::move(config)));
}

void ScopedSubresourceFilterConfigurator::ResetConfiguration(
    std::vector<Configuration> config) {
  ResetConfiguration(
      base::MakeRefCounted<ConfigurationList>(std::move(config)));
}

// ScopedSubresourceFilterFeatureToggle ---------------------------------------

ScopedSubresourceFilterFeatureToggle::ScopedSubresourceFilterFeatureToggle() {}
ScopedSubresourceFilterFeatureToggle::ScopedSubresourceFilterFeatureToggle(
    base::FeatureList::OverrideState feature_state,
    const std::string& additional_features_to_enable) {
  ResetSubresourceFilterState(feature_state, additional_features_to_enable);
}

void ScopedSubresourceFilterFeatureToggle::ResetSubresourceFilterState(
    base::FeatureList::OverrideState feature_state,
    const std::string& additional_features_to_enable) {
  std::string enabled_features;
  std::string disabled_features;

  if (feature_state == base::FeatureList::OVERRIDE_ENABLE_FEATURE) {
    enabled_features = kSafeBrowsingSubresourceFilter.name;
  } else if (feature_state == base::FeatureList::OVERRIDE_DISABLE_FEATURE) {
    disabled_features = kSafeBrowsingSubresourceFilter.name;
  }

  if (!additional_features_to_enable.empty()) {
    if (!enabled_features.empty())
      enabled_features += ',';
    enabled_features += additional_features_to_enable;
  }

  scoped_configuration_.ResetConfiguration();
  scoped_feature_list_ = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_list_->InitFromCommandLine(enabled_features,
                                            disabled_features);
}

ScopedSubresourceFilterFeatureToggle::~ScopedSubresourceFilterFeatureToggle() {}

std::ostream& operator<<(std::ostream& os, const Configuration& config) {
  std::unique_ptr<base::Value> value = config.ToTracedValue()->ToBaseValue();
  base::DictionaryValue* dict;
  value->GetAsDictionary(&dict);
  std::string json;
  base::JSONWriter::WriteWithOptions(
      *dict, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  return os << json;
}

}  // namespace testing
}  // namespace subresource_filter
