// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/flags_ui/flags_state.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/flags_ui/feature_entry.h"
#include "components/flags_ui/flags_storage.h"
#include "components/flags_ui/flags_ui_switches.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace flags_ui {

namespace internal {
const char kTrialGroupAboutFlags[] = "AboutFlags";
}  // namespace internal

namespace {

// Convert switch constants to proper CommandLine::StringType strings.
base::CommandLine::StringType GetSwitchString(const std::string& flag) {
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  cmd_line.AppendSwitch(flag);
  DCHECK_EQ(2U, cmd_line.argv().size());
  return cmd_line.argv()[1];
}

// Return the span between the first occurrence of |begin_sentinel_switch| and
// the last occurrence of |end_sentinel_switch|.
base::span<const base::CommandLine::StringType> GetSwitchesBetweenSentinels(
    const base::CommandLine::StringVector& switches,
    const base::CommandLine::StringType& begin_sentinel_switch,
    const base::CommandLine::StringType& end_sentinel_switch) {
  const auto first =
      std::find(switches.begin(), switches.end(), begin_sentinel_switch);
  if (first == switches.end())
    return {};
  // Go backwards in order to find the last occurrence (as opposed to
  // std::find() which would return the first one).
  for (auto last = --switches.end(); last != first; --last) {
    if (*last == end_sentinel_switch)
      return base::make_span(&first[1], last - first - 1);
  }
  return {};
}

// Scoops flags from a command line.
// Only switches between --flag-switches-begin and --flag-switches-end are
// compared. The embedder may use |extra_flag_sentinel_begin_flag_name| and
// |extra_sentinel_end_flag_name| to specify other delimiters, if supported.
std::set<base::CommandLine::StringType> ExtractFlagsFromCommandLine(
    const base::CommandLine& cmdline,
    const char* extra_flag_sentinel_begin_flag_name,
    const char* extra_flag_sentinel_end_flag_name) {
  DCHECK_EQ(!!extra_flag_sentinel_begin_flag_name,
            !!extra_flag_sentinel_end_flag_name);
  std::set<base::CommandLine::StringType> flags;
  // First do the ones between --flag-switches-begin and --flag-switches-end.
  const auto flags_span = GetSwitchesBetweenSentinels(
      cmdline.argv(), GetSwitchString(switches::kFlagSwitchesBegin),
      GetSwitchString(switches::kFlagSwitchesEnd));
  flags.insert(flags_span.begin(), flags_span.end());

  // Then add those between the extra sentinels.
  if (extra_flag_sentinel_begin_flag_name &&
      extra_flag_sentinel_end_flag_name) {
    const auto extra_flags_span = GetSwitchesBetweenSentinels(
        cmdline.argv(), GetSwitchString(extra_flag_sentinel_begin_flag_name),
        GetSwitchString(extra_flag_sentinel_end_flag_name));
    flags.insert(extra_flags_span.begin(), extra_flags_span.end());
  }
  return flags;
}

const struct {
  unsigned bit;
  const char* const name;
} kBitsToOs[] = {
    {kOsMac, "Mac"},
    {kOsWin, "Windows"},
    {kOsLinux, "Linux"},
    {kOsCrOS, "Chrome OS"},
    {kOsAndroid, "Android"},
    {kOsCrOSOwnerOnly, "Chrome OS (owner only)"},
    {kOsIos, "iOS"},
};

// Adds a |StringValue| to |list| for each platform where |bitmask| indicates
// whether the entry is available on that platform.
void AddOsStrings(unsigned bitmask, base::ListValue* list) {
  for (size_t i = 0; i < arraysize(kBitsToOs); ++i) {
    if (bitmask & kBitsToOs[i].bit)
      list->AppendString(kBitsToOs[i].name);
  }
}

// Adds the internal names for the specified entry to |names|.
void AddInternalName(const FeatureEntry& e, std::set<std::string>* names) {
  switch (e.type) {
    case FeatureEntry::SINGLE_VALUE:
    case FeatureEntry::SINGLE_DISABLE_VALUE:
    case FeatureEntry::ORIGIN_LIST_VALUE:
      names->insert(e.internal_name);
      break;
    case FeatureEntry::MULTI_VALUE:
    case FeatureEntry::ENABLE_DISABLE_VALUE:
    case FeatureEntry::FEATURE_VALUE:
    case FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
      for (int i = 0; i < e.num_options; ++i)
        names->insert(e.NameForOption(i));
      break;
  }
}

// Confirms that an entry is valid, used in a DCHECK in
// SanitizeList below.
bool ValidateFeatureEntry(const FeatureEntry& e) {
  switch (e.type) {
    case FeatureEntry::SINGLE_VALUE:
    case FeatureEntry::SINGLE_DISABLE_VALUE:
    case FeatureEntry::ORIGIN_LIST_VALUE:
      DCHECK_EQ(0, e.num_options);
      DCHECK(!e.choices);
      return true;
    case FeatureEntry::MULTI_VALUE:
      DCHECK_GT(e.num_options, 0);
      DCHECK(e.choices);
      DCHECK(e.ChoiceForOption(0).command_line_switch);
      DCHECK_EQ('\0', e.ChoiceForOption(0).command_line_switch[0]);
      return true;
    case FeatureEntry::ENABLE_DISABLE_VALUE:
      DCHECK_EQ(3, e.num_options);
      DCHECK(!e.choices);
      DCHECK(e.command_line_switch);
      DCHECK(e.command_line_value);
      DCHECK(e.disable_command_line_switch);
      DCHECK(e.disable_command_line_value);
      return true;
    case FeatureEntry::FEATURE_VALUE:
      DCHECK_EQ(3, e.num_options);
      DCHECK(!e.choices);
      DCHECK(e.feature);
      return true;
    case FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
      DCHECK_GT(e.num_options, 2);
      DCHECK(!e.choices);
      DCHECK(e.feature);
      DCHECK(e.feature_variations);
      DCHECK(e.feature_trial_name);
      return true;
  }
  NOTREACHED();
  return false;
}

// Returns true if none of this entry's options have been enabled.
bool IsDefaultValue(const FeatureEntry& entry,
                    const std::set<std::string>& enabled_entries) {
  switch (entry.type) {
    case FeatureEntry::SINGLE_VALUE:
    case FeatureEntry::SINGLE_DISABLE_VALUE:
    case FeatureEntry::ORIGIN_LIST_VALUE:
      return enabled_entries.count(entry.internal_name) == 0;
    case FeatureEntry::MULTI_VALUE:
    case FeatureEntry::ENABLE_DISABLE_VALUE:
    case FeatureEntry::FEATURE_VALUE:
    case FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
      for (int i = 0; i < entry.num_options; ++i) {
        if (enabled_entries.count(entry.NameForOption(i)) > 0)
          return false;
      }
      return true;
  }
  NOTREACHED();
  return true;
}

// Returns the Value representing the choice data in the specified entry.
std::unique_ptr<base::Value> CreateOptionsData(
    const FeatureEntry& entry,
    const std::set<std::string>& enabled_entries) {
  DCHECK(entry.type == FeatureEntry::MULTI_VALUE ||
         entry.type == FeatureEntry::ENABLE_DISABLE_VALUE ||
         entry.type == FeatureEntry::FEATURE_VALUE ||
         entry.type == FeatureEntry::FEATURE_WITH_PARAMS_VALUE);
  auto result = std::make_unique<base::ListValue>();
  for (int i = 0; i < entry.num_options; ++i) {
    auto value = std::make_unique<base::DictionaryValue>();
    const std::string name = entry.NameForOption(i);
    value->SetString("internal_name", name);
    value->SetString("description", entry.DescriptionForOption(i));
    value->SetBoolean("selected", enabled_entries.count(name) > 0);
    result->Append(std::move(value));
  }
  return std::move(result);
}

// Registers variation parameters specified by |feature_variation_params| for
// the field trial named |feature_trial_name|, unless a group for this trial has
// already been created (e.g. via command-line switches that take precedence
// over about:flags). In the trial, the function creates a new constant group
// called |kTrialGroupAboutFlags|.
base::FieldTrial* RegisterFeatureVariationParameters(
    const std::string& feature_trial_name,
    const std::map<std::string, std::string>& feature_variation_params) {
  bool success = variations::AssociateVariationParams(
      feature_trial_name, internal::kTrialGroupAboutFlags,
      feature_variation_params);
  if (!success)
    return nullptr;
  // Successful association also means that no group is created and selected
  // for the trial, yet. Thus, create the trial to select the group. This way,
  // the parameters cannot get overwritten in later phases (such as from the
  // server).
  base::FieldTrial* trial = base::FieldTrialList::CreateFieldTrial(
      feature_trial_name, internal::kTrialGroupAboutFlags);
  if (!trial) {
    DLOG(WARNING) << "Could not create the trial " << feature_trial_name
                  << " with group " << internal::kTrialGroupAboutFlags;
  }
  return trial;
}

// Returns true if |value| is safe to include in a command line string in the
// form of --flag=value.
bool IsSafeValue(const std::string& value) {
  // Punctuation characters at the end ("-", ".", ":", "/") are allowed because
  // origins can contain those (e.g. http://example.test). Comma is allowed
  // because it's used as the separator character.
  static const char kAllowedChars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789"
      "-.:/,";
  return value.find_first_not_of(kAllowedChars) == std::string::npos;
}

// Sanitizes |value| which contains a list of origins separated by whitespace
// and/or comma. The sanitized value is added as a command line argument, so
// this is a security critical operation: The sanitized value must have no
// whitespaces, each individual origin must be separated by a comma, and each
// origin must represent a url::Origin().
std::string SanitizeOriginListFlag(const std::string& value) {
  const std::string input = base::CollapseWhitespaceASCII(value, false);
  const std::string delimiters = " ,";
  base::StringTokenizer tokenizer(input, delimiters);
  std::vector<std::string> origin_strings;
  while (tokenizer.GetNext()) {
    const std::string token = tokenizer.token();
    if (token.empty()) {
      continue;
    }
    const GURL url(token);
    if (!url.is_valid() ||
        (!url.SchemeIsHTTPOrHTTPS() && !url.SchemeIsWSOrWSS())) {
      continue;
    }
    const std::string origin = url::Origin::Create(url).Serialize();
    if (!IsSafeValue(origin)) {
      continue;
    }
    origin_strings.push_back(origin);
  }
  const std::string result = base::JoinString(origin_strings, ",");
  CHECK(IsSafeValue(result));
  return result;
}

}  // namespace

