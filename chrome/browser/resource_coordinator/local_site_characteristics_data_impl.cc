// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_database.h"
#include "chrome/browser/resource_coordinator/time.h"

namespace resource_coordinator {
namespace internal {

namespace {

base::TimeDelta GetTickDeltaSinceEpoch() {
  return resource_coordinator::NowTicks() - base::TimeTicks::UnixEpoch();
}

// Returns all the SiteCharacteristicsFeatureProto elements contained in a
// SiteCharacteristicsProto protobuf object.
std::vector<SiteCharacteristicsFeatureProto*> GetAllFeaturesFromProto(
    SiteCharacteristicsProto* proto) {
  std::vector<SiteCharacteristicsFeatureProto*> ret(
      {proto->mutable_updates_favicon_in_background(),
       proto->mutable_updates_title_in_background(),
       proto->mutable_uses_audio_in_background(),
       proto->mutable_uses_notifications_in_background()});

  return ret;
}

}  // namespace

void LocalSiteCharacteristicsDataImpl::NotifySiteLoaded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Update the last loaded time when this origin gets loaded for the first
  // time.
  if (active_webcontents_count_ == 0) {
    site_characteristics_.set_last_loaded(
        TimeDeltaToInternalRepresentation(GetTickDeltaSinceEpoch()));
  }
  active_webcontents_count_++;
}

void LocalSiteCharacteristicsDataImpl::NotifySiteUnloaded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  active_webcontents_count_--;
  // Only update the last loaded time when there's no more loaded instance of
  // this origin.
  if (active_webcontents_count_ > 0U)
    return;

  base::TimeDelta current_unix_time = GetTickDeltaSinceEpoch();
  base::TimeDelta extra_observation_duration =
      current_unix_time -
      InternalRepresentationToTimeDelta(site_characteristics_.last_loaded());

  // Update the |last_loaded_time_| field, as the moment this site gets unloaded
  // also corresponds to the last moment it was loaded.
  site_characteristics_.set_last_loaded(
      TimeDeltaToInternalRepresentation(current_unix_time));

  // Update the observation duration fields.
  for (auto* iter : GetAllFeaturesFromProto(&site_characteristics_))
    IncrementFeatureObservationDuration(iter, extra_observation_duration);
}

SiteFeatureUsage LocalSiteCharacteristicsDataImpl::UpdatesFaviconInBackground()
    const {
  return GetFeatureUsage(
      site_characteristics_.updates_favicon_in_background(),
      GetStaticProactiveTabDiscardParams().favicon_update_observation_window);
}

SiteFeatureUsage LocalSiteCharacteristicsDataImpl::UpdatesTitleInBackground()
    const {
  return GetFeatureUsage(
      site_characteristics_.updates_title_in_background(),
      GetStaticProactiveTabDiscardParams().title_update_observation_window);
}

SiteFeatureUsage LocalSiteCharacteristicsDataImpl::UsesAudioInBackground()
    const {
  return GetFeatureUsage(
      site_characteristics_.uses_audio_in_background(),
      GetStaticProactiveTabDiscardParams().audio_usage_observation_window);
}

SiteFeatureUsage
LocalSiteCharacteristicsDataImpl::UsesNotificationsInBackground() const {
  return GetFeatureUsage(
      site_characteristics_.uses_notifications_in_background(),
      GetStaticProactiveTabDiscardParams()
          .notifications_usage_observation_window);
}

void LocalSiteCharacteristicsDataImpl::NotifyUpdatesFaviconInBackground() {
  NotifyFeatureUsage(
      site_characteristics_.mutable_updates_favicon_in_background());
}

void LocalSiteCharacteristicsDataImpl::NotifyUpdatesTitleInBackground() {
  NotifyFeatureUsage(
      site_characteristics_.mutable_updates_title_in_background());
}

void LocalSiteCharacteristicsDataImpl::NotifyUsesAudioInBackground() {
  NotifyFeatureUsage(site_characteristics_.mutable_uses_audio_in_background());
}

void LocalSiteCharacteristicsDataImpl::NotifyUsesNotificationsInBackground() {
  NotifyFeatureUsage(
      site_characteristics_.mutable_uses_notifications_in_background());
}

