// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_CONFIGURATION_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_CONFIGURATION_IMPL_H_

#include "base/macros.h"
#include "components/offline_pages/core/prefetch/prefetch_configuration.h"

class PrefService;

namespace offline_pages {

// Implementation of PrefetchConfiguration that queries Chrome preferences.
class PrefetchConfigurationImpl : public PrefetchConfiguration {
 public:
  explicit PrefetchConfigurationImpl(PrefService* pref_service);
  ~PrefetchConfigurationImpl() override;

  // PrefetchConfiguration implementation.
  bool IsPrefetchingEnabledBySettings() override;

 private:
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchConfigurationImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_CONFIGURATION_IMPL_H_
