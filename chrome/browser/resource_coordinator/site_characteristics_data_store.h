// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_STORE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_STORE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/site_characteristics_data_reader.h"
#include "chrome/browser/resource_coordinator/site_characteristics_data_writer.h"

namespace resource_coordinator {

// Pure virtual interface for a site characteristics data store.
class SiteCharacteristicsDataStore {
 public:
  SiteCharacteristicsDataStore() = default;
  virtual ~SiteCharacteristicsDataStore() {}

  // Returns a SiteCharacteristicsDataReader for the given origin.
  virtual std::unique_ptr<SiteCharacteristicsDataReader> GetReaderForOrigin(
      const std::string& origin) = 0;

  // Returns a SiteCharacteristicsDataWriter for the given origin.
  virtual std::unique_ptr<SiteCharacteristicsDataWriter> GetWriterForOrigin(
      const std::string& origin_str) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SiteCharacteristicsDataStore);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_STORE_H_
