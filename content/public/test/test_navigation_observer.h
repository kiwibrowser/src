// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_NAVIGATION_OBSERVER_H_
#define CONTENT_PUBLIC_TEST_TEST_NAVIGATION_OBSERVER_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class WebContents;

// For browser_tests, which run on the UI thread, run a second
// MessageLoop and quit when the navigation completes loading.
class TestNavigationObserver {
 public:
  enum class WaitEvent {
    kLoadStopped,
    kNavigationFinished,
  };

  // Create and register a new TestNavigationObserver against the
  // |web_contents|.
  TestNavigationObserver(WebContents* web_contents,
                         int number_of_navigations,
                         MessageLoopRunner::QuitMode quit_mode =
                             MessageLoopRunner::QuitMode::IMMEDIATE);
  // Like above but waits for one navigation.
  explicit TestNavigationObserver(WebContents* web_contents,
                                  MessageLoopRunner::QuitMode quit_mode =
                                      MessageLoopRunner::QuitMode::IMMEDIATE);
  // Create and register a new TestNavigationObserver that will wait for
  // |target_url| to complete loading or for a committed navigation to
  // |target_url|.
  explicit TestNavigationObserver(const GURL& target_url,
                                  MessageLoopRunner::QuitMode quit_mode =
                                      MessageLoopRunner::QuitMode::IMMEDIATE);

  virtual ~TestNavigationObserver();

  // Runs a nested run loop and blocks until the expected number of navigations
  // stop loading or |target_url| has loaded.
  void Wait();

  // Runs a nested run loop and blocks until the expected number of navigations
  // finished or a navigation to |target_url| has finished.
  void WaitForNavigationFinished();

  // Start/stop watching newly created WebContents.
  void StartWatchingNewWebContents();
  void StopWatchingNewWebContents();

  // Makes this TestNavigationObserver an observer of all previously created
  // WebContents.
  void WatchExistingWebContents();

  const GURL& last_navigation_url() const { return last_navigation_url_; }

  bool last_navigation_succeeded() const { return last_navigation_succeeded_; }

  net::Error last_net_error_code() const { return last_net_error_code_; }

 protected:
  // Register this TestNavigationObserver as an observer of the |web_contents|.
  void RegisterAsObserver(WebContents* web_contents);

  // Protected so that subclasses can retrieve extra information from the
  // |navigation_handle|.
  virtual void OnDidFinishNavigation(NavigationHandle* navigation_handle);

 private:
  class TestWebContentsObserver;

  TestNavigationObserver(WebContents* web_contents,
                         int number_of_navigations,
                         const GURL& target_url,
                         MessageLoopRunner::QuitMode quit_mode =
                             MessageLoopRunner::QuitMode::IMMEDIATE);

  // Callbacks for WebContents-related events.
  void OnWebContentsCreated(WebContents* web_contents);
  void OnWebContentsDestroyed(TestWebContentsObserver* observer,
                              WebContents* web_contents);
  void OnNavigationEntryCommitted(
      TestWebContentsObserver* observer,
      WebContents* web_contents,
      const LoadCommittedDetails& load_details);
  void OnDidAttachInterstitialPage(WebContents* web_contents);
  void OnDidStartLoading(WebContents* web_contents);
  void OnDidStopLoading(WebContents* web_contents);
  void OnDidStartNavigation();
  void EventTriggered();

  // The event that once triggered will quit the run loop.
  WaitEvent wait_event_;

  // If true the navigation has started.
  bool navigation_started_;

  // The number of navigations that have been completed.
  int navigations_completed_;

  // The number of navigations to wait for.
  int number_of_navigations_;

  // The URL to wait for.
  const GURL target_url_;

  // The url of the navigation that last committed.
  GURL last_navigation_url_;

  // True if the last navigation succeeded.
  bool last_navigation_succeeded_;

  // The net error code of the last navigation.
  net::Error last_net_error_code_;

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  // Callback invoked on WebContents creation.
  base::Callback<void(WebContents*)> web_contents_created_callback_;

  // Living TestWebContentsObservers created by this observer.
  std::set<std::unique_ptr<TestWebContentsObserver>> web_contents_observers_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationObserver);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_NAVIGATION_OBSERVER_H_
