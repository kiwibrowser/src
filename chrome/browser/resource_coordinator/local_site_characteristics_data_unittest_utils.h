// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_UNITTEST_UTILS_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_UNITTEST_UTILS_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_database.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace resource_coordinator {
namespace testing {

class MockLocalSiteCharacteristicsDataImplOnDestroyDelegate
    : public internal::LocalSiteCharacteristicsDataImpl::OnDestroyDelegate {
 public:
  MockLocalSiteCharacteristicsDataImplOnDestroyDelegate();
  ~MockLocalSiteCharacteristicsDataImplOnDestroyDelegate();

  MOCK_METHOD1(OnLocalSiteCharacteristicsDataImplDestroyed,
               void(internal::LocalSiteCharacteristicsDataImpl*));

 private:
  DISALLOW_COPY_AND_ASSIGN(
      MockLocalSiteCharacteristicsDataImplOnDestroyDelegate);
};

// An implementation of a LocalSiteCharacteristicsDatabase that doesn't record
// anything.
class NoopLocalSiteCharacteristicsDatabase
    : public LocalSiteCharacteristicsDatabase {
 public:
  NoopLocalSiteCharacteristicsDatabase();
  ~NoopLocalSiteCharacteristicsDatabase() override;

  // LocalSiteCharacteristicsDatabase:
  void ReadSiteCharacteristicsFromDB(
      const std::string& site_origin,
      ReadSiteCharacteristicsFromDBCallback callback) override;
  void WriteSiteCharacteristicsIntoDB(
      const std::string& site_origin,
      const SiteCharacteristicsProto& site_characteristic_proto) override;
  void RemoveSiteCharacteristicsFromDB(
      const std::vector<std::string>& site_origins) override;
  void ClearDatabase() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NoopLocalSiteCharacteristicsDatabase);
};

}  // namespace testing
}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_DATA_UNITTEST_UTILS_H_
