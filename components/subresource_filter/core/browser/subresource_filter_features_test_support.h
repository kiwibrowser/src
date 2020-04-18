// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_SUBRESOURCE_FILTER_FEATURES_TEST_SUPPORT_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_SUBRESOURCE_FILTER_FEATURES_TEST_SUPPORT_H_

#include <iosfwd>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"

namespace subresource_filter {
namespace testing {

// Helper class to override the active subresource filtering configuration to be
// used in tests while the instance is in scope.
//
// Configuration overrides can be nested, and will take effect regardless of
// field trial, feature, and/or variation parameter states.
class ScopedSubresourceFilterConfigurator {
 public:
  explicit ScopedSubresourceFilterConfigurator(
      scoped_refptr<ConfigurationList> config_list = nullptr);
  explicit ScopedSubresourceFilterConfigurator(Configuration config);
  explicit ScopedSubresourceFilterConfigurator(
      std::vector<Configuration> configs);
  ~ScopedSubresourceFilterConfigurator();

  void ResetConfiguration(
      scoped_refptr<ConfigurationList> config_list = nullptr);
  void ResetConfiguration(Configuration config);
  void ResetConfiguration(std::vector<Configuration> config);

 private:
  scoped_refptr<ConfigurationList> original_config_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSubresourceFilterConfigurator);
};

// Helper class to override the state of the |kSafeBrowsingSubresourceFilter|
// feature.
//
// Clears the active subresource filtering configuration override upon
// construction, if any, and restores it on destruction. So while the instance
// is in scope, calls to GetEnabledConfigurations() will default to returning
// the hard-coded configuration corresponding to the forced feature state. Tests
// that need to toggle both the feature and override the active configuration
// should therefore do so in that order.
class ScopedSubresourceFilterFeatureToggle {
 public:
  ScopedSubresourceFilterFeatureToggle();
  explicit ScopedSubresourceFilterFeatureToggle(
      base::FeatureList::OverrideState feature_state,
      const std::string& additional_features_to_enable = std::string());
  ~ScopedSubresourceFilterFeatureToggle();

  void ResetSubresourceFilterState(
      base::FeatureList::OverrideState feature_state,
      const std::string& additional_features_to_enable = std::string());

 private:
  ScopedSubresourceFilterConfigurator scoped_configuration_;
  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSubresourceFilterFeatureToggle);
};

// For logging in tests.
std::ostream& operator<<(std::ostream& os, const Configuration& config);

}  // namespace testing
}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_BROWSER_SUBRESOURCE_FILTER_FEATURES_TEST_SUPPORT_H_
