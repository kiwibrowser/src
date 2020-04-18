// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/persisted_data.h"

#include <string>
#include <vector>

#include "base/guid.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/update_client/activity_data_service.h"

const char kPersistedDataPreference[] = "updateclientdata";

namespace update_client {

PersistedData::PersistedData(PrefService* pref_service,
                             ActivityDataService* activity_data_service)
    : pref_service_(pref_service),
      activity_data_service_(activity_data_service) {}

PersistedData::~PersistedData() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

int PersistedData::GetInt(const std::string& id,
                          const std::string& key,
                          int fallback) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We assume ids do not contain '.' characters.
  DCHECK_EQ(std::string::npos, id.find('.'));
  if (!pref_service_)
    return fallback;
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kPersistedDataPreference);
  if (!dict)
    return fallback;
  int result = 0;
  return dict->GetInteger(
             base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()), &result)
             ? result
             : fallback;
}

std::string PersistedData::GetString(const std::string& id,
                                     const std::string& key) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We assume ids do not contain '.' characters.
  DCHECK_EQ(std::string::npos, id.find('.'));
  if (!pref_service_)
    return std::string();
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kPersistedDataPreference);
  if (!dict)
    return std::string();
  std::string result;
  return dict->GetString(
             base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()), &result)
             ? result
             : std::string();
}

int PersistedData::GetDateLastRollCall(const std::string& id) const {
  return GetInt(id, "dlrc", kDateUnknown);
}

int PersistedData::GetDateLastActive(const std::string& id) const {
  return GetInt(id, "dla", kDateUnknown);
}

std::string PersistedData::GetPingFreshness(const std::string& id) const {
  std::string result = GetString(id, "pf");
  return !result.empty() ? base::StringPrintf("{%s}", result.c_str()) : result;
}

std::string PersistedData::GetCohort(const std::string& id) const {
  return GetString(id, "cohort");
}

std::string PersistedData::GetCohortName(const std::string& id) const {
  return GetString(id, "cohortname");
}

std::string PersistedData::GetCohortHint(const std::string& id) const {
  return GetString(id, "cohorthint");
}

void PersistedData::SetDateLastRollCall(const std::vector<std::string>& ids,
                                        int datenum) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_ || datenum < 0)
    return;
  DictionaryPrefUpdate update(pref_service_, kPersistedDataPreference);
  for (const auto& id : ids) {
    // We assume ids do not contain '.' characters.
    DCHECK_EQ(std::string::npos, id.find('.'));
    update->SetInteger(base::StringPrintf("apps.%s.dlrc", id.c_str()), datenum);
    update->SetString(base::StringPrintf("apps.%s.pf", id.c_str()),
                      base::GenerateGUID());
  }
}

void PersistedData::SetDateLastActive(const std::vector<std::string>& ids,
                                      int datenum) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_ || datenum < 0)
    return;
  DictionaryPrefUpdate update(pref_service_, kPersistedDataPreference);
  for (const auto& id : ids) {
    if (GetActiveBit(id)) {
      // We assume ids do not contain '.' characters.
      DCHECK_EQ(std::string::npos, id.find('.'));
      update->SetInteger(base::StringPrintf("apps.%s.dla", id.c_str()),
                         datenum);
      activity_data_service_->ClearActiveBit(id);
    }
  }
}

void PersistedData::SetString(const std::string& id,
                              const std::string& key,
                              const std::string& value) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_)
    return;
  DictionaryPrefUpdate update(pref_service_, kPersistedDataPreference);
  update->SetString(base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()),
                    value);
}

void PersistedData::SetCohort(const std::string& id,
                              const std::string& cohort) {
  SetString(id, "cohort", cohort);
}

void PersistedData::SetCohortName(const std::string& id,
                                  const std::string& cohort_name) {
  SetString(id, "cohortname", cohort_name);
}

void PersistedData::SetCohortHint(const std::string& id,
                                  const std::string& cohort_hint) {
  SetString(id, "cohorthint", cohort_hint);
}

bool PersistedData::GetActiveBit(const std::string& id) const {
  return activity_data_service_ && activity_data_service_->GetActiveBit(id);
}

int PersistedData::GetDaysSinceLastRollCall(const std::string& id) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return activity_data_service_
             ? activity_data_service_->GetDaysSinceLastRollCall(id)
             : kDaysUnknown;
}

int PersistedData::GetDaysSinceLastActive(const std::string& id) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return activity_data_service_
             ? activity_data_service_->GetDaysSinceLastActive(id)
             : kDaysUnknown;
}

void PersistedData::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kPersistedDataPreference);
}

}  // namespace update_client
