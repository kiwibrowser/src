// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/prerender/prerender_dispatcher.h"

#include <map>
#include <utility>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace prerender {

namespace {

int g_next_prerender_id = 0;

}  // namespace

using blink::WebPrerender;

// Since we can't mock out blink::WebPrerender in chrome, this test can't test
// signalling to or from the WebKit side. Instead, it checks only that the
// messages received from the browser generate consistant state in the
// PrerenderDispatcher. Since prerenders couldn't even start or stop without the
// WebKit signalling, we can expect PrerenderBrowserTest to provide adequate
// coverage of this.
class PrerenderDispatcherTest : public testing::Test {
 public:
  PrerenderDispatcherTest() {}

  bool IsPrerenderURL(const GURL& url) const {
    return prerender_dispatcher_.IsPrerenderURL(url);
  }

  const std::map<int, WebPrerender>& prerenders() const {
    return prerender_dispatcher_.prerenders_;
  }

  int StartPrerender(const GURL& url) {
    DCHECK_EQ(0u, prerender_dispatcher_.prerenders_.count(g_next_prerender_id));
    prerender_dispatcher_.prerenders_[g_next_prerender_id] = WebPrerender();

    prerender_dispatcher_.PrerenderStart(g_next_prerender_id);
    prerender_dispatcher_.PrerenderAddAlias(url);
    return g_next_prerender_id++;
  }

  void AddAliasToPrerender(const GURL& url) {
    prerender_dispatcher_.PrerenderAddAlias(url);
  }

  void RemoveAliasFromPrerender(const GURL& url) {
    std::vector<GURL> urls;
    urls.push_back(url);
    prerender_dispatcher_.PrerenderRemoveAliases(urls);
  }

  void StopPrerender(int prerender_id) {
    prerender_dispatcher_.PrerenderStop(prerender_id);
  }

  int GetCountForURL(const GURL& url) const {
    return prerender_dispatcher_.running_prerender_urls_.count(url);
  }

 private:
  PrerenderDispatcher prerender_dispatcher_;
  DISALLOW_COPY_AND_ASSIGN(PrerenderDispatcherTest);
};

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherEmpty) {
  EXPECT_TRUE(prerenders().empty());
}

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherSingleAdd) {
  GURL foo_url = GURL("http://foo.com");
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  StartPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_EQ(1, GetCountForURL(foo_url));
}

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherMultipleAdd) {
  GURL foo_url = GURL("http://foo.com");
  GURL bar_url = GURL("http://bar.com");

  EXPECT_FALSE(IsPrerenderURL(foo_url));
  EXPECT_FALSE(IsPrerenderURL(bar_url));
  StartPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_FALSE(IsPrerenderURL(bar_url));

  AddAliasToPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_FALSE(IsPrerenderURL(bar_url));
  EXPECT_EQ(2, GetCountForURL(foo_url));

  StartPrerender(bar_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_TRUE(IsPrerenderURL(bar_url));
  EXPECT_EQ(2, GetCountForURL(foo_url));
  EXPECT_EQ(1, GetCountForURL(bar_url));
}

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherSingleRemove) {
  GURL foo_url = GURL("http://foo.com");
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  int foo_id = StartPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  StopPrerender(foo_id);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_EQ(1, GetCountForURL(foo_url));
  RemoveAliasFromPrerender(foo_url);
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  EXPECT_EQ(0, GetCountForURL(foo_url));
}

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherTooManyRemoves) {
  GURL foo_url = GURL("http://foo.com");
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  int foo_id = StartPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  StopPrerender(foo_id);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_EQ(1, GetCountForURL(foo_url));
  RemoveAliasFromPrerender(foo_url);
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  EXPECT_EQ(0, GetCountForURL(foo_url));
  RemoveAliasFromPrerender(foo_url);
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  EXPECT_EQ(0, GetCountForURL(foo_url));
}

TEST_F(PrerenderDispatcherTest, PrerenderDispatcherMultipleRemoves) {
  GURL foo_url = GURL("http://foo.com");
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  int foo_id = StartPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  AddAliasToPrerender(foo_url);
  StopPrerender(foo_id);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_EQ(2, GetCountForURL(foo_url));
  RemoveAliasFromPrerender(foo_url);
  EXPECT_TRUE(IsPrerenderURL(foo_url));
  EXPECT_EQ(1, GetCountForURL(foo_url));
  RemoveAliasFromPrerender(foo_url);
  EXPECT_FALSE(IsPrerenderURL(foo_url));
  EXPECT_EQ(0, GetCountForURL(foo_url));
}

}  // end namespace prerender
