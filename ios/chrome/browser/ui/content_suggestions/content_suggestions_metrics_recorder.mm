// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_metrics_recorder.h"

#include "base/mac/foundation_util.h"
#include "components/ntp_snippets/content_suggestions_metrics.h"
#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_item.h"
#import "ios/chrome/browser/ui/content_suggestions/cells/suggested_content.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_category_wrapper.h"
#import "ios/chrome/browser/ui/content_suggestions/identifier/content_suggestion_identifier.h"
#import "ios/chrome/browser/ui/content_suggestions/identifier/content_suggestions_section_information.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContentSuggestionsMetricsRecorder

@synthesize delegate = _delegate;

#pragma mark - Public

- (void)onSuggestionOpened:(ContentSuggestionsItem*)item
               atIndexPath:(NSIndexPath*)indexPath
        sectionsShownAbove:(NSInteger)sectionsShownAbove
     suggestionsShownAbove:(NSInteger)suggestionsAbove
                withAction:(WindowOpenDisposition)action {
  ContentSuggestionsSectionInformation* sectionInfo =
      item.suggestionIdentifier.sectionInfo;
  ContentSuggestionsCategoryWrapper* categoryWrapper =
      [self.delegate categoryWrapperForSectionInfo:sectionInfo];

  ntp_snippets::metrics::OnSuggestionOpened(
      suggestionsAbove + indexPath.item, [categoryWrapper category],
      sectionsShownAbove, indexPath.item, item.publishDate, item.score, action,
      /*is_prefetched=*/false, /*is_offline=*/false);
}

- (void)onMenuOpenedForSuggestion:(ContentSuggestionsItem*)item
                      atIndexPath:(NSIndexPath*)indexPath
            suggestionsShownAbove:(NSInteger)suggestionsAbove {
  ContentSuggestionsSectionInformation* sectionInfo =
      item.suggestionIdentifier.sectionInfo;
  ContentSuggestionsCategoryWrapper* categoryWrapper =
      [self.delegate categoryWrapperForSectionInfo:sectionInfo];

  ntp_snippets::metrics::OnSuggestionMenuOpened(
      suggestionsAbove + indexPath.item, [categoryWrapper category],
      indexPath.item, item.publishDate, item.score);
}

#pragma mark - ContentSuggestionsMetricsRecording

- (void)onSuggestionShown:(CollectionViewItem*)item
              atIndexPath:(NSIndexPath*)indexPath
    suggestionsShownAbove:(NSInteger)suggestionsAbove {
  ContentSuggestionsItem* suggestion =
      base::mac::ObjCCastStrict<ContentSuggestionsItem>(item);
  ContentSuggestionsSectionInformation* sectionInfo =
      suggestion.suggestionIdentifier.sectionInfo;
  ContentSuggestionsCategoryWrapper* categoryWrapper =
      [self.delegate categoryWrapperForSectionInfo:sectionInfo];

  ntp_snippets::metrics::OnSuggestionShown(
      suggestionsAbove + indexPath.item, [categoryWrapper category],
      indexPath.item, suggestion.publishDate, suggestion.score,
      suggestion.fetchDate);
}

- (void)onMoreButtonTappedAtPosition:(NSInteger)position
                           inSection:(ContentSuggestionsSectionInformation*)
                                         sectionInfo {
  ContentSuggestionsCategoryWrapper* categoryWrapper =
      [self.delegate categoryWrapperForSectionInfo:sectionInfo];

  ntp_snippets::metrics::OnMoreButtonClicked([categoryWrapper category],
                                             position);
}

- (void)onSuggestionDismissed:(CollectionViewItem<SuggestedContent>*)item
                  atIndexPath:(NSIndexPath*)indexPath
        suggestionsShownAbove:(NSInteger)suggestionsAbove {
  ContentSuggestionsSectionInformation* sectionInfo =
      item.suggestionIdentifier.sectionInfo;
  ContentSuggestionsCategoryWrapper* categoryWrapper =
      [self.delegate categoryWrapperForSectionInfo:sectionInfo];

  ntp_snippets::metrics::OnSuggestionDismissed(
      suggestionsAbove + indexPath.item, [categoryWrapper category],
      indexPath.item, /*visited=*/false);
}

@end