struct FlagsState::SwitchEntry {
  // Corresponding base::Feature to toggle.
  std::string feature_name;

  // If |feature_name| is not empty, the state (enable/disabled) to set.
  bool feature_state;

  // The name of the switch to add.
  std::string switch_name;

  // If |switch_name| is not empty, the value of the switch to set.
  std::string switch_value;

  SwitchEntry() : feature_state(false) {}
};

FlagsState::FlagsState(const FeatureEntry* feature_entries,
                       size_t num_feature_entries)
    : feature_entries_(feature_entries),
      num_feature_entries_(num_feature_entries),
      needs_restart_(false) {}

FlagsState::~FlagsState() {}

void FlagsState::ConvertFlagsToSwitches(
    FlagsStorage* flags_storage,
    base::CommandLine* command_line,
    SentinelsMode sentinels,
    const char* enable_features_flag_name,
    const char* disable_features_flag_name) {
  std::set<std::string> enabled_entries;
  std::map<std::string, SwitchEntry> name_to_switch_map;
  GenerateFlagsToSwitchesMapping(flags_storage, &enabled_entries,
                                 &name_to_switch_map);
  AddSwitchesToCommandLine(enabled_entries, name_to_switch_map, sentinels,
                           command_line, enable_features_flag_name,
                           disable_features_flag_name);
}

