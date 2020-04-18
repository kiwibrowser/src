// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_field_trial_creator.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "android_webview/browser/aw_metrics_service_client.h"
#include "android_webview/browser/aw_variations_seed_bridge.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "cc/base/switches.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/pref_names.h"
#include "components/variations/seed_response.h"
#include "components/variations/service/safe_seed_manager.h"
#include "components/variations/service/variations_service.h"

namespace android_webview {
namespace {

// TODO(kmilka): Update to work properly in environments both with and without
// UMA enabled.
std::unique_ptr<const base::FieldTrial::EntropyProvider>
CreateLowEntropyProvider(const std::string& client_id) {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      // Since variations are only enabled for users opted in to UMA, it is
      // acceptable to use the SHA1EntropyProvider for randomization.
      new variations::SHA1EntropyProvider(client_id));
}

// This experiment is for testing and doesn't control any features. Log it so QA
// can check whether variations is working.
// TODO(crbug/841623): Remove this after launch.
void LogTestExperiment() {
  static const char* const test_name = "First-WebView-Experiment";
  base::FieldTrial* test_trial = base::FieldTrialList::Find(test_name);
  if (test_trial) {
    LOG(INFO) << test_name << " found, group=" << test_trial->group_name();
  } else {
    LOG(INFO) << test_name << " not found";
  }
}

}  // anonymous namespace

AwFieldTrialCreator::AwFieldTrialCreator()
    : aw_field_trials_(std::make_unique<AwFieldTrials>()) {}

AwFieldTrialCreator::~AwFieldTrialCreator() {}

void AwFieldTrialCreator::SetUpFieldTrials() {
  DoSetUpFieldTrials();

  // If DoSetUpFieldTrials failed, it might have skipped creating
  // FeatureList. If so, create a FeatureList without field trials.
  if (!base::FeatureList::GetInstance()) {
    const base::CommandLine* command_line =
        base::CommandLine::ForCurrentProcess();
    auto feature_list = std::make_unique<base::FeatureList>();
    feature_list->InitializeFromCommandLine(
        command_line->GetSwitchValueASCII(switches::kEnableFeatures),
        command_line->GetSwitchValueASCII(switches::kDisableFeatures));
    base::FeatureList::SetInstance(std::move(feature_list));
  }
}

void AwFieldTrialCreator::DoSetUpFieldTrials() {
  // If the client ID isn't available yet, don't delay startup by creating it.
  // Instead, variations will be disabled for this run.
  std::string client_id;
  if (!AwMetricsServiceClient::GetPreloadedClientId(&client_id))
    return;

  DCHECK(!field_trial_list_);
  field_trial_list_ = std::make_unique<base::FieldTrialList>(
      CreateLowEntropyProvider(client_id));

  variations::UIStringOverrider ui_string_overrider;
  client_ = std::make_unique<AwVariationsServiceClient>();
  variations_field_trial_creator_ =
      std::make_unique<variations::VariationsFieldTrialCreator>(
          GetLocalState(), client_.get(), ui_string_overrider,
          GetAndClearJavaSeed());
  variations_field_trial_creator_->OverrideVariationsPlatform(
      variations::Study::PLATFORM_ANDROID_WEBVIEW);

  // Unused by WebView, but required by
  // VariationsFieldTrialCreator::SetupFieldTrials().
  // TODO(isherman): We might want a more genuine SafeSeedManager:
  // https://crbug.com/801771
  std::set<std::string> unforceable_field_trials;
  variations::SafeSeedManager ignored_safe_seed_manager(true, GetLocalState());
  // Populates the FieldTrialList singleton via the static member functions.
  variations_field_trial_creator_->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials,
      std::vector<std::string>(), CreateLowEntropyProvider(client_id),
      std::make_unique<base::FeatureList>(), aw_field_trials_.get(),
      &ignored_safe_seed_manager);

  LogTestExperiment();
}

PrefService* AwFieldTrialCreator::GetLocalState() {
  if (!local_state_) {
    scoped_refptr<PrefRegistrySimple> pref_registry =
        base::MakeRefCounted<PrefRegistrySimple>();
    variations::VariationsService::RegisterPrefs(pref_registry.get());

    PrefServiceFactory factory;
    factory.set_user_prefs(base::MakeRefCounted<InMemoryPrefStore>());
    local_state_ = factory.Create(pref_registry.get());
  }
  return local_state_.get();
}

}  // namespace android_webview
