// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_TEST_BASE_H_
#define CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_TEST_BASE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/profile_resetter/profile_resetter.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {
class BrowserContext;
}

// The ProfileResetterMockObject is used to block the thread until
// ProfileResetter::Reset has completed:

// ProfileResetterMockObject mock_object;
// resetter_->Reset(ProfileResetter::ALL,
//                  pointer,
//                  base::Bind(&ProfileResetterMockObject::StopLoop,
//                             base::Unretained(&mock_object)));
// mock_object.RunLoop();
class ProfileResetterMockObject {
 public:
  ProfileResetterMockObject();
  ~ProfileResetterMockObject();

  void RunLoop();
  void StopLoop();

 private:
  MOCK_METHOD0(Callback, void(void));

  scoped_refptr<content::MessageLoopRunner> runner_;

  DISALLOW_COPY_AND_ASSIGN(ProfileResetterMockObject);
};

// Base class for all ProfileResetter unit tests.
class ProfileResetterTestBase {
 public:
  ProfileResetterTestBase();
  ~ProfileResetterTestBase();

  void ResetAndWait(ProfileResetter::ResettableFlags resettable_flags);
  void ResetAndWait(ProfileResetter::ResettableFlags resettable_flags,
                    const std::string& prefs);
 protected:
  testing::StrictMock<ProfileResetterMockObject> mock_object_;
  std::unique_ptr<ProfileResetter> resetter_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileResetterTestBase);
};

std::unique_ptr<KeyedService> CreateTemplateURLServiceForTesting(
    content::BrowserContext* context);

#endif  // CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_TEST_BASE_H_