std::set<std::string> FlagsState::GetSwitchesFromFlags(
    FlagsStorage* flags_storage) {
  std::set<std::string> enabled_entries;
  std::map<std::string, SwitchEntry> name_to_switch_map;
  GenerateFlagsToSwitchesMapping(flags_storage, &enabled_entries,
                                 &name_to_switch_map);

  std::set<std::string> switches;
  for (const std::string& entry_name : enabled_entries) {
    const auto& entry_it = name_to_switch_map.find(entry_name);
    DCHECK(entry_it != name_to_switch_map.end());

    const SwitchEntry& entry = entry_it->second;
    if (!entry.switch_name.empty())
      switches.insert("--" + entry.switch_name);
  }
  return switches;
}

std::set<std::string> FlagsState::GetFeaturesFromFlags(
    FlagsStorage* flags_storage) {
  std::set<std::string> enabled_entries;
  std::map<std::string, SwitchEntry> name_to_switch_map;
  GenerateFlagsToSwitchesMapping(flags_storage, &enabled_entries,
                                 &name_to_switch_map);

  std::set<std::string> features;
  for (const std::string& entry_name : enabled_entries) {
    const auto& entry_it = name_to_switch_map.find(entry_name);
    DCHECK(entry_it != name_to_switch_map.end());

    const SwitchEntry& entry = entry_it->second;
    if (!entry.feature_name.empty()) {
      if (entry.feature_state)
        features.insert(entry.feature_name + ":enabled");
      else
        features.insert(entry.feature_name + ":disabled");
    }
  }
  return features;
}

