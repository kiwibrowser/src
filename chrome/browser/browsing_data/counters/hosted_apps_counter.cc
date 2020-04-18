// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/hosted_apps_counter.h"

#include <algorithm>
#include <string>

#include "chrome/browser/profiles/profile.h"
#include "components/browsing_data/core/pref_names.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"

HostedAppsCounter::HostedAppsCounter(Profile* profile)
    : profile_(profile) {}

HostedAppsCounter::~HostedAppsCounter() {}

const char* HostedAppsCounter::GetPrefName() const {
  return browsing_data::prefs::kDeleteHostedAppsData;
}

void HostedAppsCounter::Count() {
  int count = 0;
  std::vector<std::string> names;

  std::unique_ptr<extensions::ExtensionSet> extensions =
      extensions::ExtensionRegistry::Get(profile_)
          ->GenerateInstalledExtensionsSet();

  for (const auto& extension : *extensions) {
    if (extension->is_hosted_app())
      names.push_back(extension->short_name());
  }

  count = names.size();

  // Give the first two names (alphabetically) as examples.
  std::sort(names.begin(), names.end());
  names.resize(std::min<size_t>(2u, names.size()));

  ReportResult(std::make_unique<HostedAppsResult>(this, count, names));
}

// HostedAppsCounter::HostedAppsResult -----------------------------------------

HostedAppsCounter::HostedAppsResult::HostedAppsResult(
    const HostedAppsCounter* source,
    ResultInt num_apps,
    const std::vector<std::string>& examples)
    : FinishedResult(source, num_apps), examples_(examples) {}

HostedAppsCounter::HostedAppsResult::~HostedAppsResult() {}
