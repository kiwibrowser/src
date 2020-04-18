// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_WRITER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_WRITER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/resource_coordinator/site_characteristics_data_writer.h"

namespace resource_coordinator {

namespace internal {
class LocalSiteCharacteristicsDataImpl;
}  // namespace internal

// Specialization of a SiteCharacteristicsDataWriter that delegates to a
// LocalSiteCharacteristicsDataImpl.
class LocalSiteCharacteristicsDataWriter
    : public SiteCharacteristicsDataWriter {
 public:
  ~LocalSiteCharacteristicsDataWriter() override;

  // SiteCharacteristicsDataWriter:
  void NotifySiteLoaded() override;
  void NotifySiteUnloaded() override;
  void NotifyUpdatesFaviconInBackground() override;
  void NotifyUpdatesTitleInBackground() override;
  void NotifyUsesAudioInBackground() override;
  void NotifyUsesNotificationsInBackground() override;

 private:
  friend class LocalSiteCharacteristicsDataWriterTest;
  friend class LocalSiteCharacteristicsDataStoreTest;
  friend class LocalSiteCharacteristicsDataStore;

  // Private constructor, these objects are meant to be created by a site
  // characteristics data store.
  explicit LocalSiteCharacteristicsDataWriter(
      scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl);

  // The LocalSiteCharacteristicDataInternal object we delegate to.
  const scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(LocalSiteCharacteristicsDataWriter);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_WRITER_H_
