// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/content_security_policy/csp_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// Allow() is an abbreviation of CSPSource::Allow(). Useful for writting test
// expectations on one line.
bool Allow(const CSPSource& source,
           const GURL& url,
           CSPContext* context,
           bool is_redirect = false) {
  return CSPSource::Allow(source, url, context, is_redirect);
}

}  // namespace

TEST(CSPSourceTest, BasicMatching) {
  CSPContext context;

  CSPSource source("http", "example.com", false, 8000, false, "/foo/");

  EXPECT_TRUE(Allow(source, GURL("http://example.com:8000/foo/"), &context));
  EXPECT_TRUE(Allow(source, GURL("http://example.com:8000/foo/bar"), &context));
  EXPECT_TRUE(Allow(source, GURL("HTTP://EXAMPLE.com:8000/foo/BAR"), &context));

  EXPECT_FALSE(Allow(source, GURL("http://example.com:8000/bar/"), &context));
  EXPECT_FALSE(Allow(source, GURL("https://example.com:8000/bar/"), &context));
  EXPECT_FALSE(Allow(source, GURL("http://example.com:9000/bar/"), &context));
  EXPECT_FALSE(
      Allow(source, GURL("HTTP://example.com:8000/FOO/bar"), &context));
  EXPECT_FALSE(
      Allow(source, GURL("HTTP://example.com:8000/FOO/BAR"), &context));
}

TEST(CSPSourceTest, AllowScheme) {
  CSPContext context;

  // http -> {http, https}.
  {
    CSPSource source("http", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
    // This passes because the source is "scheme only" so the upgrade is
    // allowed.
    EXPECT_TRUE(Allow(source, GURL("https://a.com:80"), &context));
    EXPECT_FALSE(Allow(source, GURL("ftp://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("ws://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("wss://a.com"), &context));
  }

  // ws -> {ws, wss}.
  {
    CSPSource source("ws", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("https://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("ftp://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("ws://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("wss://a.com"), &context));
  }

  // Exact matches required (ftp)
  {
    CSPSource source("ftp", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("ftp://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
  }

  // Exact matches required (https)
  {
    CSPSource source("https", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
  }

  // Exact matches required (wss)
  {
    CSPSource source("wss", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("wss://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("ws://a.com"), &context));
  }

  // Scheme is empty (ProtocolMatchesSelf).
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));

    // Self's scheme is http.
    context.SetSelf(url::Origin::Create(GURL("http://a.com")));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("ftp://a.com"), &context));

    // Self's is https.
    context.SetSelf(url::Origin::Create(GURL("https://a.com")));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("ftp://a.com"), &context));

    // Self's scheme is not in the http familly.
    context.SetSelf(url::Origin::Create(GURL("ftp://a.com/")));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("ftp://a.com"), &context));

    // Self's scheme is unique (non standard scheme).
    context.SetSelf(url::Origin::Create(GURL("non-standard-scheme://a.com")));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("non-standard-scheme://a.com"), &context));

    // Self's scheme is unique (data-url).
    context.SetSelf(
        url::Origin::Create(GURL("data:text/html,<iframe src=[...]>")));
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("data:text/html,hello"), &context));
  }
}

TEST(CSPSourceTest, AllowHost) {
  CSPContext context;
  context.SetSelf(url::Origin::Create(GURL("http://example.com")));

  // Host is * (source-expression = "http://*")
  {
    CSPSource source("http", "", true, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://."), &context));
  }

  // Host is *.foo.bar
  {
    CSPSource source("", "foo.bar", true, url::PORT_UNSPECIFIED, false, "");
    EXPECT_FALSE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://bar"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://foo.bar"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://o.bar"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://*.foo.bar"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://sub.foo.bar"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://sub.sub.foo.bar"), &context));
    // Please see http://crbug.com/692505
    EXPECT_TRUE(Allow(source, GURL("http://.foo.bar"), &context));
  }

  // Host is exact.
  {
    CSPSource source("", "foo.bar", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://foo.bar"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://sub.foo.bar"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://bar"), &context));
    // Please see http://crbug.com/692505
    EXPECT_FALSE(Allow(source, GURL("http://.foo.bar"), &context));
  }
}

TEST(CSPSourceTest, AllowPort) {
  CSPContext context;
  context.SetSelf(url::Origin::Create(GURL("http://example.com")));

  // Source's port unspecified.
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com:80"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com:8080"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com:443"), &context));
    EXPECT_FALSE(Allow(source, GURL("https://a.com:80"), &context));
    EXPECT_FALSE(Allow(source, GURL("https://a.com:8080"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com:443"), &context));
    EXPECT_FALSE(Allow(source, GURL("unknown://a.com:80"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
  }

  // Source's port is "*".
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, true, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com:80"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com:8080"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com:8080"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com:0"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
  }

  // Source has a port.
  {
    CSPSource source("", "a.com", false, 80, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com:80"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com:8080"), &context));
    EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context));
  }

  // Allow upgrade from :80 to :443
  {
    CSPSource source("", "a.com", false, 80, false, "");
    EXPECT_TRUE(Allow(source, GURL("https://a.com:443"), &context));
    // Should not allow scheme upgrades unless both port and scheme are
    // upgraded.
    EXPECT_FALSE(Allow(source, GURL("http://a.com:443"), &context));
  }

  // Host is * but port is specified
  {
    CSPSource source("http", "", true, 111, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com:111"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com:222"), &context));
  }
}

