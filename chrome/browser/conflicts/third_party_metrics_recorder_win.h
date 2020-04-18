// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_THIRD_PARTY_METRICS_RECORDER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_THIRD_PARTY_METRICS_RECORDER_WIN_H_

#include "base/macros.h"
#include "chrome/browser/conflicts/module_database_observer_win.h"

struct ModuleInfoData;
struct ModuleInfoKey;

// Records metrics about third party modules loaded into Chrome.
class ThirdPartyMetricsRecorder : public ModuleDatabaseObserver {
 public:
  ThirdPartyMetricsRecorder();
  ~ThirdPartyMetricsRecorder() override;

  // ModuleDatabaseObserver:
  void OnNewModuleFound(const ModuleInfoKey& module_key,
                        const ModuleInfoData& module_data) override;
  void OnModuleDatabaseIdle() override;

 private:
  // Flag used to avoid sending module counts multiple times.
  bool metrics_emitted_ = false;

  // Counters for different types of modules.
  size_t module_count_ = 0;
  size_t signed_module_count_ = 0;
  size_t catalog_module_count_ = 0;
  size_t microsoft_module_count_ = 0;
  size_t loaded_third_party_module_count_ = 0;
  size_t not_loaded_third_party_module_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ThirdPartyMetricsRecorder);
};

#endif  // CHROME_BROWSER_CONFLICTS_THIRD_PARTY_METRICS_RECORDER_WIN_H_
