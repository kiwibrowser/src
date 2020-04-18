// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_TEST_WEB_STATE_OBSERVER_UTIL_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_TEST_WEB_STATE_OBSERVER_UTIL_H_

#include <memory>

#include "ios/web/public/favicon_url.h"
#include "ios/web/public/load_committed_details.h"
#include "ios/web/public/web_state/form_activity_params.h"
#include "url/gurl.h"

namespace web {

class NavigationContext;
class WebState;

// Arguments passed to |WasShown|.
struct TestWasShownInfo {
  WebState* web_state;
};

// Arguments passed to |WasHidden|.
struct TestWasHiddenInfo {
  WebState* web_state;
};

// Arguments passed to |DidStartNavigation|.
struct TestDidStartNavigationInfo {
  TestDidStartNavigationInfo();
  ~TestDidStartNavigationInfo();
  WebState* web_state;
  std::unique_ptr<web::NavigationContext> context;
};

// Arguments passed to |DidFinishNavigation|.
struct TestDidFinishNavigationInfo {
  TestDidFinishNavigationInfo();
  ~TestDidFinishNavigationInfo();
  WebState* web_state;
  std::unique_ptr<web::NavigationContext> context;
};

// Arguments passed to |NavigationItemCommitted|.
struct TestCommitNavigationInfo {
  WebState* web_state;
  LoadCommittedDetails load_details;
};

// Arguments passed to |PageLoaded|.
struct TestLoadPageInfo {
  WebState* web_state;
  bool success;
};

// Arguments passed to |LoadProgressChanged|.
struct TestChangeLoadingProgressInfo {
  WebState* web_state;
  double progress;
};

// Arguments passed to |NavigationItemsPruned|.
struct TestNavigationItemsPrunedInfo {
  WebState* web_state;
  int count;
};

// Arguments passed to |NavigationItemChanged|.
struct TestNavigationItemChangedInfo {
  WebState* web_state;
};

// Arguments passed to |TitleWasSet|.
struct TestTitleWasSetInfo {
  WebState* web_state;
};

// Arguments passed to |DidChangeVisibleSecurityState|.
struct TestDidChangeVisibleSecurityStateInfo {
  WebState* web_state;
};

// Arguments passed to |DidSuppressDialog|.
struct TestDidSuppressDialogInfo {
  WebState* web_state;
};

// Arguments passed to |DocumentSubmitted|.
struct TestSubmitDocumentInfo {
  WebState* web_state;
  std::string form_name;
  bool user_initiated;
  bool is_main_frame;
};

// Arguments passed to |FormActivityRegistered|.
struct TestFormActivityInfo {
  TestFormActivityInfo();
  ~TestFormActivityInfo();
  WebState* web_state;
  FormActivityParams form_activity;
};

// Arguments passed to |FaviconUrlUpdated|.
struct TestUpdateFaviconUrlCandidatesInfo {
  TestUpdateFaviconUrlCandidatesInfo();
  ~TestUpdateFaviconUrlCandidatesInfo();
  WebState* web_state;
  std::vector<web::FaviconURL> candidates;
};

// Arguments passed to |RenderProcessGone|.
struct TestRenderProcessGoneInfo {
  WebState* web_state;
};

// Arguments passed to |WebStateDestroyed|.
struct TestWebStateDestroyedInfo {
  WebState* web_state;
};

// Arguments passed to |DidStartLoading|.
struct TestStartLoadingInfo {
  WebState* web_state;
};

// Arguments passed to |DidStopLoading|.
struct TestStopLoadingInfo {
  WebState* web_state;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_TEST_WEB_STATE_OBSERVER_UTIL_H_