LocalSiteCharacteristicsDataImpl::LocalSiteCharacteristicsDataImpl(
    const std::string& origin_str,
    OnDestroyDelegate* delegate,
    LocalSiteCharacteristicsDatabase* database)
    : origin_str_(origin_str),
      active_webcontents_count_(0U),
      database_(database),
      delegate_(delegate),
      safe_to_write_to_db_(false),
      weak_factory_(this) {
  DCHECK(database_);
  DCHECK(delegate_);
  DCHECK(!site_characteristics_.IsInitialized());

  database_->ReadSiteCharacteristicsFromDB(
      origin_str_,
      base::BindOnce(&LocalSiteCharacteristicsDataImpl::OnInitCallback,
                     weak_factory_.GetWeakPtr()));
}

LocalSiteCharacteristicsDataImpl::~LocalSiteCharacteristicsDataImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // It's currently required that the site gets unloaded before destroying this
  // object.
  // TODO(sebmarchand): Check if this is a valid assumption.
  DCHECK(!IsLoaded());

  DCHECK(delegate_);
  delegate_->OnLocalSiteCharacteristicsDataImplDestroyed(this);

  // TODO(sebmarchand): Some data might be lost here if the read operation has
  // not completed, add some metrics to measure if this is really an issue.
  if (safe_to_write_to_db_) {
    DCHECK(site_characteristics_.IsInitialized());
    database_->WriteSiteCharacteristicsIntoDB(origin_str_,
                                              site_characteristics_);
  }
}

base::TimeDelta LocalSiteCharacteristicsDataImpl::FeatureObservationDuration(
    const SiteCharacteristicsFeatureProto& feature_proto) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Get the current observation duration value, this'll be equal to 0 for
  // features that have been observed.
  base::TimeDelta observation_time_for_feature =
      InternalRepresentationToTimeDelta(feature_proto.observation_duration());

  // If this site is still loaded and the feature isn't in use then the
  // observation time since load needs to be added.
  if (IsLoaded() &&
      InternalRepresentationToTimeDelta(feature_proto.use_timestamp())
          .is_zero()) {
    base::TimeDelta observation_time_since_load =
        GetTickDeltaSinceEpoch() -
        InternalRepresentationToTimeDelta(site_characteristics_.last_loaded());
    observation_time_for_feature += observation_time_since_load;
  }

  return observation_time_for_feature;
}

// static:
void LocalSiteCharacteristicsDataImpl::IncrementFeatureObservationDuration(
    SiteCharacteristicsFeatureProto* feature_proto,
    base::TimeDelta extra_observation_duration) {
  if (!feature_proto->has_use_timestamp() ||
      InternalRepresentationToTimeDelta(feature_proto->use_timestamp())
          .is_zero()) {
    feature_proto->set_observation_duration(TimeDeltaToInternalRepresentation(
        InternalRepresentationToTimeDelta(
            feature_proto->observation_duration()) +
        extra_observation_duration));
  }
}

// static:
void LocalSiteCharacteristicsDataImpl::
    InitSiteCharacteristicsFeatureProtoWithDefaultValues(
        SiteCharacteristicsFeatureProto* proto) {
  DCHECK_NE(nullptr, proto);
  static const auto zero_interval =
      LocalSiteCharacteristicsDataImpl::TimeDeltaToInternalRepresentation(
          base::TimeDelta());
  proto->set_observation_duration(zero_interval);
  proto->set_use_timestamp(zero_interval);
}

void LocalSiteCharacteristicsDataImpl::InitWithDefaultValues(
    bool only_init_uninitialized_fields) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Initialize the feature elements with the default value, this is required
  // because some fields might otherwise never be initialized.
  for (auto* iter : GetAllFeaturesFromProto(&site_characteristics_)) {
    if (!only_init_uninitialized_fields || !iter->IsInitialized())
      InitSiteCharacteristicsFeatureProtoWithDefaultValues(iter);
  }

  if (!only_init_uninitialized_fields ||
      !site_characteristics_.has_last_loaded()) {
    site_characteristics_.set_last_loaded(
        LocalSiteCharacteristicsDataImpl::TimeDeltaToInternalRepresentation(
            base::TimeDelta()));
  }
}

