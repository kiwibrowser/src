// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_url.h"

#include <string>

#include "chrome/browser/chromeos/smb_client/smb_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace chromeos {
namespace smb_client {

class SmbUrlTest : public testing::Test {
 public:
  SmbUrlTest() {
    // Add the scheme to the "standard" list for url_util. This enables GURL to
    // properly process the domain from the url.
    url::AddStandardScheme(kSmbScheme, url::SCHEME_WITH_HOST);
  }

  ~SmbUrlTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(SmbUrlTest);
};

TEST_F(SmbUrlTest, NotValidWhenInitialized) {
  SmbUrl smb_url;

  EXPECT_FALSE(smb_url.IsValid());
}

TEST_F(SmbUrlTest, EmptyUrlIsInvalid) {
  const std::string empty_url = "";
  SmbUrl smb_url;
  EXPECT_FALSE(smb_url.InitializeWithUrl(empty_url));

  EXPECT_FALSE(smb_url.IsValid());
}

TEST_F(SmbUrlTest, InvalidUrls) {
  {
    SmbUrl smb_url;
    EXPECT_FALSE(smb_url.InitializeWithUrl("smb"));
  }
  {
    SmbUrl smb_url;
    EXPECT_FALSE(smb_url.InitializeWithUrl("smb://"));
  }
  {
    SmbUrl smb_url;
    EXPECT_FALSE(smb_url.InitializeWithUrl("\\"));
  }
  {
    SmbUrl smb_url;
    EXPECT_FALSE(smb_url.InitializeWithUrl("\\\\"));
  }
  {
    SmbUrl smb_url;
    EXPECT_FALSE(smb_url.InitializeWithUrl("smb:///"));
  }
}

TEST_F(SmbUrlTest, ValidUrls) {
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("smb://x"));

    const std::string expected_url = "smb://x/";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "x";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("smb:///x"));

    const std::string expected_url = "smb://x/";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "x";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("smb://server/share/long/folder"));

    const std::string expected_url = "smb://server/share/long/folder";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "server";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(
        smb_url.InitializeWithUrl("smb://server/share/folder.with.dots"));

    const std::string expected_url = "smb://server/share/folder.with.dots";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "server";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(
        smb_url.InitializeWithUrl("smb://server\\share/mixed\\slashes"));

    const std::string expected_url = "smb://server/share/mixed/slashes";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "server";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("\\\\server/share"));

    const std::string expected_url = "smb://server/share";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "server";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("\\\\server\\share/mixed//slashes"));

    const std::string expected_url = "smb://server/share/mixed//slashes";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "server";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
  {
    SmbUrl smb_url;
    EXPECT_TRUE(smb_url.InitializeWithUrl("smb://192.168.0.1/share"));

    const std::string expected_url = "smb://192.168.0.1/share";
    EXPECT_EQ(expected_url, smb_url.ToString());

    const std::string expected_host = "192.168.0.1";
    EXPECT_EQ(expected_host, smb_url.GetHost());
  }
}

TEST_F(SmbUrlTest, NotValidIfStartsWithoutSchemeOrDoubleBackslash) {
  const std::string url = "192.168.0.1/share";
  SmbUrl smb_url;
  EXPECT_FALSE(smb_url.InitializeWithUrl(url));

  EXPECT_FALSE(smb_url.IsValid());
}

TEST_F(SmbUrlTest, StartsWithBackslashRemovesBackslashAndAddsScheme) {
  const std::string url = "\\\\192.168.0.1\\share";
  SmbUrl smb_url;
  EXPECT_TRUE(smb_url.InitializeWithUrl(url));

  const std::string expected_url = "smb://192.168.0.1/share";
  EXPECT_TRUE(smb_url.IsValid());
  EXPECT_EQ(expected_url, smb_url.ToString());
}

TEST_F(SmbUrlTest, GetHostWithIp) {
  const std::string url = "smb://192.168.0.1/share";
  SmbUrl smb_url;
  EXPECT_TRUE(smb_url.InitializeWithUrl(url));

  const std::string expected_host = "192.168.0.1";
  EXPECT_EQ(expected_host, smb_url.GetHost());
}

TEST_F(SmbUrlTest, GetHostWithDomain) {
  const std::string url = "smb://server/share";
  SmbUrl smb_url;
  EXPECT_TRUE(smb_url.InitializeWithUrl(url));

  const std::string expected_host = "server";
  EXPECT_EQ(expected_host, smb_url.GetHost());
}

TEST_F(SmbUrlTest, HostBecomesLowerCase) {
  const std::string url = "smb://SERVER/share";
  SmbUrl smb_url;
  EXPECT_TRUE(smb_url.InitializeWithUrl(url));

  const std::string expected_host = "server";
  EXPECT_EQ(expected_host, smb_url.GetHost());

  const std::string expected_url = "smb://server/share";
  EXPECT_EQ(expected_url, smb_url.ToString());
}

TEST_F(SmbUrlTest, ReplacesHost) {
  const std::string url = "smb://server/share";
  const std::string new_host = "192.168.0.1";
  SmbUrl smb_url;
  EXPECT_TRUE(smb_url.InitializeWithUrl(url));

  const std::string expected_host = "server";
  EXPECT_EQ(expected_host, smb_url.GetHost());

  const std::string expected_url = "smb://192.168.0.1/share";
  EXPECT_EQ(expected_url, smb_url.ReplaceHost(new_host));

  // GetHost returns the original host.
  EXPECT_EQ(expected_host, smb_url.GetHost());
}

}  // namespace smb_client
}  // namespace chromeos
