// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERT_FACTORY_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERT_FACTORY_H_

#import <UIKit/UIKit.h>

@class AlertCoordinator;
@class ContentSuggestionsItem;
@class ContentSuggestionsMostVisitedItem;
@protocol ContentSuggestionsGestureCommands;

// Factory for AlertCoordinators for ContentSuggestions.
@interface ContentSuggestionsAlertFactory : NSObject

// Returns an AlertCoordinator for a suggestions |item| with the indexPath
// |indexPath|. The alert will be presented on the |viewController| at the
// |touchLocation|, in the coordinates of the |viewController|'s collectionView.
// The |commandHandler| will receive callbacks when the user chooses one of the
// options displayed by the alert.
+ (AlertCoordinator*)
alertCoordinatorForSuggestionItem:(ContentSuggestionsItem*)item
                 onViewController:(UICollectionViewController*)viewController
                          atPoint:(CGPoint)touchLocation
                      atIndexPath:(NSIndexPath*)indexPath
                  readLaterAction:(BOOL)readLaterAction
                   commandHandler:
                       (id<ContentSuggestionsGestureCommands>)commandHandler;

// Same as above but for a MostVisited item.
+ (AlertCoordinator*)
alertCoordinatorForMostVisitedItem:(ContentSuggestionsMostVisitedItem*)item
                  onViewController:(UICollectionViewController*)viewController
                           atPoint:(CGPoint)touchLocation
                       atIndexPath:(NSIndexPath*)indexPath
                    commandHandler:
                        (id<ContentSuggestionsGestureCommands>)commandHandler;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_ALERT_FACTORY_H_
