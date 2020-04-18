// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/field_trial_config/field_trial_util.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "components/variations/field_trial_config/fieldtrial_testing_config.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/escape.h"

namespace variations {
namespace {

void AssociateParamsFromExperiment(
    const std::string& study_name,
    const FieldTrialTestingExperiment& experiment,
    base::FeatureList* feature_list) {
  if (experiment.params_size != 0) {
    std::map<std::string, std::string> params;
    for (size_t i = 0; i < experiment.params_size; ++i) {
      const FieldTrialTestingExperimentParams& param = experiment.params[i];
      params[param.key] = param.value;
    }
    AssociateVariationParams(study_name, experiment.name, params);
  }
  base::FieldTrial* trial =
      base::FieldTrialList::CreateFieldTrial(study_name, experiment.name);

  if (!trial) {
    DLOG(WARNING) << "Field trial config study skipped: " << study_name
                  << "." << experiment.name
                  << " (it is overridden from chrome://flags)";
    return;
  }

  for (size_t i = 0; i < experiment.enable_features_size; ++i) {
    feature_list->RegisterFieldTrialOverride(
        experiment.enable_features[i],
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, trial);
  }
  for (size_t i = 0; i < experiment.disable_features_size; ++i) {
    feature_list->RegisterFieldTrialOverride(
        experiment.disable_features[i],
        base::FeatureList::OVERRIDE_DISABLE_FEATURE, trial);
  }
}

// Chooses an experiment taking into account the command line. Defaults to
// picking the first experiment.
const FieldTrialTestingExperiment& ChooseExperiment(
    const FieldTrialTestingExperiment experiments[],
    size_t experiment_size) {
  DCHECK_GT(experiment_size, 0ul);
  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  size_t chosen_index = 0;
  for (size_t i = 1; i < experiment_size && chosen_index == 0; ++i) {
    const auto& experiment = experiments[i];
    if (experiment.forcing_flag &&
        command_line.HasSwitch(experiment.forcing_flag)) {
      chosen_index = i;
      break;
    }
  }
  DCHECK_GT(experiment_size, chosen_index);
  return experiments[chosen_index];
}

} // namespace

std::string UnescapeValue(const std::string& value) {
  return net::UnescapeURLComponent(
      value, net::UnescapeRule::PATH_SEPARATORS |
                 net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);
}

std::string EscapeValue(const std::string& value) {
  // This needs to be the inverse of UnescapeValue in the anonymous namespace
  // above.
  std::string net_escaped_str =
      net::EscapeQueryParamValue(value, true /* use_plus */);

  // net doesn't escape '.' and '*' but UnescapeValue() covers those cases.
  std::string escaped_str;
  escaped_str.reserve(net_escaped_str.length());
  for (const char ch : net_escaped_str) {
    if (ch == '.')
      escaped_str.append("%2E");
    else if (ch == '*')
      escaped_str.append("%2A");
    else
      escaped_str.push_back(ch);
  }
  return escaped_str;
}

bool AssociateParamsFromString(const std::string& varations_string) {
  // Format: Trial1.Group1:k1/v1/k2/v2,Trial2.Group2:k1/v1/k2/v2
  std::set<std::pair<std::string, std::string>> trial_groups;
  for (const base::StringPiece& experiment_group : base::SplitStringPiece(
           varations_string, ",",
           base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    std::vector<base::StringPiece> experiment = base::SplitStringPiece(
        experiment_group, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (experiment.size() != 2) {
      DLOG(ERROR) << "Experiment and params should be separated by ':'";
      return false;
    }

    std::vector<std::string> group_parts = base::SplitString(
        experiment[0], ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (group_parts.size() != 2) {
      DLOG(ERROR) << "Trial and group name should be separated by '.'";
      return false;
    }

    std::vector<std::string> key_values = base::SplitString(
        experiment[1], "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (key_values.size() % 2 != 0) {
      DLOG(ERROR) << "Param name and param value should be separated by '/'";
      return false;
    }
    std::string trial = UnescapeValue(group_parts[0]);
    std::string group = UnescapeValue(group_parts[1]);
    auto trial_group = std::make_pair(trial, group);
    if (trial_groups.find(trial_group) != trial_groups.end()) {
      DLOG(ERROR) << base::StringPrintf(
          "A (trial, group) pair listed more than once. (%s, %s)",
          trial.c_str(), group.c_str());
      return false;
    }
    trial_groups.insert(trial_group);
    std::map<std::string, std::string> params;
    for (size_t i = 0; i < key_values.size(); i += 2) {
      std::string key = UnescapeValue(key_values[i]);
      std::string value = UnescapeValue(key_values[i + 1]);
      params[key] = value;
    }
    bool result = AssociateVariationParams(trial, group, params);
    if (!result) {
      DLOG(ERROR) << "Failed to associate variation params for group \""
                  << group << "\" in trial \"" << trial << "\"";
      return false;
    }
  }
  return true;
}

void AssociateParamsFromFieldTrialConfig(const FieldTrialTestingConfig& config,
                                         base::FeatureList* feature_list) {
  for (size_t i = 0; i < config.studies_size; ++i) {
    const FieldTrialTestingStudy& study = config.studies[i];
    if (study.experiments_size > 0) {
      AssociateParamsFromExperiment(
          study.name,
          ChooseExperiment(study.experiments, study.experiments_size),
          feature_list);
    } else {
      DLOG(ERROR) << "Unexpected empty study: " << study.name;
    }
  }
}

void AssociateDefaultFieldTrialConfig(base::FeatureList* feature_list) {
  AssociateParamsFromFieldTrialConfig(kFieldTrialConfig, feature_list);
}

}  // namespace variations
