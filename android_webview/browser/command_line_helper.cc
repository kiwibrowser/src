// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/command_line_helper.h"

#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

using std::string;
using std::vector;

namespace {

// Adds |feature_name| to the list of features in |feature_list_name|, but only
// if the |feature_name| is absent from the list of features in both
// |feature_list_name| and |other_feature_list_name|.
void AddFeatureToList(base::CommandLine& command_line,
                      const string& feature_name,
                      const char* feature_list_name,
                      const char* other_feature_list_name) {
  const string features_list =
      command_line.GetSwitchValueASCII(feature_list_name);
  const string other_features_list =
      command_line.GetSwitchValueASCII(other_feature_list_name);

  if (features_list.empty() && other_features_list.empty()) {
    command_line.AppendSwitchASCII(feature_list_name, feature_name);
    return;
  }

  vector<base::StringPiece> features =
      base::FeatureList::SplitFeatureListString(features_list);
  vector<base::StringPiece> other_features =
      base::FeatureList::SplitFeatureListString(other_features_list);

  if (!base::ContainsValue(features, feature_name) &&
      !base::ContainsValue(other_features, feature_name)) {
    features.push_back(feature_name);
    command_line.AppendSwitchASCII(feature_list_name,
                                   base::JoinString(features, ","));
  }
}

}  // namespace

// static
void CommandLineHelper::AddEnabledFeature(base::CommandLine& command_line,
                                          const string& feature_name) {
  AddFeatureToList(command_line, feature_name, switches::kEnableFeatures,
                   switches::kDisableFeatures);
}

// static
void CommandLineHelper::AddDisabledFeature(base::CommandLine& command_line,
                                           const string& feature_name) {
  AddFeatureToList(command_line, feature_name, switches::kDisableFeatures,
                   switches::kEnableFeatures);
}
