// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_NON_RECORDING_DATA_STORE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_NON_RECORDING_DATA_STORE_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/site_characteristics_data_store.h"

namespace resource_coordinator {

// Specialization of a SiteCharacteristicsDataStore whose
// SiteCharacteristicsDataWriters don't persist observations and whose
// SiteCharacteristicsDataReader are obtained from another
// SiteCharacteristicsDataStore.
class LocalSiteCharacteristicsNonRecordingDataStore
    : public SiteCharacteristicsDataStore {
 public:
  // |data_store_for_readers| should outlive this object.
  explicit LocalSiteCharacteristicsNonRecordingDataStore(
      SiteCharacteristicsDataStore* data_store_for_readers);
  ~LocalSiteCharacteristicsNonRecordingDataStore() override;

  // SiteCharacteristicDataStore:
  std::unique_ptr<SiteCharacteristicsDataReader> GetReaderForOrigin(
      const std::string& origin_str) override;
  std::unique_ptr<SiteCharacteristicsDataWriter> GetWriterForOrigin(
      const std::string& origin_str) override;

 private:
  // The data store to use to create the readers served by this data store. E.g.
  // during an incognito session it should point to the data store used by the
  // parent session.
  SiteCharacteristicsDataStore* data_store_for_readers_;

  DISALLOW_COPY_AND_ASSIGN(LocalSiteCharacteristicsNonRecordingDataStore);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_NON_RECORDING_DATA_STORE_H_
