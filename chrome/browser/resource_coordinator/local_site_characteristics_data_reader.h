// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_READER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_READER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/resource_coordinator/site_characteristics_data_reader.h"

namespace resource_coordinator {

namespace internal {
class LocalSiteCharacteristicsDataImpl;
}  // namespace internal

// Specialization of a SiteCharacteristicDataReader that delegates to a
// LocalSiteCharacteristicsDataImpl.
class LocalSiteCharacteristicsDataReader
    : public SiteCharacteristicsDataReader {
 public:
  ~LocalSiteCharacteristicsDataReader() override;

  // SiteCharacteristicsDataReader:
  SiteFeatureUsage UpdatesFaviconInBackground() const override;
  SiteFeatureUsage UpdatesTitleInBackground() const override;
  SiteFeatureUsage UsesAudioInBackground() const override;
  SiteFeatureUsage UsesNotificationsInBackground() const override;

 private:
  friend class LocalSiteCharacteristicsDataReaderTest;
  friend class LocalSiteCharacteristicsDataStoreTest;
  friend class LocalSiteCharacteristicsDataStore;

  // Private constructor, these objects are meant to be created by a site
  // characteristics data store.
  explicit LocalSiteCharacteristicsDataReader(
      scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl);

  // The LocalSiteCharacteristicDataInternal object we delegate to.
  const scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(LocalSiteCharacteristicsDataReader);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_READER_H_