void LocalSiteCharacteristicsDataImpl::
    ClearObservationsAndInvalidateReadOperation() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Invalidate the weak pointer that have been served, this will ensure that
  // this object doesn't get initialized from the database after being cleared.
  weak_factory_.InvalidateWeakPtrs();

  // Reset all the observations.
  InitWithDefaultValues(false);

  // Set the last loaded time to the current time if there's some loaded
  // instances of this site.
  if (IsLoaded()) {
    site_characteristics_.set_last_loaded(
        TimeDeltaToInternalRepresentation(GetTickDeltaSinceEpoch()));
  }

  // This object is now in a valid state and can be written in the database.
  safe_to_write_to_db_ = true;
}

SiteFeatureUsage LocalSiteCharacteristicsDataImpl::GetFeatureUsage(
    const SiteCharacteristicsFeatureProto& feature_proto,
    const base::TimeDelta min_obs_time) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!feature_proto.IsInitialized())
    return SiteFeatureUsage::kSiteFeatureUsageUnknown;

  // Checks if this feature has already been observed.
  // TODO(sebmarchand): Check the timestamp and reset features that haven't been
  // observed in a long time, https://crbug.com/826446.
  if (!InternalRepresentationToTimeDelta(feature_proto.use_timestamp())
           .is_zero()) {
    return SiteFeatureUsage::kSiteFeatureInUse;
  }

  if (FeatureObservationDuration(feature_proto) >= min_obs_time)
    return SiteFeatureUsage::kSiteFeatureNotInUse;

  return SiteFeatureUsage::kSiteFeatureUsageUnknown;
}

void LocalSiteCharacteristicsDataImpl::NotifyFeatureUsage(
    SiteCharacteristicsFeatureProto* feature_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsLoaded());

  feature_proto->set_use_timestamp(
      TimeDeltaToInternalRepresentation(GetTickDeltaSinceEpoch()));
  feature_proto->set_observation_duration(
      LocalSiteCharacteristicsDataImpl::TimeDeltaToInternalRepresentation(
          base::TimeDelta()));
}

void LocalSiteCharacteristicsDataImpl::OnInitCallback(
    base::Optional<SiteCharacteristicsProto> db_site_characteristics) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Check if the initialization has succeeded.
  if (db_site_characteristics) {
    // If so, iterates over all the features and initialize them.
    auto this_features = GetAllFeaturesFromProto(&site_characteristics_);
    auto db_features =
        GetAllFeaturesFromProto(&db_site_characteristics.value());
    auto this_features_iter = this_features.begin();
    auto db_features_iter = db_features.begin();
    for (; this_features_iter != this_features.end() &&
           db_features_iter != db_features.end();
         ++this_features_iter, ++db_features_iter) {
      // If the |use_timestamp| field is set for the in-memory entry for this
      // feature then there's nothing to do, otherwise update it with the values
      // from the database.
      if (!(*this_features_iter)->has_use_timestamp()) {
        if ((*db_features_iter)->has_use_timestamp()) {
          // Keep the use timestamp from the database, if any.
          (*this_features_iter)
              ->set_use_timestamp((*db_features_iter)->use_timestamp());
          (*this_features_iter)
              ->set_observation_duration(
                  LocalSiteCharacteristicsDataImpl::
                      TimeDeltaToInternalRepresentation(base::TimeDelta()));
        } else {
          // Else, add the observation duration from the database to the
          // in-memory observation duration.
          if (!(*this_features_iter)->has_observation_duration()) {
            (*this_features_iter)
                ->set_observation_duration(
                    LocalSiteCharacteristicsDataImpl::
                        TimeDeltaToInternalRepresentation(base::TimeDelta()));
          }
          IncrementFeatureObservationDuration(
              (*this_features_iter),
              InternalRepresentationToTimeDelta(
                  (*db_features_iter)->observation_duration()));
        }
      }
    }
    // Only update the last loaded field if we haven't updated it since the
    // creation of this object.
    if (!site_characteristics_.has_last_loaded()) {
      site_characteristics_.set_last_loaded(
          db_site_characteristics->last_loaded());
    }
  } else {
    // Init all the fields that haven't been initialized with a default value.
    InitWithDefaultValues(true /* only_init_uninitialized_fields */);
  }

  safe_to_write_to_db_ = true;
  DCHECK(site_characteristics_.IsInitialized());
}

}  // namespace internal
}  // namespace resource_coordinator
