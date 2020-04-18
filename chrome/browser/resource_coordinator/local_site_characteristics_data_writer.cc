// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_writer.h"

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"

namespace resource_coordinator {

LocalSiteCharacteristicsDataWriter::~LocalSiteCharacteristicsDataWriter() =
    default;

void LocalSiteCharacteristicsDataWriter::NotifySiteLoaded() {
  impl_->NotifySiteLoaded();
}

void LocalSiteCharacteristicsDataWriter::NotifySiteUnloaded() {
  impl_->NotifySiteUnloaded();
}

void LocalSiteCharacteristicsDataWriter::NotifyUpdatesFaviconInBackground() {
  impl_->NotifyUpdatesFaviconInBackground();
}

void LocalSiteCharacteristicsDataWriter::NotifyUpdatesTitleInBackground() {
  impl_->NotifyUpdatesTitleInBackground();
}

void LocalSiteCharacteristicsDataWriter::NotifyUsesAudioInBackground() {
  impl_->NotifyUsesAudioInBackground();
}

void LocalSiteCharacteristicsDataWriter::NotifyUsesNotificationsInBackground() {
  impl_->NotifyUsesNotificationsInBackground();
}

LocalSiteCharacteristicsDataWriter::LocalSiteCharacteristicsDataWriter(
    scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl)
    : impl_(std::move(impl)) {}

}  // namespace resource_coordinator