TEST(CSPSourceTest, AllowPath) {
  CSPContext context;
  context.SetSelf(url::Origin::Create(GURL("http://example.com")));

  // Path to a file
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false,
                     "/path/to/file");
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/file"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com/path/to/"), &context));
    EXPECT_FALSE(
        Allow(source, GURL("http://a.com/path/to/file/subpath"), &context));
    EXPECT_FALSE(
        Allow(source, GURL("http://a.com/path/to/something"), &context));
  }

  // Path to a directory
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false,
                     "/path/to/");
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/file"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com/path/"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com/path/to"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com/path/to"), &context));
  }

  // Empty path
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/file"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
  }

  // Almost empty path
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "/");
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/file"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/path/to/"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com"), &context));
  }

  // Path encoded.
  {
    CSPSource source("http", "a.com", false, url::PORT_UNSPECIFIED, false,
                     "/Hello Günter");
    EXPECT_TRUE(
        Allow(source, GURL("http://a.com/Hello%20G%C3%BCnter"), &context));
    EXPECT_TRUE(Allow(source, GURL("http://a.com/Hello Günter"), &context));
  }

  // Host is * but path is specified.
  {
    CSPSource source("http", "", true, url::PORT_UNSPECIFIED, false,
                     "/allowed-path");
    EXPECT_TRUE(Allow(source, GURL("http://a.com/allowed-path"), &context));
    EXPECT_FALSE(Allow(source, GURL("http://a.com/disallowed-path"), &context));
  }
}

TEST(CSPSourceTest, RedirectMatching) {
  CSPContext context;
  CSPSource source("http", "a.com", false, 8000, false, "/bar/");
  EXPECT_TRUE(Allow(source, GURL("http://a.com:8000/"), &context, true));
  EXPECT_TRUE(Allow(source, GURL("http://a.com:8000/foo"), &context, true));
  EXPECT_FALSE(Allow(source, GURL("https://a.com:8000/foo"), &context, true));
  EXPECT_FALSE(
      Allow(source, GURL("http://not-a.com:8000/foo"), &context, true));
  EXPECT_FALSE(Allow(source, GURL("http://a.com:9000/foo/"), &context, false));
}

TEST(CSPSourceTest, ToString) {
  {
    CSPSource source("http", "", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_EQ("http:", source.ToString());
  }
  {
    CSPSource source("http", "a.com", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_EQ("http://a.com", source.ToString());
  }
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "");
    EXPECT_EQ("a.com", source.ToString());
  }
  {
    CSPSource source("", "a.com", true, url::PORT_UNSPECIFIED, false, "");
    EXPECT_EQ("*.a.com", source.ToString());
  }
  {
    CSPSource source("", "", true, url::PORT_UNSPECIFIED, false, "");
    EXPECT_EQ("*", source.ToString());
  }
  {
    CSPSource source("", "a.com", false, 80, false, "");
    EXPECT_EQ("a.com:80", source.ToString());
  }
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, true, "");
    EXPECT_EQ("a.com:*", source.ToString());
  }
  {
    CSPSource source("", "a.com", false, url::PORT_UNSPECIFIED, false, "/path");
    EXPECT_EQ("a.com/path", source.ToString());
  }
}

TEST(CSPSourceTest, UpgradeRequests) {
  CSPContext context;
  CSPSource source("http", "a.com", false, 80, false, "");
  EXPECT_TRUE(Allow(source, GURL("http://a.com:80"), &context, true));
  EXPECT_FALSE(Allow(source, GURL("https://a.com:80"), &context, true));
  EXPECT_FALSE(Allow(source, GURL("http://a.com:443"), &context, true));
  EXPECT_TRUE(Allow(source, GURL("https://a.com:443"), &context, true));
  EXPECT_TRUE(Allow(source, GURL("https://a.com"), &context, true));
}

}  // namespace content
