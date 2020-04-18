// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_navigator_params.h"

#include "build/build_config.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#endif

using content::GlobalRequestID;
using content::NavigationController;
using content::WebContents;

#if defined(OS_ANDROID)
NavigateParams::NavigateParams(std::unique_ptr<WebContents> contents_to_insert)
    : contents_to_insert(std::move(contents_to_insert)) {}
#else
NavigateParams::NavigateParams(Browser* a_browser,
                               const GURL& a_url,
                               ui::PageTransition a_transition)
    : url(a_url), transition(a_transition), browser(a_browser) {}

NavigateParams::NavigateParams(Browser* a_browser,
                               std::unique_ptr<WebContents> contents_to_insert)
    : contents_to_insert(std::move(contents_to_insert)), browser(a_browser) {}
#endif  // !defined(OS_ANDROID)

NavigateParams::NavigateParams(Profile* a_profile,
                               const GURL& a_url,
                               ui::PageTransition a_transition)
    : url(a_url),
      disposition(WindowOpenDisposition::NEW_FOREGROUND_TAB),
      transition(a_transition),
      window_action(SHOW_WINDOW),
      initiating_profile(a_profile) {}

NavigateParams::NavigateParams(NavigateParams&&) = default;

NavigateParams::~NavigateParams() {}

void NavigateParams::FillNavigateParamsFromOpenURLParams(
    const content::OpenURLParams& params) {
  this->referrer = params.referrer;
  this->source_site_instance = params.source_site_instance;
  this->frame_tree_node_id = params.frame_tree_node_id;
  this->redirect_chain = params.redirect_chain;
  this->extra_headers = params.extra_headers;
  this->disposition = params.disposition;
  this->trusted_source = false;
  this->is_renderer_initiated = params.is_renderer_initiated;
  this->should_replace_current_entry = params.should_replace_current_entry;
  this->uses_post = params.uses_post;
  this->post_data = params.post_data;
  this->started_from_context_menu = params.started_from_context_menu;
  this->open_pwa_window_if_possible = params.open_app_window_if_possible;
}
