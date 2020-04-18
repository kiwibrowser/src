// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_CAST_DISPLAY_INFO_PROVIDER_H_
#define CHROMECAST_BROWSER_EXTENSIONS_CAST_DISPLAY_INFO_PROVIDER_H_

#include "base/macros.h"
#include "extensions/browser/api/system_display/display_info_provider.h"

namespace extensions {

class CastDisplayInfoProvider : public DisplayInfoProvider {
 public:
  CastDisplayInfoProvider();

  // DisplayInfoProvider implementation.
  void UpdateDisplayUnitInfoForPlatform(
      const display::Display& display,
      extensions::api::system_display::DisplayUnitInfo* unit) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastDisplayInfoProvider);
};

}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_CAST_DISPLAY_INFO_PROVIDER_H_
