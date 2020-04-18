// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/forced_extensions/installation_tracker.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/pref_names.h"

namespace {
// Timeout to report UMA if not all force-installed extension were loaded.
constexpr base::TimeDelta kInstallationTimeout =
    base::TimeDelta::FromMinutes(5);
}  // namespace

namespace extensions {

InstallationTracker::InstallationTracker(ExtensionRegistry* registry,
                                         PrefService* pref_service,
                                         std::unique_ptr<base::Timer> timer)
    : registry_(registry),
      pref_service_(pref_service),
      start_time_(base::Time::Now()),
      observer_(this),
      timer_(std::move(timer)) {
  observer_.Add(registry_);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      pref_names::kInstallForceList,
      base::BindRepeating(&InstallationTracker::OnForcedExtensionsPrefChanged,
                          base::Unretained(this)));

  timer_->Start(
      FROM_HERE, kInstallationTimeout,
      base::BindRepeating(&InstallationTracker::ReportResults,
                          base::Unretained(this), false /* succeeded */));

  // Try to load list now.
  OnForcedExtensionsPrefChanged();
}

InstallationTracker::~InstallationTracker() = default;

void InstallationTracker::OnForcedExtensionsPrefChanged() {
  // Load forced extensions list only once.
  if (!forced_extensions_.empty())
    return;

  const base::DictionaryValue* value =
      pref_service_->GetDictionary(pref_names::kInstallForceList);
  if (!value || value->empty())
    return;

  std::unique_ptr<ExtensionSet> installed_extensions =
      registry_->GenerateInstalledExtensionsSet();

  for (const auto& entry : *value) {
    forced_extensions_.insert(entry.first);
    if (!installed_extensions->Contains(entry.first))
      pending_forced_extensions_.insert(entry.first);
  }
  if (pending_forced_extensions_.empty())
    ReportResults(true /* succeeded */);
}

void InstallationTracker::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (pending_forced_extensions_.erase(extension->id()) &&
      pending_forced_extensions_.empty()) {
    ReportResults(true /* succeeded */);
  }
}

void InstallationTracker::ReportResults(bool succeeded) {
  DCHECK(!reported_);
  // Report only if there was non-empty list of force-installed extensions.
  if (!forced_extensions_.empty()) {
    if (succeeded) {
      UMA_HISTOGRAM_LONG_TIMES("Extensions.ForceInstalledLoadTime",
                               base::Time::Now() - start_time_);
    } else {
      UMA_HISTOGRAM_COUNTS_100("Extensions.ForceInstalledTimedOutCount",
                               pending_forced_extensions_.size());
    }
  }
  reported_ = true;
  observer_.RemoveAll();
  pref_change_registrar_.RemoveAll();
  timer_->Stop();
}

}  //  namespace extensions
