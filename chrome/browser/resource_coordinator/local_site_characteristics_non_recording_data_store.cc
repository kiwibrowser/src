// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_non_recording_data_store.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_noop_data_writer.h"

namespace resource_coordinator {

LocalSiteCharacteristicsNonRecordingDataStore::
    LocalSiteCharacteristicsNonRecordingDataStore(
        SiteCharacteristicsDataStore* data_store_for_readers)
    : data_store_for_readers_(data_store_for_readers) {
  DCHECK(data_store_for_readers_);
}

LocalSiteCharacteristicsNonRecordingDataStore::
    ~LocalSiteCharacteristicsNonRecordingDataStore() = default;

std::unique_ptr<SiteCharacteristicsDataReader>
LocalSiteCharacteristicsNonRecordingDataStore::GetReaderForOrigin(
    const std::string& origin_str) {
  return data_store_for_readers_->GetReaderForOrigin(origin_str);
}

std::unique_ptr<SiteCharacteristicsDataWriter>
LocalSiteCharacteristicsNonRecordingDataStore::GetWriterForOrigin(
    const std::string& origin_str) {
  // Return a fake data writer.
  SiteCharacteristicsDataWriter* writer =
      new LocalSiteCharacteristicsNoopDataWriter();
  return base::WrapUnique(writer);
}

}  // namespace resource_coordinator
