// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/clipboard_url_provider.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/open_from_clipboard/fake_clipboard_recent_content.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char kCurrentURL[] = "http://example.com/current";
const char kClipboardURL[] = "http://example.com/clipboard";

class ClipboardURLProviderTest : public testing::Test {
 public:
  ClipboardURLProviderTest()
      : client_(new MockAutocompleteProviderClient()),
        provider_(new ClipboardURLProvider(client_.get(),
                                           nullptr,
                                           &clipboard_content_)) {
    SetClipboardUrl(GURL(kClipboardURL));
  }

  ~ClipboardURLProviderTest() override {}

  void ClearClipboard() { clipboard_content_.SuppressClipboardContent(); }

  void SetClipboardUrl(const GURL& url) {
    clipboard_content_.SetClipboardContent(url,
                                           base::TimeDelta::FromMinutes(10));
  }

  AutocompleteInput CreateAutocompleteInput(bool from_omnibox_focus) {
    AutocompleteInput input(base::string16(), metrics::OmniboxEventProto::OTHER,
                            classifier_);
    input.set_current_url(GURL(kCurrentURL));
    input.set_from_omnibox_focus(from_omnibox_focus);
    return input;
  }

 protected:
  TestSchemeClassifier classifier_;
  FakeClipboardRecentContent clipboard_content_;
  std::unique_ptr<MockAutocompleteProviderClient> client_;
  scoped_refptr<ClipboardURLProvider> provider_;
};

TEST_F(ClipboardURLProviderTest, NotFromOmniboxFocus) {
  provider_->Start(CreateAutocompleteInput(false), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(ClipboardURLProviderTest, EmptyClipboard) {
  ClearClipboard();
  provider_->Start(CreateAutocompleteInput(true), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(ClipboardURLProviderTest, ClipboardIsCurrentURL) {
  SetClipboardUrl(GURL(kCurrentURL));
  provider_->Start(CreateAutocompleteInput(true), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(ClipboardURLProviderTest, HasMultipleMatches) {
  provider_->Start(CreateAutocompleteInput(true), false);
  ASSERT_GE(provider_->matches().size(), 1U);
  EXPECT_EQ(GURL(kClipboardURL), provider_->matches().back().destination_url);
}

}  // namespace