bool FlagsState::IsRestartNeededToCommitChanges() {
  return needs_restart_;
}

void FlagsState::SetFeatureEntryEnabled(FlagsStorage* flags_storage,
                                        const std::string& internal_name,
                                        bool enable) {
  size_t at_index = internal_name.find(testing::kMultiSeparator);
  if (at_index != std::string::npos) {
    DCHECK(enable);
    // We're being asked to enable a multi-choice entry. Disable the
    // currently selected choice.
    DCHECK_NE(at_index, 0u);
    const std::string entry_name = internal_name.substr(0, at_index);
    SetFeatureEntryEnabled(flags_storage, entry_name, false);

    // And enable the new choice, if it is not the default first choice.
    if (internal_name != entry_name + "@0") {
      std::set<std::string> enabled_entries;
      GetSanitizedEnabledFlags(flags_storage, &enabled_entries);
      needs_restart_ |= enabled_entries.insert(internal_name).second;
      flags_storage->SetFlags(enabled_entries);
    }
    return;
  }

  std::set<std::string> enabled_entries;
  GetSanitizedEnabledFlags(flags_storage, &enabled_entries);

  const FeatureEntry* e = FindFeatureEntryByName(internal_name);
  DCHECK(e);

  if (e->type == FeatureEntry::SINGLE_VALUE ||
      e->type == FeatureEntry::ORIGIN_LIST_VALUE) {
    if (enable)
      needs_restart_ |= enabled_entries.insert(internal_name).second;
    else
      needs_restart_ |= (enabled_entries.erase(internal_name) > 0);

    // If an origin list was enabled or disabled, update the command line flag.
    if (e->type == FeatureEntry::ORIGIN_LIST_VALUE)
      DidModifyOriginListFlag(*e, enable);

  } else if (e->type == FeatureEntry::SINGLE_DISABLE_VALUE) {
    if (!enable)
      needs_restart_ |= enabled_entries.insert(internal_name).second;
    else
      needs_restart_ |= (enabled_entries.erase(internal_name) > 0);
  } else {
    if (enable) {
      // Enable the first choice.
      needs_restart_ |= enabled_entries.insert(e->NameForOption(0)).second;
    } else {
      // Find the currently enabled choice and disable it.
      for (int i = 0; i < e->num_options; ++i) {
        std::string choice_name = e->NameForOption(i);
        if (enabled_entries.find(choice_name) != enabled_entries.end()) {
          needs_restart_ = true;
          enabled_entries.erase(choice_name);
          // Continue on just in case there's a bug and more than one
          // entry for this choice was enabled.
        }
      }
    }
  }

  flags_storage->SetFlags(enabled_entries);
}

void FlagsState::SetOriginListFlag(const std::string& internal_name,
                                   const std::string& value,
                                   FlagsStorage* flags_storage) {
  const FeatureEntry* entry = FindFeatureEntryByName(internal_name);
  DCHECK(entry);

  std::set<std::string> enabled_entries;
  GetSanitizedEnabledFlags(flags_storage, &enabled_entries);
  // const bool enabled =
  //    enabled_entries.find(entry->internal_name) != enabled_entries.end();
  const bool enabled = base::ContainsKey(enabled_entries, entry->internal_name);
  switch_values_[entry->command_line_switch] = value;
  DidModifyOriginListFlag(*entry, enabled);
}

void FlagsState::RemoveFlagsSwitches(
    base::CommandLine::SwitchMap* switch_list) {
  for (const auto& entry : flags_switches_)
    switch_list->erase(entry.first);

  // If feature entries were added to --enable-features= or --disable-features=
  // lists, remove them here while preserving existing values.
  for (const auto& entry : appended_switches_) {
    const auto& switch_name = entry.first;
    const auto& switch_added_values = entry.second;

    // The below is either a std::string or a base::string16 based on platform.
    const auto& existing_value = (*switch_list)[switch_name];
#if defined(OS_WIN)
    const std::string existing_value_utf8 = base::UTF16ToUTF8(existing_value);
#else
    const std::string& existing_value_utf8 = existing_value;
#endif

    std::vector<base::StringPiece> features =
        base::FeatureList::SplitFeatureListString(existing_value_utf8);
    std::vector<base::StringPiece> remaining_features;
    // For any featrue name in |features| that is not in |switch_added_values| -
    // i.e. it wasn't added by about_flags code, add it to |remaining_features|.
    for (const auto& feature : features) {
      if (!base::ContainsKey(switch_added_values, feature.as_string()))
        remaining_features.push_back(feature);
    }

    // Either remove the flag entirely if |remaining_features| is empty, or set
    // the new list.
    if (remaining_features.empty()) {
      switch_list->erase(switch_name);
    } else {
      std::string switch_value = base::JoinString(remaining_features, ",");
#if defined(OS_WIN)
      (*switch_list)[switch_name] = base::UTF8ToUTF16(switch_value);
#else
      (*switch_list)[switch_name] = switch_value;
#endif
    }
  }
}

