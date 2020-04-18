// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_utils.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/favicon/favicon_loader.h"
#include "ios/chrome/browser/favicon/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

UIImage* DefaultFaviconImage() {
  return NativeImage(IDR_IOS_OMNIBOX_HTTP);
}

enum BacktrackOperation { NOTHING, SUBSTITUTION, DELETION, INSERTION };

BacktrackOperation BacktrackOperationInCostMatrix(
    std::vector<std::vector<int>> const& costMatrix,
    size_t finalIndex,
    size_t initialIndex) {
  DCHECK(finalIndex || initialIndex);
  DCHECK(initialIndex < costMatrix.size());
  DCHECK(finalIndex < costMatrix[initialIndex].size());

  if (finalIndex == 0)
    return DELETION;
  if (initialIndex == 0)
    return INSERTION;

  int currentCost = costMatrix[initialIndex][finalIndex];

  int costBeforeInsertion = costMatrix[initialIndex][finalIndex - 1];
  if (costBeforeInsertion + 1 == currentCost)
    return INSERTION;

  int costBeforeDeletion = costMatrix[initialIndex - 1][finalIndex];
  if (costBeforeDeletion + 1 == currentCost)
    return DELETION;

  int costBeforeSubstitution = costMatrix[initialIndex - 1][finalIndex - 1];
  if (costBeforeSubstitution == currentCost)
    return NOTHING;

  return SUBSTITUTION;
}

}  // namespace

void TabSwitcherGetFavicon(GURL const& url,
                           ios::ChromeBrowserState* browser_state,
                           TabSwitcherFaviconGetterCompletionBlock block) {
  DCHECK(browser_state);
  syncer::SyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state);
  sync_sessions::OpenTabsUIDelegate* open_tabs =
      sync_service ? sync_service->GetOpenTabsUIDelegate() : NULL;
  scoped_refptr<base::RefCountedMemory> favicon;
  if (open_tabs &&
      open_tabs->GetSyncedFaviconForPageURL(url.spec(), &favicon)) {
    dispatch_queue_t queue =
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
    dispatch_async(queue, ^{
      NSData* pngData =
          [NSData dataWithBytes:favicon->front() length:favicon->size()];
      UIImage* image = [[UIImage alloc] initWithData:pngData];
      dispatch_async(dispatch_get_main_queue(), ^{
        // |UIImage initWithData:| may return nil.
        if (image) {
          block(image);
        } else {
          block(DefaultFaviconImage());
        }
      });
    });
    block(DefaultFaviconImage());
    return;
  }

  // Use the FaviconCache if there is no synced favicon.
  FaviconLoader* loader =
      IOSChromeFaviconLoaderFactory::GetForBrowserState(browser_state);
  if (loader) {
    UIImage* image = loader->ImageForURL(
        url,
        {favicon_base::IconType::kFavicon, favicon_base::IconType::kTouchIcon,
         favicon_base::IconType::kTouchPrecomposedIcon},
        block);
    DCHECK(image);
    block(image);
    return;
  }
  // Finally returns a default image.
  block(DefaultFaviconImage());
}

void TabSwitcherMinimalReplacementOperations(std::vector<size_t> const& initial,
                                             std::vector<size_t> const& final,
                                             std::vector<size_t>* substitutions,
                                             std::vector<size_t>* deletions,
                                             std::vector<size_t>* insertions) {
  DCHECK(substitutions);
  DCHECK(deletions);
  DCHECK(insertions);
  DCHECK_EQ(substitutions->size(), 0UL);
  DCHECK_EQ(deletions->size(), 0UL);
  DCHECK_EQ(insertions->size(), 0UL);

  // The substitutions/deletions/insertions are computed using the Levenshtein
  // algorithm.
  // https://en.wikipedia.org/wiki/Levenshtein_distance

  const size_t initialSize = initial.size() + 1;
  const size_t finalSize = final.size() + 1;

  std::vector<std::vector<int>> costMatrix(initialSize,
                                           std::vector<int>(finalSize));

  for (size_t i = 1; i < initialSize; i++)
    costMatrix[i][0] = i;
  for (size_t i = 1; i < finalSize; i++)
    costMatrix[0][i] = i;

  // Step 1: Generate cost matrix.
  for (size_t initialIndex = 1; initialIndex < initialSize; initialIndex++) {
    for (size_t finalIndex = 1; finalIndex < finalSize; finalIndex++) {
      if (initial[initialIndex - 1] == final[finalIndex - 1]) {
        costMatrix[initialIndex][finalIndex] =
            costMatrix[initialIndex - 1][finalIndex - 1];
      } else {
        int costAfterSubstitution =
            costMatrix[initialIndex - 1][finalIndex - 1] + 1;
        int costAfterInsertion = costMatrix[initialIndex][finalIndex - 1] + 1;
        int costAfterDeletion = costMatrix[initialIndex - 1][finalIndex] + 1;
        costMatrix[initialIndex][finalIndex] =
            std::min(std::min(costAfterDeletion, costAfterInsertion),
                     costAfterSubstitution);
      }
    }
  }

  // Step 2: Backtrack to find the operations (deletion, insertion,
  // substitution).
  size_t initialIndex = initialSize - 1;
  size_t finalIndex = finalSize - 1;
  while (initialIndex != 0 || finalIndex != 0) {
    BacktrackOperation op =
        BacktrackOperationInCostMatrix(costMatrix, finalIndex, initialIndex);
    switch (op) {
      case SUBSTITUTION:
        finalIndex--;
        initialIndex--;
        // The substitution is relative to |initial|.
        substitutions->push_back(initialIndex);
        break;
      case DELETION:
        initialIndex--;
        // The deletion is relative to |initial|.
        deletions->push_back(initialIndex);
        break;
      case INSERTION:
        finalIndex--;
        // The insertion is relative to |final|.
        insertions->push_back(finalIndex);
        break;
      case NOTHING:
        finalIndex--;
        initialIndex--;
        break;
    }
  }

  // The backtracking results in the indexes of the operations being stored in
  // decreasing order.
  // For readability, order them in ascending value.
  std::reverse(std::begin(*substitutions), std::end(*substitutions));
  std::reverse(std::begin(*deletions), std::end(*deletions));
  std::reverse(std::begin(*insertions), std::end(*insertions));
}
