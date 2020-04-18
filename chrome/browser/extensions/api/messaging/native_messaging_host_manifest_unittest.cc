// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_messaging_host_manifest.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/string_escape.h"
#include "build/build_config.h"
#include "extensions/common/url_pattern_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

const char kTestHostName[] = "com.chrome.test.native_host";
#if defined(OS_WIN)
const char kTestHostPath[] = "C:\\ProgramFiles\\host.exe";
#else
const char kTestHostPath[] = "/usr/bin/host";
#endif
const char kTestOrigin[] =
    "chrome-extension://knldjmfmopnpolahpmmgbagdohdnhkik/";

class NativeMessagingHostManifestTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    manifest_path_ = temp_dir_.GetPath().AppendASCII("test.json");
  }

 protected:
  bool WriteManifest(const std::string& name,
                     const std::string& path,
                     const std::string& origin) {
    return WriteManifest("{"
      "  \"name\": \"" + name + "\","
      "  \"description\": \"Native Messaging Test\","
      "  \"path\": " + base::GetQuotedJSONString(path) + ","
      "  \"type\": \"stdio\","
      "  \"allowed_origins\": ["
      "    \"" + origin + "\""
      "  ]"
      "}");
  }

  bool WriteManifest(const std::string& manifest_content) {
    return base::WriteFile(manifest_path_, manifest_content.data(),
                           manifest_content.size()) ==
           static_cast<int>(manifest_content.size());
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath manifest_path_;
};

TEST_F(NativeMessagingHostManifestTest, HostNameValidation) {
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("a"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("foo"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("foo132"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("foo.bar"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("foo.bar2"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("a._.c"));
  EXPECT_TRUE(NativeMessagingHostManifest::IsValidName("a._.c"));
  EXPECT_FALSE(NativeMessagingHostManifest::IsValidName("A.b"));
  EXPECT_FALSE(NativeMessagingHostManifest::IsValidName("a..b"));
  EXPECT_FALSE(NativeMessagingHostManifest::IsValidName(".a"));
  EXPECT_FALSE(NativeMessagingHostManifest::IsValidName("b."));
  EXPECT_FALSE(NativeMessagingHostManifest::IsValidName("a*"));
}

TEST_F(NativeMessagingHostManifestTest, LoadValid) {
  ASSERT_TRUE(WriteManifest(kTestHostName, kTestHostPath, kTestOrigin));

  std::string error_message;
  std::unique_ptr<NativeMessagingHostManifest> manifest =
      NativeMessagingHostManifest::Load(manifest_path_, &error_message);
  ASSERT_TRUE(manifest) << "Failed to load manifest: " << error_message;
  EXPECT_TRUE(error_message.empty());

  EXPECT_EQ(manifest->name(), "com.chrome.test.native_host");
  EXPECT_EQ(manifest->description(), "Native Messaging Test");
  EXPECT_EQ(manifest->interface(),
            NativeMessagingHostManifest::HOST_INTERFACE_STDIO);
  EXPECT_EQ(manifest->path(), base::FilePath::FromUTF8Unsafe(kTestHostPath));
  EXPECT_TRUE(manifest->allowed_origins().MatchesSecurityOrigin(
      GURL("chrome-extension://knldjmfmopnpolahpmmgbagdohdnhkik/")));
  EXPECT_FALSE(manifest->allowed_origins().MatchesSecurityOrigin(
      GURL("chrome-extension://jnldjmfmopnpolahpmmgbagdohdnhkik/")));
}

TEST_F(NativeMessagingHostManifestTest, InvalidName) {
  ASSERT_TRUE(WriteManifest(".com.chrome.test.native_host",
                            kTestHostPath, kTestOrigin));

  std::string error_message;
  std::unique_ptr<NativeMessagingHostManifest> manifest =
      NativeMessagingHostManifest::Load(manifest_path_, &error_message);
  ASSERT_FALSE(manifest);
  EXPECT_FALSE(error_message.empty());
}

// Verify that match-all origins are rejected.
TEST_F(NativeMessagingHostManifestTest, MatchAllOrigin) {
  ASSERT_TRUE(WriteManifest(kTestHostName, kTestHostPath,
                            "chrome-extension://*/"));

  std::string error_message;
  std::unique_ptr<NativeMessagingHostManifest> manifest =
      NativeMessagingHostManifest::Load(manifest_path_, &error_message);
  ASSERT_FALSE(manifest);
  EXPECT_FALSE(error_message.empty());
}

}  // namespace extensions