void FlagsState::ResetAllFlags(FlagsStorage* flags_storage) {
  needs_restart_ = true;

  std::set<std::string> no_entries;
  flags_storage->SetFlags(no_entries);
}

void FlagsState::Reset() {
  needs_restart_ = false;
  flags_switches_.clear();
  appended_switches_.clear();
}

std::vector<std::string> FlagsState::RegisterAllFeatureVariationParameters(
    FlagsStorage* flags_storage,
    base::FeatureList* feature_list) {
  std::set<std::string> enabled_entries;
  GetSanitizedEnabledFlagsForCurrentPlatform(flags_storage, &enabled_entries);
  std::vector<std::string> variation_ids;
  std::map<std::string, std::set<std::string>> enabled_features_by_trial_name;
  std::map<std::string, std::map<std::string, std::string>>
      params_by_trial_name;

  // First collect all the data for each trial.
  for (size_t i = 0; i < num_feature_entries_; ++i) {
    const FeatureEntry& e = feature_entries_[i];
    if (e.type == FeatureEntry::FEATURE_WITH_PARAMS_VALUE) {
      for (int j = 0; j < e.num_options; ++j) {
        if (e.StateForOption(j) == FeatureEntry::FeatureState::ENABLED &&
            enabled_entries.count(e.NameForOption(j))) {
          std::string trial_name = e.feature_trial_name;
          // The user has chosen to enable the feature by this option.
          enabled_features_by_trial_name[trial_name].insert(e.feature->name);

          const FeatureEntry::FeatureVariation* variation =
              e.VariationForOption(j);
          if (!variation)
            continue;

          // The selected variation is non-default, collect its params & id.

          for (int i = 0; i < variation->num_params; ++i) {
            auto insert_result = params_by_trial_name[trial_name].insert(
                std::make_pair(variation->params[i].param_name,
                               variation->params[i].param_value));
            DCHECK(insert_result.second)
                << "Multiple values for the same parameter '"
                << variation->params[i].param_name
                << "' are specified in chrome://flags!";
          }
          if (variation->variation_id)
            variation_ids.push_back(variation->variation_id);
        }
      }
    }
  }

  // Now create the trials and associate the features to them.
  for (const auto& kv : enabled_features_by_trial_name) {
    const std::string& trial_name = kv.first;
    const std::set<std::string>& trial_features = kv.second;

    base::FieldTrial* field_trial = RegisterFeatureVariationParameters(
        trial_name, params_by_trial_name[trial_name]);
    if (!field_trial)
      continue;

    for (const std::string& feature_name : trial_features) {
      feature_list->RegisterFieldTrialOverride(
          feature_name,
          base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE,
          field_trial);
    }
  }

  return variation_ids;
}

