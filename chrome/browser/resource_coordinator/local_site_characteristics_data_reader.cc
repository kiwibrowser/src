// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_reader.h"

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"

namespace resource_coordinator {

LocalSiteCharacteristicsDataReader::LocalSiteCharacteristicsDataReader(
    scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl)
    : impl_(std::move(impl)) {}

LocalSiteCharacteristicsDataReader::~LocalSiteCharacteristicsDataReader() {}

SiteFeatureUsage
LocalSiteCharacteristicsDataReader::UpdatesFaviconInBackground() const {
  return impl_->UpdatesFaviconInBackground();
}

SiteFeatureUsage LocalSiteCharacteristicsDataReader::UpdatesTitleInBackground()
    const {
  return impl_->UpdatesTitleInBackground();
}

SiteFeatureUsage LocalSiteCharacteristicsDataReader::UsesAudioInBackground()
    const {
  return impl_->UsesAudioInBackground();
}

SiteFeatureUsage
LocalSiteCharacteristicsDataReader::UsesNotificationsInBackground() const {
  return impl_->UsesNotificationsInBackground();
}

}  // namespace resource_coordinator
