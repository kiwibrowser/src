// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/dragdrop/os_exchange_data_provider_builder_mac.h"

#include "ui/base/dragdrop/os_exchange_data_provider_mac.h"

namespace ui {

std::unique_ptr<OSExchangeData::Provider> BuildOSExchangeDataProviderMac() {
  return std::make_unique<OSExchangeDataProviderMac>();
}

}  // namespace ui
