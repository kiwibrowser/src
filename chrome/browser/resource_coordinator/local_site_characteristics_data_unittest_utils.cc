// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_unittest_utils.h"

namespace resource_coordinator {
namespace testing {

MockLocalSiteCharacteristicsDataImplOnDestroyDelegate::
    MockLocalSiteCharacteristicsDataImplOnDestroyDelegate() = default;
MockLocalSiteCharacteristicsDataImplOnDestroyDelegate::
    ~MockLocalSiteCharacteristicsDataImplOnDestroyDelegate() = default;

NoopLocalSiteCharacteristicsDatabase::NoopLocalSiteCharacteristicsDatabase() =
    default;
NoopLocalSiteCharacteristicsDatabase::~NoopLocalSiteCharacteristicsDatabase() =
    default;

void NoopLocalSiteCharacteristicsDatabase::ReadSiteCharacteristicsFromDB(
    const std::string& site_origin,
    ReadSiteCharacteristicsFromDBCallback callback) {
  std::move(callback).Run(base::nullopt);
}

void NoopLocalSiteCharacteristicsDatabase::WriteSiteCharacteristicsIntoDB(
    const std::string& site_origin,
    const SiteCharacteristicsProto& site_characteristic_proto) {}

void NoopLocalSiteCharacteristicsDatabase::RemoveSiteCharacteristicsFromDB(
    const std::vector<std::string>& site_origins) {}

void NoopLocalSiteCharacteristicsDatabase::ClearDatabase() {}

}  // namespace testing
}  // namespace resource_coordinator
