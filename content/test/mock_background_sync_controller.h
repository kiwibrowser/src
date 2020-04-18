// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_BACKGROUND_SYNC_CONTROLLER_H_
#define CONTENT_TEST_MOCK_BACKGROUND_SYNC_CONTROLLER_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/background_sync_controller.h"
#include "content/public/browser/background_sync_parameters.h"
#include "url/gurl.h"

namespace content {

// Mocks a BackgroundSyncController, tracking state for use in tests.
class MockBackgroundSyncController : public BackgroundSyncController {
 public:
  MockBackgroundSyncController() = default;
  ~MockBackgroundSyncController() override = default;

  // BackgroundSyncController:
  void NotifyBackgroundSyncRegistered(const GURL& origin) override;
  void RunInBackground(bool enabled, int64_t min_ms) override;
  void GetParameterOverrides(
      BackgroundSyncParameters* parameters) const override;

  int registration_count() const { return registration_count_; }
  GURL registration_origin() const { return registration_origin_; }
  int run_in_background_count() const { return run_in_background_count_; }
  bool run_in_background_enabled() const { return run_in_background_enabled_; }
  int64_t run_in_background_min_ms() const { return run_in_background_min_ms_; }
  BackgroundSyncParameters* background_sync_parameters() {
    return &background_sync_parameters_;
  }

 private:
  int registration_count_ = 0;
  GURL registration_origin_;

  int run_in_background_count_ = 0;
  bool run_in_background_enabled_ = true;
  int64_t run_in_background_min_ms_ = 0;
  BackgroundSyncParameters background_sync_parameters_;

  DISALLOW_COPY_AND_ASSIGN(MockBackgroundSyncController);
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_BACKGROUND_SYNC_CONTROLLER_H_
