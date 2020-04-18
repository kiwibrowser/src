// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_noop_data_writer.h"

namespace resource_coordinator {

LocalSiteCharacteristicsNoopDataWriter::
    LocalSiteCharacteristicsNoopDataWriter() = default;
LocalSiteCharacteristicsNoopDataWriter::
    ~LocalSiteCharacteristicsNoopDataWriter() = default;

void LocalSiteCharacteristicsNoopDataWriter::NotifySiteLoaded() {}

void LocalSiteCharacteristicsNoopDataWriter::NotifySiteUnloaded() {}

void LocalSiteCharacteristicsNoopDataWriter::
    NotifyUpdatesFaviconInBackground() {}

void LocalSiteCharacteristicsNoopDataWriter::NotifyUpdatesTitleInBackground() {}

void LocalSiteCharacteristicsNoopDataWriter::NotifyUsesAudioInBackground() {}

void LocalSiteCharacteristicsNoopDataWriter::
    NotifyUsesNotificationsInBackground() {}

}  // namespace resource_coordinator
