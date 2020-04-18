// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/notification_promo.h"

#include <stddef.h>

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"

namespace ios {

namespace {

const char kNTPPromoFinchExperiment[] = "IOSNTPPromotion";

// The name of the preference that stores the promotion object.
const char kPrefPromoObject[] = "ios.ntppromo";

// Keys in the kPrefPromoObject dictionary; used only here.
const char kPrefPromoFirstViewTime[] = "first_view_time";
const char kPrefPromoViews[] = "views";
const char kPrefPromoClosed[] = "closed";

}  // namespace

NotificationPromo::NotificationPromo(PrefService* local_state)
    : local_state_(local_state),
      promo_payload_(new base::DictionaryValue()),
      start_(0.0),
      end_(0.0),
      promo_id_(-1),
      max_views_(0),
      max_seconds_(0),
      first_view_time_(0),
      views_(0),
      closed_(false) {
  DCHECK(local_state_);
}

NotificationPromo::~NotificationPromo() {}

void NotificationPromo::InitFromVariations() {
  std::map<std::string, std::string> params;
  if (!variations::GetVariationParams(kNTPPromoFinchExperiment, &params)) {
    return;
  }

  // Build dictionary of parameters to pass to |InitFromJson|. Some parameters
  // are stored in a payload subdictionary, so two dictionaries are
  // built: one to represent the payload and one to represent all of the
  // paremeters. The payload is then added to the overall dictionary. This code
  // can be removed if this class is refactored and the payload is then
  // disregarded (crbug.com/608525).
  base::DictionaryValue json;
  base::DictionaryValue payload;
  std::map<std::string, std::string>::iterator iter;
  for (iter = params.begin(); iter != params.end(); ++iter) {
    int converted_number;
    bool converted = base::StringToInt(iter->second, &converted_number);
    // Choose the dictionary to which parameter is added based on whether the
    // parameter belongs in the payload or not.
    base::DictionaryValue& json_or_payload =
        IsPayloadParam(iter->first) ? payload : json;
    if (converted) {
      json_or_payload.SetInteger(iter->first, converted_number);
    } else {
      json_or_payload.SetString(iter->first, iter->second);
    }
  }
  json.SetKey("payload", payload.Clone());

  InitFromJson(json);
}

void NotificationPromo::InitFromJson(const base::DictionaryValue& promo) {
  std::string time_str;
  base::Time time;
  if (promo.GetString("start", &time_str) &&
      base::Time::FromString(time_str.c_str(), &time)) {
    start_ = time.ToDoubleT();
    DVLOG(1) << "start str=" << time_str
             << ", start_=" << base::NumberToString(start_);
  }
  if (promo.GetString("end", &time_str) &&
      base::Time::FromString(time_str.c_str(), &time)) {
    end_ = time.ToDoubleT();
    DVLOG(1) << "end str =" << time_str
             << ", end_=" << base::NumberToString(end_);
  }

  promo.GetString("promo_text", &promo_text_);
  DVLOG(1) << "promo_text_=" << promo_text_;

  const base::DictionaryValue* payload = NULL;
  if (promo.GetDictionary("payload", &payload)) {
    promo_payload_.reset(payload->DeepCopy());
  }

  promo.GetInteger("max_views", &max_views_);
  DVLOG(1) << "max_views_ " << max_views_;

  promo.GetInteger("max_seconds", &max_seconds_);
  DVLOG(1) << "max_seconds_ " << max_seconds_;

  promo.GetInteger("promo_id", &promo_id_);
  DVLOG(1) << "promo_id_ " << promo_id_;
}

// static
void NotificationPromo::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kPrefPromoObject);
}

// static
void NotificationPromo::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kPrefPromoObject);
}

// static
void NotificationPromo::MigrateUserPrefs(PrefService* user_prefs) {
  user_prefs->ClearPref(kPrefPromoObject);
}

void NotificationPromo::WritePrefs() {
  WritePrefs(promo_id_, first_view_time_, views_, closed_);
}

void NotificationPromo::WritePrefs(int promo_id,
                                   double first_view_time,
                                   int views,
                                   bool closed) {
  auto ntp_promo = std::make_unique<base::DictionaryValue>();
  ntp_promo->SetDouble(kPrefPromoFirstViewTime, first_view_time);
  ntp_promo->SetInteger(kPrefPromoViews, views);
  ntp_promo->SetBoolean(kPrefPromoClosed, closed);

  base::DictionaryValue promo_dict;
  promo_dict.MergeDictionary(local_state_->GetDictionary(kPrefPromoObject));
  promo_dict.Set(base::IntToString(promo_id), std::move(ntp_promo));
  local_state_->Set(kPrefPromoObject, promo_dict);
  DVLOG(1) << "WritePrefs " << promo_dict;
}

void NotificationPromo::InitFromPrefs() {
  // If |promo_id_| is not set, do nothing.
  if (promo_id_ == -1)
    return;

  const base::DictionaryValue* promo_dict =
      local_state_->GetDictionary(kPrefPromoObject);
  if (!promo_dict)
    return;

  const base::DictionaryValue* ntp_promo = NULL;
  promo_dict->GetDictionary(base::IntToString(promo_id_), &ntp_promo);
  if (!ntp_promo)
    return;

  ntp_promo->GetDouble(kPrefPromoFirstViewTime, &first_view_time_);
  ntp_promo->GetInteger(kPrefPromoViews, &views_);
  ntp_promo->GetBoolean(kPrefPromoClosed, &closed_);
}

bool NotificationPromo::CanShow() const {
  return !closed_ && !promo_text_.empty() && !ExceedsMaxViews() &&
         !ExceedsMaxSeconds() &&
         base::Time::FromDoubleT(StartTime()) < base::Time::Now() &&
         base::Time::FromDoubleT(EndTime()) > base::Time::Now();
}

void NotificationPromo::HandleClosed() {
  if (!closed_) {
    WritePrefs(promo_id_, first_view_time_, views_, true);
  }
}

void NotificationPromo::HandleViewed() {
  int views = views_ + 1;
  double first_view_time = first_view_time_;
  if (first_view_time == 0) {
    first_view_time = base::Time::Now().ToDoubleT();
  }
  WritePrefs(promo_id_, first_view_time, views, closed_);
}

bool NotificationPromo::ExceedsMaxViews() const {
  return (max_views_ == 0) ? false : views_ >= max_views_;
}

bool NotificationPromo::ExceedsMaxSeconds() const {
  if (max_seconds_ == 0 || first_view_time_ == 0)
    return false;

  const base::Time last_view_time = base::Time::FromDoubleT(first_view_time_) +
                                    base::TimeDelta::FromSeconds(max_seconds_);
  return last_view_time < base::Time::Now();
}

bool NotificationPromo::IsPayloadParam(const std::string& param_name) const {
  return param_name != "start" && param_name != "end" &&
         param_name != "promo_text" && param_name != "max_views" &&
         param_name != "max_seconds" && param_name != "promo_id";
}

double NotificationPromo::StartTime() const {
  return start_;
}

double NotificationPromo::EndTime() const {
  return end_;
}

}  // namespace ios