void FlagsState::GetFlagFeatureEntries(
    FlagsStorage* flags_storage,
    FlagAccess access,
    base::ListValue* supported_entries,
    base::ListValue* unsupported_entries,
    base::Callback<bool(const FeatureEntry&)> skip_feature_entry) {
  std::set<std::string> enabled_entries;
  GetSanitizedEnabledFlags(flags_storage, &enabled_entries);

  int current_platform = GetCurrentPlatform();

  for (size_t i = 0; i < num_feature_entries_; ++i) {
    const FeatureEntry& entry = feature_entries_[i];
    if (skip_feature_entry.Run(entry))
      continue;

    std::unique_ptr<base::DictionaryValue> data(new base::DictionaryValue());
    data->SetString("internal_name", entry.internal_name);
    data->SetString("name", base::StringPiece(entry.visible_name));
    data->SetString("description",
                    base::StringPiece(entry.visible_description));

    auto supported_platforms = std::make_unique<base::ListValue>();
    AddOsStrings(entry.supported_platforms, supported_platforms.get());
    data->Set("supported_platforms", std::move(supported_platforms));
    // True if the switch is not currently passed.
    bool is_default_value = IsDefaultValue(entry, enabled_entries);
    data->SetBoolean("is_default", is_default_value);

    switch (entry.type) {
      case FeatureEntry::SINGLE_VALUE:
      case FeatureEntry::SINGLE_DISABLE_VALUE:
        data->SetBoolean(
            "enabled",
            (!is_default_value && entry.type == FeatureEntry::SINGLE_VALUE) ||
                (is_default_value &&
                 entry.type == FeatureEntry::SINGLE_DISABLE_VALUE));
        break;
      case FeatureEntry::ORIGIN_LIST_VALUE:
        data->SetBoolean("enabled", !is_default_value);
        switch_values_[entry.internal_name] =
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                entry.command_line_switch);
        data->SetString("origin_list_value",
                        switch_values_[entry.internal_name]);
        break;
      case FeatureEntry::MULTI_VALUE:
      case FeatureEntry::ENABLE_DISABLE_VALUE:
      case FeatureEntry::FEATURE_VALUE:
      case FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
        data->Set("options", CreateOptionsData(entry, enabled_entries));
        break;
    }

    bool supported = (entry.supported_platforms & current_platform) != 0;
#if defined(OS_CHROMEOS)
    if (access == kOwnerAccessToFlags &&
        (entry.supported_platforms & kOsCrOSOwnerOnly) != 0) {
      supported = true;
    }
#endif
    if (supported)
      supported_entries->Append(std::move(data));
    else
      unsupported_entries->Append(std::move(data));
  }
}

// static
int FlagsState::GetCurrentPlatform() {
#if defined(OS_IOS)  // Needs to be before the OS_MACOSX check.
  return kOsIos;
#elif defined(OS_MACOSX)
  return kOsMac;
#elif defined(OS_WIN)
  return kOsWin;
#elif defined(OS_CHROMEOS)  // Needs to be before the OS_LINUX check.
  return kOsCrOS;
#elif defined(OS_LINUX) || defined(OS_OPENBSD)
  return kOsLinux;
#elif defined(OS_ANDROID)
  return kOsAndroid;
#else
#error Unknown platform
#endif
}

// static
bool FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
    const base::CommandLine& new_cmdline,
    const base::CommandLine& active_cmdline,
    std::set<base::CommandLine::StringType>* out_difference,
    const char* extra_flag_sentinel_begin_flag_name,
    const char* extra_flag_sentinel_end_flag_name) {
  std::set<base::CommandLine::StringType> new_flags =
      ExtractFlagsFromCommandLine(new_cmdline,
                                  extra_flag_sentinel_begin_flag_name,
                                  extra_flag_sentinel_end_flag_name);
  std::set<base::CommandLine::StringType> active_flags =
      ExtractFlagsFromCommandLine(active_cmdline,
                                  extra_flag_sentinel_begin_flag_name,
                                  extra_flag_sentinel_end_flag_name);

  bool result = false;
  // Needed because std::equal doesn't check if the 2nd set is empty.
  if (new_flags.size() == active_flags.size()) {
    result =
        std::equal(new_flags.begin(), new_flags.end(), active_flags.begin());
  }

  if (out_difference && !result) {
    std::set_symmetric_difference(
        new_flags.begin(), new_flags.end(), active_flags.begin(),
        active_flags.end(),
        std::inserter(*out_difference, out_difference->begin()));
  }

  return result;
}

void FlagsState::AddSwitchMapping(
    const std::string& key,
    const std::string& switch_name,
    const std::string& switch_value,
    std::map<std::string, SwitchEntry>* name_to_switch_map) {
  DCHECK(!base::ContainsKey(*name_to_switch_map, key));

  SwitchEntry* entry = &(*name_to_switch_map)[key];
  entry->switch_name = switch_name;
  entry->switch_value = switch_value;
}

void FlagsState::AddFeatureMapping(
    const std::string& key,
    const std::string& feature_name,
    bool feature_state,
    std::map<std::string, SwitchEntry>* name_to_switch_map) {
  DCHECK(!base::ContainsKey(*name_to_switch_map, key));

  SwitchEntry* entry = &(*name_to_switch_map)[key];
  entry->feature_name = feature_name;
  entry->feature_state = feature_state;
}

