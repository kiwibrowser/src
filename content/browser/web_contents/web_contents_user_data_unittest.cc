// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_contents_user_data.h"

#include <memory>

#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class WebContentsAttachedClass1
    : public WebContentsUserData<WebContentsAttachedClass1> {
 public:
  ~WebContentsAttachedClass1() override {}

 private:
  explicit WebContentsAttachedClass1(WebContents* contents) {}
  friend class WebContentsUserData<WebContentsAttachedClass1>;
};

class WebContentsAttachedClass2
    : public WebContentsUserData<WebContentsAttachedClass2> {
 public:
  ~WebContentsAttachedClass2() override {}

 private:
  explicit WebContentsAttachedClass2(WebContents* contents) {}
  friend class WebContentsUserData<WebContentsAttachedClass2>;
};

DEFINE_WEB_CONTENTS_USER_DATA_KEY(WebContentsAttachedClass1);
DEFINE_WEB_CONTENTS_USER_DATA_KEY(WebContentsAttachedClass2);

typedef RenderViewHostTestHarness WebContentsUserDataTest;

TEST_F(WebContentsUserDataTest, OneInstanceTwoAttachments) {
  WebContents* contents = web_contents();
  WebContentsAttachedClass1* class1 =
      WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_EQ(nullptr, class1);
  WebContentsAttachedClass2* class2 =
      WebContentsAttachedClass2::FromWebContents(contents);
  ASSERT_EQ(nullptr, class2);

  WebContentsAttachedClass1::CreateForWebContents(contents);
  class1 = WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_TRUE(class1 != nullptr);
  class2 = WebContentsAttachedClass2::FromWebContents(contents);
  ASSERT_EQ(nullptr, class2);

  WebContentsAttachedClass2::CreateForWebContents(contents);
  WebContentsAttachedClass1* class1again =
      WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_TRUE(class1again != nullptr);
  class2 = WebContentsAttachedClass2::FromWebContents(contents);
  ASSERT_TRUE(class2 != nullptr);
  ASSERT_EQ(class1, class1again);
  ASSERT_NE(static_cast<void*>(class1), static_cast<void*>(class2));
}

TEST_F(WebContentsUserDataTest, TwoInstancesOneAttachment) {
  WebContents* contents1 = web_contents();
  std::unique_ptr<WebContents> contents2(
      WebContentsTester::CreateTestWebContents(browser_context(), nullptr));

  WebContentsAttachedClass1* one_class =
      WebContentsAttachedClass1::FromWebContents(contents1);
  ASSERT_EQ(nullptr, one_class);
  WebContentsAttachedClass1* two_class =
      WebContentsAttachedClass1::FromWebContents(contents2.get());
  ASSERT_EQ(nullptr, two_class);

  WebContentsAttachedClass1::CreateForWebContents(contents1);
  one_class = WebContentsAttachedClass1::FromWebContents(contents1);
  ASSERT_TRUE(one_class != nullptr);
  two_class = WebContentsAttachedClass1::FromWebContents(contents2.get());
  ASSERT_EQ(nullptr, two_class);

  WebContentsAttachedClass1::CreateForWebContents(contents2.get());
  WebContentsAttachedClass1* one_class_again =
      WebContentsAttachedClass1::FromWebContents(contents1);
  ASSERT_TRUE(one_class_again != nullptr);
  two_class = WebContentsAttachedClass1::FromWebContents(contents2.get());
  ASSERT_TRUE(two_class != nullptr);
  ASSERT_EQ(one_class, one_class_again);
  ASSERT_NE(one_class, two_class);
}

TEST_F(WebContentsUserDataTest, Idempotence) {
  WebContents* contents = web_contents();
  WebContentsAttachedClass1* clazz =
      WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_EQ(nullptr, clazz);

  WebContentsAttachedClass1::CreateForWebContents(contents);
  clazz = WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_TRUE(clazz != nullptr);

  WebContentsAttachedClass1::CreateForWebContents(contents);
  WebContentsAttachedClass1* class_again =
      WebContentsAttachedClass1::FromWebContents(contents);
  ASSERT_TRUE(class_again != nullptr);
  ASSERT_EQ(clazz, class_again);
}

}  // namespace content
