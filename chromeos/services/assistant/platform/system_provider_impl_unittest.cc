// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/platform/system_provider_impl.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace assistant {

class SystemProviderImplTest : public testing::Test {
 public:
  SystemProviderImplTest() {}

  SystemProviderImpl* system_provider_() { return &system_provider_impl_; }

 private:
  SystemProviderImpl system_provider_impl_;

  DISALLOW_COPY_AND_ASSIGN(SystemProviderImplTest);
};

TEST_F(SystemProviderImplTest, DebugServerOnForDebugBuild) {
#if DCHECK_IS_ON()
  ASSERT_GT(system_provider_()->GetDebugServerPort(), 0);
#endif  // DCHECK_IS_ON()
}

TEST_F(SystemProviderImplTest, DebugServerOffForReleaseBuild) {
#if !DCHECK_IS_ON()
  ASSERT_LT(system_provider_()->GetDebugServerPort(), 0);
#endif  // !DCHECK_IS_ON()
}

}  // namespace assistant
}  // namespace chromeos
