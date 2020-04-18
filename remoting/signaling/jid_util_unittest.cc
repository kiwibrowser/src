// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/jid_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(JidUtil, NormalizeJid) {
  EXPECT_EQ(NormalizeJid("USER@DOMAIN.com"), "user@domain.com");
  EXPECT_EQ(NormalizeJid("user@domain.com"), "user@domain.com");
  EXPECT_EQ(NormalizeJid("USER@DOMAIN.com/RESOURCE"),
            "user@domain.com/RESOURCE");
  EXPECT_EQ(NormalizeJid("USER@DOMAIN.com/"), "user@domain.com/");
}

TEST(JidUtil, SplitJidResource) {
  std::string bare_jid;
  std::string resource_suffix;

  EXPECT_TRUE(SplitJidResource("user@domain/resource", nullptr, nullptr));
  EXPECT_TRUE(
      SplitJidResource("user@domain/resource", &bare_jid, &resource_suffix));
  EXPECT_EQ(bare_jid, "user@domain");
  EXPECT_EQ(resource_suffix, "resource");

  EXPECT_FALSE(SplitJidResource("user@domain", nullptr, nullptr));
  EXPECT_FALSE(SplitJidResource("user@domain", &bare_jid, &resource_suffix));
  EXPECT_EQ(bare_jid, "user@domain");
  EXPECT_EQ(resource_suffix, "");
}

}  // namespace remoting
