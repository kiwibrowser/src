// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_METRICS_RECORDER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_METRICS_RECORDER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_metrics_recording.h"

@class ContentSuggestionsCategoryWrapper;
@class ContentSuggestionsItem;
@class ContentSuggestionsSectionInformation;

// Delegate for the metrics recorder.
@protocol ContentSuggestionsMetricsRecorderDelegate

// Returns a CategoryWrapper corresponding to this |sectionInfo|.
- (nullable ContentSuggestionsCategoryWrapper*)categoryWrapperForSectionInfo:
    (nonnull ContentSuggestionsSectionInformation*)sectionInfo;

@end

// Records different metrics for ContentSuggestions
@interface ContentSuggestionsMetricsRecorder
    : NSObject<ContentSuggestionsMetricsRecording>

@property(nonatomic, weak, nullable)
    id<ContentSuggestionsMetricsRecorderDelegate>
        delegate;

// Records the opening of an |item| suggestion at the |indexPath|. Also needs
// the number of |sectionsShownAbove| this section and the number of
// |suggestionsAbove| its section. The item is opened with |action|.
- (void)onSuggestionOpened:(nonnull ContentSuggestionsItem*)item
               atIndexPath:(nonnull NSIndexPath*)indexPath
        sectionsShownAbove:(NSInteger)sectionsShownAbove
     suggestionsShownAbove:(NSInteger)suggestionsAbove
                withAction:(WindowOpenDisposition)action;

// Records the opening of a context menu for an |item| at the |indexPath|. Needs
// the number of |suggestionsAbove| the section of the item.
- (void)onMenuOpenedForSuggestion:(nonnull ContentSuggestionsItem*)item
                      atIndexPath:(nonnull NSIndexPath*)indexPath
            suggestionsShownAbove:(NSInteger)suggestionsAbove;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_METRICS_RECORDER_H_