void FlagsState::AddSwitchesToCommandLine(
    const std::set<std::string>& enabled_entries,
    const std::map<std::string, SwitchEntry>& name_to_switch_map,
    SentinelsMode sentinels,
    base::CommandLine* command_line,
    const char* enable_features_flag_name,
    const char* disable_features_flag_name) {
  std::map<std::string, bool> feature_switches;
  if (sentinels == kAddSentinels) {
    command_line->AppendSwitch(switches::kFlagSwitchesBegin);
    flags_switches_[switches::kFlagSwitchesBegin] = std::string();
  }

  for (const std::string& entry_name : enabled_entries) {
    const auto& entry_it = name_to_switch_map.find(entry_name);
    if (entry_it == name_to_switch_map.end()) {
      NOTREACHED();
      continue;
    }

    const FeatureEntry* feature_entry = FindFeatureEntryByName(entry_name);
    if (feature_entry &&
        feature_entry->type == FeatureEntry::ORIGIN_LIST_VALUE) {
      // This is not a feature value that can be enabled/disabled, it's a
      // command line argument that takes a list of origins. Skip it.
      continue;
    }

    const SwitchEntry& entry = entry_it->second;
    if (!entry.feature_name.empty()) {
      feature_switches[entry.feature_name] = entry.feature_state;
    } else if (!entry.switch_name.empty()) {
      command_line->AppendSwitchASCII(entry.switch_name, entry.switch_value);
      flags_switches_[entry.switch_name] = entry.switch_value;
    }
    // If an entry doesn't match either of the above, then it is likely the
    // default entry for a FEATURE_VALUE entry. Safe to ignore.
  }

  if (!feature_switches.empty()) {
    MergeFeatureCommandLineSwitch(feature_switches, enable_features_flag_name,
                                  true, command_line);
    MergeFeatureCommandLineSwitch(feature_switches, disable_features_flag_name,
                                  false, command_line);
  }

  if (sentinels == kAddSentinels) {
    command_line->AppendSwitch(switches::kFlagSwitchesEnd);
    flags_switches_[switches::kFlagSwitchesEnd] = std::string();
  }
}

void FlagsState::MergeFeatureCommandLineSwitch(
    const std::map<std::string, bool>& feature_switches,
    const char* switch_name,
    bool feature_state,
    base::CommandLine* command_line) {
  std::string original_switch_value =
      command_line->GetSwitchValueASCII(switch_name);
  std::vector<base::StringPiece> features =
      base::FeatureList::SplitFeatureListString(original_switch_value);
  // Only add features that don't already exist in the lists.
  // Note: The base::ContainsValue() call results in O(n^2) performance, but in
  // practice n should be very small.
  for (const auto& entry : feature_switches) {
    if (entry.second == feature_state &&
        !base::ContainsValue(features, entry.first)) {
      features.push_back(entry.first);
      appended_switches_[switch_name].insert(entry.first);
    }
  }
  // Update the switch value only if it didn't change. This avoids setting an
  // empty list or duplicating the same list (since AppendSwitch() adds the
  // switch to the end but doesn't remove previous ones).
  std::string switch_value = base::JoinString(features, ",");
  if (switch_value != original_switch_value)
    command_line->AppendSwitchASCII(switch_name, switch_value);
}

void FlagsState::SanitizeList(FlagsStorage* flags_storage) {
  std::set<std::string> known_entries;
  for (size_t i = 0; i < num_feature_entries_; ++i) {
    DCHECK(ValidateFeatureEntry(feature_entries_[i]));
    AddInternalName(feature_entries_[i], &known_entries);
  }

  std::set<std::string> enabled_entries = flags_storage->GetFlags();

  std::set<std::string> new_enabled_entries =
      base::STLSetIntersection<std::set<std::string>>(known_entries,
                                                      enabled_entries);

  if (new_enabled_entries != enabled_entries)
    flags_storage->SetFlags(new_enabled_entries);
}

void FlagsState::GetSanitizedEnabledFlags(FlagsStorage* flags_storage,
                                          std::set<std::string>* result) {
  SanitizeList(flags_storage);
  *result = flags_storage->GetFlags();
}

