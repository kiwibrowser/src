// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/ad_tracker.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

#include <memory>

namespace blink {

namespace {

class TestAdTracker : public AdTracker {
 public:
  explicit TestAdTracker(LocalFrame* frame) : AdTracker(frame) {}
  void SetScriptAtTopOfStack(String url) { script_at_top_ = url; }
  ~TestAdTracker() override {}

 protected:
  // Returns "" by default unless SetScriptAtTopOfStack is called with
  // something else.
  String ScriptAtTopOfStack(ExecutionContext* execution_context) override {
    return script_at_top_;
  }

 private:
  String script_at_top_;
};

}  // namespace

class AdTrackerTest : public testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;
  LocalFrame* GetFrame() const {
    return page_holder_->GetDocument().GetFrame();
  }

  void WillExecuteScript(const String& script_url) {
    ad_tracker_->WillExecuteScript(script_url);
  }

  bool AnyExecutingScriptsTaggedAsAdResource() {
    return ad_tracker_->IsAdScriptInStack(&page_holder_->GetDocument());
  }

  void AppendToKnownAdScripts(const KURL& url) {
    ad_tracker_->AppendToKnownAdScripts(url);
  }

  Persistent<TestAdTracker> ad_tracker_;
  std::unique_ptr<DummyPageHolder> page_holder_;
};

void AdTrackerTest::SetUp() {
  page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  page_holder_->GetDocument().SetURL(KURL("https://example.com/foo"));
  ad_tracker_ = new TestAdTracker(GetFrame());
}

void AdTrackerTest::TearDown() {
  ad_tracker_->Shutdown();
}

TEST_F(AdTrackerTest, AnyExecutingScriptsTaggedAsAdResource) {
  KURL ad_script_url("https://example.com/bar.js");
  AppendToKnownAdScripts(ad_script_url);

  WillExecuteScript("https://example.com/foo.js");
  WillExecuteScript("https://example.com/bar.js");
  EXPECT_TRUE(AnyExecutingScriptsTaggedAsAdResource());
}

// Tests that if neither script in the stack is an ad,
// AnyExecutingScriptsTaggedAsAdResource should return false.
TEST_F(AdTrackerTest, AnyExecutingScriptsTaggedAsAdResource_False) {
  WillExecuteScript("https://example.com/foo.js");
  WillExecuteScript("https://example.com/bar.js");
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());
}

TEST_F(AdTrackerTest, TopOfStackIncluded) {
  KURL ad_script_url("https://example.com/ad.js");
  AppendToKnownAdScripts(ad_script_url);

  WillExecuteScript("https://example.com/foo.js");
  WillExecuteScript("https://example.com/bar.js");
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());

  ad_tracker_->SetScriptAtTopOfStack("https://www.example.com/baz.js");
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());

  ad_tracker_->SetScriptAtTopOfStack(ad_script_url);
  EXPECT_TRUE(AnyExecutingScriptsTaggedAsAdResource());

  ad_tracker_->SetScriptAtTopOfStack("https://www.example.com/baz.js");
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());

  ad_tracker_->SetScriptAtTopOfStack("");
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());

  ad_tracker_->SetScriptAtTopOfStack(WTF::String());
  EXPECT_FALSE(AnyExecutingScriptsTaggedAsAdResource());

  WillExecuteScript(ad_script_url);
  EXPECT_TRUE(AnyExecutingScriptsTaggedAsAdResource());
}

}  // namespace blink
