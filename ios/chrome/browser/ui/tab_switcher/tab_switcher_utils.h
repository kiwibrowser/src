// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_UTILS_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_UTILS_H_

#include <vector>

class GURL;
@class UIImage;

namespace ios {
class ChromeBrowserState;
}  // namespace ios

typedef void (^TabSwitcherFaviconGetterCompletionBlock)(UIImage*);

// Favicon for |url|, calls |block| when loaded.
void TabSwitcherGetFavicon(GURL const& url,
                           ios::ChromeBrowserState* browserState,
                           TabSwitcherFaviconGetterCompletionBlock block);

// Returns the substitutions/deletions/insertions needed to go from |initial| to
// |final|.
// To be in accordance with the UICollectionView's |performBatchUpdates| method:
// -the indexes in |substitutions| are relative to |initial|.
// -the indexes in |deletions| are relative to |initial|.
// -the indexes in |insertions| are relative to |final|.
//
// The returned sequence is chosen with a preference of insertions and deletions
// over substitutions.
// For example, AB => BC could be done with 2 substitutions, but doing
// 1 insertion and 1 deletion is preferable because it better communicates the
// changes to the user in the UICollectionViews.
void TabSwitcherMinimalReplacementOperations(std::vector<size_t> const& initial,
                                             std::vector<size_t> const& final,
                                             std::vector<size_t>* substitutions,
                                             std::vector<size_t>* deletions,
                                             std::vector<size_t>* insertions);

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_UTILS_H_