void FlagsState::GetSanitizedEnabledFlagsForCurrentPlatform(
    FlagsStorage* flags_storage,
    std::set<std::string>* result) {
  GetSanitizedEnabledFlags(flags_storage, result);

  // Filter out any entries that aren't enabled on the current platform.  We
  // don't remove these from prefs else syncing to a platform with a different
  // set of entries would be lossy.
  std::set<std::string> platform_entries;
  int current_platform = GetCurrentPlatform();
  for (size_t i = 0; i < num_feature_entries_; ++i) {
    const FeatureEntry& entry = feature_entries_[i];
    if (entry.supported_platforms & current_platform)
      AddInternalName(entry, &platform_entries);
#if defined(OS_CHROMEOS)
    if (feature_entries_[i].supported_platforms & kOsCrOSOwnerOnly)
      AddInternalName(entry, &platform_entries);
#endif
  }

  std::set<std::string> new_enabled_entries =
      base::STLSetIntersection<std::set<std::string>>(platform_entries,
                                                      *result);

  result->swap(new_enabled_entries);
}

void FlagsState::GenerateFlagsToSwitchesMapping(
    FlagsStorage* flags_storage,
    std::set<std::string>* enabled_entries,
    std::map<std::string, SwitchEntry>* name_to_switch_map) {
  GetSanitizedEnabledFlagsForCurrentPlatform(flags_storage, enabled_entries);

  for (size_t i = 0; i < num_feature_entries_; ++i) {
    const FeatureEntry& e = feature_entries_[i];
    switch (e.type) {
      case FeatureEntry::SINGLE_VALUE:
      case FeatureEntry::SINGLE_DISABLE_VALUE:
        AddSwitchMapping(e.internal_name, e.command_line_switch,
                         e.command_line_value, name_to_switch_map);
        break;

      case FeatureEntry::ORIGIN_LIST_VALUE: {
        const std::string value =
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                e.command_line_switch);
        AddSwitchMapping(e.internal_name, e.command_line_switch, value,
                         name_to_switch_map);
        break;
      }

      case FeatureEntry::MULTI_VALUE:
        for (int j = 0; j < e.num_options; ++j) {
          AddSwitchMapping(
              e.NameForOption(j), e.ChoiceForOption(j).command_line_switch,
              e.ChoiceForOption(j).command_line_value, name_to_switch_map);
        }
        break;

      case FeatureEntry::ENABLE_DISABLE_VALUE:
        AddSwitchMapping(e.NameForOption(0), std::string(), std::string(),
                         name_to_switch_map);
        AddSwitchMapping(e.NameForOption(1), e.command_line_switch,
                         e.command_line_value, name_to_switch_map);
        AddSwitchMapping(e.NameForOption(2), e.disable_command_line_switch,
                         e.disable_command_line_value, name_to_switch_map);
        break;

      case FeatureEntry::FEATURE_VALUE:
      case FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
        for (int j = 0; j < e.num_options; ++j) {
          FeatureEntry::FeatureState state = e.StateForOption(j);
          if (state == FeatureEntry::FeatureState::DEFAULT) {
            AddFeatureMapping(e.NameForOption(j), std::string(), false,
                              name_to_switch_map);
          } else {
            AddFeatureMapping(e.NameForOption(j), e.feature->name,
                              state == FeatureEntry::FeatureState::ENABLED,
                              name_to_switch_map);
          }
        }
        break;
    }
  }
}

void FlagsState::DidModifyOriginListFlag(const FeatureEntry& entry,
                                         bool enabled) {
  // Remove the switch if it exists.
  base::CommandLine* current_cl = base::CommandLine::ForCurrentProcess();
  base::CommandLine new_cl(current_cl->GetProgram());
  const base::CommandLine::SwitchMap switches = current_cl->GetSwitches();
  for (const auto& it : switches) {
    const auto& switch_name = it.first;
    const auto& switch_value = it.second;
    if (switch_name != entry.command_line_switch) {
      if (switch_value.empty()) {
        new_cl.AppendSwitch(switch_name);
      } else {
        new_cl.AppendSwitchNative(switch_name, switch_value);
      }
    }
  }
  *current_cl = new_cl;

  if (enabled) {
    current_cl->AppendSwitchASCII(
        entry.command_line_switch,
        SanitizeOriginListFlag(switch_values_[entry.command_line_switch]));
  }
}

const FeatureEntry* FlagsState::FindFeatureEntryByName(
    const std::string& internal_name) const {
  for (size_t i = 0; i < num_feature_entries_; ++i) {
    if (feature_entries_[i].internal_name == internal_name) {
      return feature_entries_ + i;
    }
  }
  return nullptr;
}

}  // namespace flags_ui
