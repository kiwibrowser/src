// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_FACTORY_H_
#define UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_FACTORY_H_

#include <memory>

#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// Builds OSExchangeDataProviders. We need to be able to switch providers at
// runtime based on the configuration flags. If no factory is set,
// CreateProvider() will default to the current operating system's default.
class UI_BASE_EXPORT OSExchangeDataProviderFactory {
 public:
  class Factory {
   public:
    virtual std::unique_ptr<OSExchangeData::Provider> BuildProvider() = 0;
  };

  // Sets the factory which builds the providers.
  static void SetFactory(Factory* factory);

  // Returns the current factory and sets the factory to null.
  static Factory* TakeFactory();

  // Creates a Provider based on the current configuration.
  static std::unique_ptr<OSExchangeData::Provider> CreateProvider();
};

}  // namespace ui

#endif  // UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_FACTORY_H_
