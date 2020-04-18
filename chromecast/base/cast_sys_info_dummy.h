// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BASE_CAST_SYS_INFO_DUMMY_H_
#define CHROMECAST_BASE_CAST_SYS_INFO_DUMMY_H_

// Note(slan): This file is needed by internal targets which cannot depend on
// "//base". Amend this include with a comment so gn check ignores it.
#include "base/macros.h"  // nogncheck
#include "chromecast/public/cast_sys_info.h"

namespace chromecast {

// Dummy implementation of CastSysInfo. Fields can be overwritten for test.
class CastSysInfoDummy : public CastSysInfo {
 public:
  CastSysInfoDummy();
  ~CastSysInfoDummy() override;

  // CastSysInfo implementation:
  BuildType GetBuildType() override;
  std::string GetSystemReleaseChannel() override;
  std::string GetSerialNumber() override;
  std::string GetProductName() override;
  std::string GetDeviceModel() override;
  std::string GetBoardName() override;
  std::string GetBoardRevision() override;
  std::string GetManufacturer() override;
  std::string GetSystemBuildNumber() override;
  std::string GetFactoryCountry() override;
  std::string GetFactoryLocale(std::string* second_locale) override;
  std::string GetWifiInterface() override;
  std::string GetApInterface() override;
  std::string GetGlVendor() override;
  std::string GetGlRenderer() override;
  std::string GetGlVersion() override;

  void SetBuildTypeForTesting(BuildType build_type);
  void SetSystemReleaseChannelForTesting(
      const std::string& system_release_channel);
  void SetSerialNumberForTesting(const std::string& serial_number);
  void SetProductNameForTesting(const std::string& product_name);
  void SetDeviceModelForTesting(const std::string& device_model);
  void SetBoardNameForTesting(const std::string& board_name);
  void SetBoardRevisionForTesting(const std::string& board_revision);
  void SetManufacturerForTesting(const std::string& manufacturer);
  void SetSystemBuildNumberForTesting(const std::string& system_build_number);
  void SetFactoryCountryForTesting(const std::string& factory_country);
  void SetFactoryLocaleForTesting(const std::string& factory_locale);
  void SetWifiInterfaceForTesting(const std::string& wifi_interface);
  void SetApInterfaceForTesting(const std::string& ap_interface);
  void SetGlVendorForTesting(const std::string& gl_vendor);
  void SetGlRendererForTesting(const std::string& gl_renderer);
  void SetGlVersionForTesting(const std::string& gl_version);

 private:
  BuildType build_type_;
  std::string system_release_channel_;
  std::string serial_number_;
  std::string product_name_;
  std::string device_model_;
  std::string board_name_;
  std::string board_revision_;
  std::string manufacturer_;
  std::string system_build_number_;
  std::string factory_country_;
  std::string factory_locale_;
  std::string wifi_interface_;
  std::string ap_interface_;
  std::string gl_vendor_;
  std::string gl_renderer_;
  std::string gl_version_;

  DISALLOW_COPY_AND_ASSIGN(CastSysInfoDummy);
};

}  // namespace chromecast

#endif  // CHROMECAST_BASE_CAST_SYS_INFO_DUMMY_H_
