// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_PRINT_SETTINGS_MANAGER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_PRINT_SETTINGS_MANAGER_H_

#include <stdint.h>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "ppapi/c/dev/pp_print_settings_dev.h"

namespace content {

// A class for getting the default print settings for the default printer.
class CONTENT_EXPORT PepperPrintSettingsManager {
 public:
  typedef std::pair<PP_PrintSettings_Dev, int32_t> Result;
  typedef base::Callback<void(Result)> Callback;

  // The default print settings are obtained asynchronously and |callback|
  // is called with the the print settings when they are available. |callback|
  // will always be called on the same thread from which
  // |GetDefaultPrintSettings| was issued.
  virtual void GetDefaultPrintSettings(Callback callback) = 0;

  virtual ~PepperPrintSettingsManager() {}
};

// Real implementation for getting the default print settings.
class CONTENT_EXPORT PepperPrintSettingsManagerImpl
    : public PepperPrintSettingsManager {
 public:
  PepperPrintSettingsManagerImpl() {}
  ~PepperPrintSettingsManagerImpl() override {}

  // PepperPrintSettingsManager implementation.
  void GetDefaultPrintSettings(
      PepperPrintSettingsManager::Callback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperPrintSettingsManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_PRINT_SETTINGS_MANAGER_H_
