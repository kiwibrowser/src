// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/get_session_name.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#endif  // OS_CHROMEOS

namespace syncer {

namespace {

class GetSessionNameTest : public ::testing::Test {
 public:
  void SetSessionName(base::OnceClosure on_done,
                      const std::string& session_name) {
    session_name_ = session_name;
    std::move(on_done).Run();
  }

 protected:
  base::MessageLoop message_loop_;
  std::string session_name_;
};

// Call GetSessionNameSynchronouslyForTesting and make sure its return
// value looks sane.
TEST_F(GetSessionNameTest, GetSessionNameSynchronously) {
  const std::string& session_name = GetSessionNameSynchronouslyForTesting();
  EXPECT_FALSE(session_name.empty());
}

#if defined(OS_CHROMEOS)

// Call GetSessionNameSynchronouslyForTesting on ChromeOS where the board type
// is CHROMEBOOK and make sure the return value is "Chromebook".
TEST_F(GetSessionNameTest, GetSessionNameSynchronouslyChromebook) {
  const char* kLsbRelease = "DEVICETYPE=CHROMEBOOK\n";
  base::SysInfo::SetChromeOSVersionInfoForTest(kLsbRelease, base::Time());
  const std::string& session_name = GetSessionNameSynchronouslyForTesting();
  EXPECT_EQ("Chromebook", session_name);
}

// Call GetSessionNameSynchronouslyForTesting on ChromeOS where the board type
// is a CHROMEBOX and make sure the return value is "Chromebox".
TEST_F(GetSessionNameTest, GetSessionNameSynchronouslyChromebox) {
  const char* kLsbRelease = "DEVICETYPE=CHROMEBOX\n";
  base::SysInfo::SetChromeOSVersionInfoForTest(kLsbRelease, base::Time());
  const std::string& session_name = GetSessionNameSynchronouslyForTesting();
  EXPECT_EQ("Chromebox", session_name);
}

#endif  // OS_CHROMEOS

// Calls GetSessionName and runs the message loop until it comes back
// with a session name.  Makes sure the returned session name is equal
// to the return value of GetSessionNameSynchronouslyForTesting().
TEST_F(GetSessionNameTest, GetSessionName) {
  base::RunLoop run_loop;
  GetSessionName(
      message_loop_.task_runner(),
      base::Bind(&GetSessionNameTest::SetSessionName, base::Unretained(this),
                 run_loop.QuitWhenIdleClosure()));
  run_loop.Run();
  EXPECT_EQ(session_name_, GetSessionNameSynchronouslyForTesting());
}

}  // namespace

}  // namespace syncer
