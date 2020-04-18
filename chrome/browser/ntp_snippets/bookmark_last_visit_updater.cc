// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/bookmark_last_visit_updater.h"

#include "base/memory/ptr_util.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/ntp_snippets/bookmarks/bookmark_last_visit_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"

namespace {

bool IsMobilePlatform() {
#if defined(OS_ANDROID)  // There are no tab helpers on iOS.
  return true;
#else
  return false;
#endif
}

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(BookmarkLastVisitUpdater);

BookmarkLastVisitUpdater::~BookmarkLastVisitUpdater() {
  bookmark_model_->RemoveObserver(this);
}

// static
void BookmarkLastVisitUpdater::MaybeCreateForWebContentsWithBookmarkModel(
    content::WebContents* web_contents,
    bookmarks::BookmarkModel* bookmark_model) {
  // Do not create the helper for missing |bookmark_model| (in some unit-tests)
  // or for incognito profiles where tracking bookmark visits is not desired.
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  if (!bookmark_model || browser_context->IsOffTheRecord()) {
    return;
  }
  web_contents->SetUserData(UserDataKey(),
                            base::WrapUnique(new BookmarkLastVisitUpdater(
                                web_contents, bookmark_model)));
}

BookmarkLastVisitUpdater::BookmarkLastVisitUpdater(
    content::WebContents* web_contents,
    bookmarks::BookmarkModel* bookmark_model)
    : content::WebContentsObserver(web_contents),
      bookmark_model_(bookmark_model),
      web_contents_(web_contents) {
  DCHECK(bookmark_model_);
  bookmark_model_->AddObserver(this);
}

void BookmarkLastVisitUpdater::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Mark visited bookmark when the navigation starts (may end somewhere else
  // due to server-side redirects).
  NewURLVisited(navigation_handle);
}

void BookmarkLastVisitUpdater::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  // Mark visited bookmark also after each redirect.
  NewURLVisited(navigation_handle);
}

void BookmarkLastVisitUpdater::NewURLVisited(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || navigation_handle->IsErrorPage()) {
    return;
  }

  ntp_snippets::UpdateBookmarkOnURLVisitedInMainFrame(
      bookmark_model_, navigation_handle->GetURL(), IsMobilePlatform());
}

void BookmarkLastVisitUpdater::BookmarkNodeAdded(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int index) {
  const GURL& new_bookmark_url = parent->GetChild(index)->url();

  if (new_bookmark_url == web_contents_->GetLastCommittedURL()) {
    // Consider in this TabHelper only bookmarks created from this tab (and not
    // the ones created from other tabs or created through bookmark sync).
    ntp_snippets::UpdateBookmarkOnURLVisitedInMainFrame(
        bookmark_model_, new_bookmark_url, IsMobilePlatform());
  }
}
