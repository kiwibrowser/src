// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_IOS_CHROME_SCOPED_TESTING_LOCAL_STATE_H_
#define IOS_CHROME_TEST_IOS_CHROME_SCOPED_TESTING_LOCAL_STATE_H_

#include "base/macros.h"
#include "components/prefs/testing_pref_service.h"

// Helper class to temporarily set up a |local_state| in the global
// TestingApplicationContext.
class IOSChromeScopedTestingLocalState {
 public:
  IOSChromeScopedTestingLocalState();
  ~IOSChromeScopedTestingLocalState();

  TestingPrefServiceSimple* Get();

 private:
  TestingPrefServiceSimple local_state_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeScopedTestingLocalState);
};

#endif  // IOS_CHROME_TEST_IOS_CHROME_SCOPED_TESTING_LOCAL_STATE_H_
