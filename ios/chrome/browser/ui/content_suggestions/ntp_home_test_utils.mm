// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/ntp_home_test_utils.h"

#include <string>

#include "base/callback.h"
#include "base/mac/foundation_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/status.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_view_controller.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Helper method to get the Article category.
ntp_snippets::Category Category() {
  return ntp_snippets::Category::FromKnownCategory(
      ntp_snippets::KnownCategories::ARTICLES);
}

// Creates a suggestion with a |title| and |url|
ntp_snippets::ContentSuggestion Suggestion(std::string title, GURL url) {
  ntp_snippets::ContentSuggestion suggestion(Category(), title, url);
  suggestion.set_title(base::UTF8ToUTF16(title));

  return suggestion;
}

// Returns the subview of |parentView| corresponding to the
// ContentSuggestionsViewController. Returns nil if it is not in its subviews.
UIView* SubviewWithAccessibilityIdentifier(NSString* accessibilityID,
                                           UIView* parentView) {
  if (parentView.accessibilityIdentifier == accessibilityID) {
    return parentView;
  }
  if (parentView.subviews.count == 0)
    return nil;
  for (UIView* view in parentView.subviews) {
    UIView* resultView =
        SubviewWithAccessibilityIdentifier(accessibilityID, view);
    if (resultView)
      return resultView;
  }
  return nil;
}

}  // namespace

namespace ntp_home {
id<GREYMatcher> OmniboxWidth(CGFloat width) {
  MatchesBlock matches = ^BOOL(UIView* view) {
    return fabs(view.bounds.size.width - width) < 0.001;
  };
  DescribeToBlock describe = ^void(id<GREYDescription> description) {
    [description
        appendText:[NSString stringWithFormat:@"Omnibox has correct width: %g",
                                              width]];
  };

  return [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                              descriptionBlock:describe];
}

id<GREYMatcher> OmniboxWidthBetween(CGFloat width, CGFloat margin) {
  MatchesBlock matches = ^BOOL(UIView* view) {
    return view.bounds.size.width >= width - margin &&
           view.bounds.size.width <= width + margin;
  };
  DescribeToBlock describe = ^void(id<GREYDescription> description) {
    [description
        appendText:[NSString
                       stringWithFormat:
                           @"Omnibox has correct width: %g with margin: %g",
                           width, margin]];
  };

  return [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                              descriptionBlock:describe];
}

id<GREYMatcher> HeaderPinnedOffset(CGFloat offset) {
  MatchesBlock matches = ^BOOL(UIView* view) {
    return view.frame.origin.y == offset;
  };
  DescribeToBlock describe = ^void(id<GREYDescription> description) {
    [description
        appendText:[NSString
                       stringWithFormat:@"CSHeader has correct offset: %g",
                                        offset]];
  };

  return [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                              descriptionBlock:describe];
}

UICollectionView* CollectionView() {
  return base::mac::ObjCCast<UICollectionView>(
      SubviewWithAccessibilityIdentifier(
          [ContentSuggestionsViewController collectionAccessibilityIdentifier],
          [[UIApplication sharedApplication] keyWindow]));
}

UIView* FakeOmnibox() {
  return SubviewWithAccessibilityIdentifier(
      FakeOmniboxAccessibilityID(),
      [[UIApplication sharedApplication] keyWindow]);
}

std::vector<ntp_snippets::ContentSuggestion> Suggestions() {
  std::vector<ntp_snippets::ContentSuggestion> suggestions;
  suggestions.emplace_back(
      Suggestion("chromium1", GURL("http://chromium.org/1")));
  suggestions.emplace_back(
      Suggestion("chromium2", GURL("http://chromium.org/2")));
  suggestions.emplace_back(
      Suggestion("chromium3", GURL("http://chromium.org/3")));
  suggestions.emplace_back(
      Suggestion("chromium4", GURL("http://chromium.org/4")));
  suggestions.emplace_back(
      Suggestion("chromium5", GURL("http://chromium.org/5")));
  suggestions.emplace_back(
      Suggestion("chromium6", GURL("http://chromium.org/6")));
  suggestions.emplace_back(
      Suggestion("chromium7", GURL("http://chromium.org/7")));
  suggestions.emplace_back(
      Suggestion("chromium8", GURL("http://chromium.org/8")));
  suggestions.emplace_back(
      Suggestion("chromium9", GURL("http://chromium.org/9")));
  suggestions.emplace_back(
      Suggestion("chromium10", GURL("http://chromium.org/10")));
  return suggestions;
}

}  // namespace ntp_home

namespace ntp_snippets {

AdditionalSuggestionsHelper::AdditionalSuggestionsHelper(const GURL& url)
    : url_(url) {}

void AdditionalSuggestionsHelper::SendAdditionalSuggestions(
    FetchDoneCallback* callback) {
  std::vector<ContentSuggestion> suggestions;
  for (int i = 0; i < 10; i++) {
    std::string title = "AdditionalSuggestion" + std::to_string(i);
    suggestions.emplace_back(Suggestion(title, url_));
  }
  std::move(*callback).Run(Status(StatusCode::SUCCESS, ""),
                           std::move(suggestions));
}

}  // namespace ntp_snippets
