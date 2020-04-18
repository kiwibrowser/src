// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_SCOPED_TESTING_LOCAL_STATE_H_
#define CHROME_TEST_BASE_SCOPED_TESTING_LOCAL_STATE_H_

#include "base/macros.h"
#include "components/prefs/testing_pref_service.h"

class TestingBrowserProcess;

// Helper class to temporarily set up a |local_state| in the global
// TestingBrowserProcess (for most unit tests it's NULL).
class ScopedTestingLocalState {
 public:
  explicit ScopedTestingLocalState(TestingBrowserProcess* browser_process);
  ~ScopedTestingLocalState();

  TestingPrefServiceSimple* Get() {
    return &local_state_;
  }

 private:
  TestingBrowserProcess* browser_process_;
  TestingPrefServiceSimple local_state_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTestingLocalState);
};

#endif  // CHROME_TEST_BASE_SCOPED_TESTING_LOCAL_STATE_H_
