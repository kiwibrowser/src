// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_session_service_impl.h"

#include <map>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/scoped_observer.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/sessions/session_ios.h"
#import "ios/chrome/browser/sessions/session_service_ios.h"
#import "ios/chrome/browser/sessions/session_window_ios.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list_serialization.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// BrowserListSessionServiceWebStateListObserver observes a WebStateList and
// invokes |closure| when the active WebState changes or a navigation item is
// committed in the active WebState.
class BrowserListSessionServiceWebStateListObserver
    : public WebStateListObserver,
      public web::WebStateObserver {
 public:
  BrowserListSessionServiceWebStateListObserver(
      WebStateList* web_state_list,
      const base::RepeatingClosure& closure);
  ~BrowserListSessionServiceWebStateListObserver() override;

  // WebStateListObserver implementation.
  void WebStateActivatedAt(WebStateList* web_state_list,
                           web::WebState* old_web_state,
                           web::WebState* new_web_state,
                           int active_index,
                           int reason) override;

  // web::WebStateObserver implementation.
  void NavigationItemCommitted(
      web::WebState* web_state,
      const web::LoadCommittedDetails& load_details) override;
  void WebStateDestroyed(web::WebState* web_state) override;

 private:
  WebStateList* web_state_list_;
  base::RepeatingClosure closure_;
  ScopedObserver<web::WebState, web::WebStateObserver> scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListSessionServiceWebStateListObserver);
};

BrowserListSessionServiceWebStateListObserver::
    BrowserListSessionServiceWebStateListObserver(
        WebStateList* web_state_list,
        const base::RepeatingClosure& closure)
    : web_state_list_(web_state_list),
      closure_(closure),
      scoped_observer_(this) {
  DCHECK(!closure_.is_null());
  web_state_list_->AddObserver(this);
  if (web_state_list_->active_index() != WebStateList::kInvalidIndex) {
    scoped_observer_.Add(web_state_list_->GetActiveWebState());
  }
}

BrowserListSessionServiceWebStateListObserver::
    ~BrowserListSessionServiceWebStateListObserver() {
  web_state_list_->RemoveObserver(this);
  scoped_observer_.RemoveAll();
}

void BrowserListSessionServiceWebStateListObserver::WebStateActivatedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int active_index,
    int reason) {
  if (old_web_state) {
    scoped_observer_.Remove(old_web_state);
    closure_.Run();
  }
  if (new_web_state) {
    scoped_observer_.Add(new_web_state);
  }
}

void BrowserListSessionServiceWebStateListObserver::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  closure_.Run();
}

void BrowserListSessionServiceWebStateListObserver::WebStateDestroyed(
    web::WebState* web_state) {
  NOTREACHED();
}

// BrowserListSessionServiceBrowserListObserver observes a BrowserList and
// invokes |closure| then the active WebState changes in any of the Browsers'
// WebStates or a navigation item is committed in one of the active WebStates.
class BrowserListSessionServiceBrowserListObserver
    : public BrowserListObserver {
 public:
  BrowserListSessionServiceBrowserListObserver(
      BrowserList* browser_list,
      const base::RepeatingClosure& closure);
  ~BrowserListSessionServiceBrowserListObserver() override;

  // BrowserListObserver implementation.
  void OnBrowserCreated(BrowserList* browser_list, Browser* browser) override;
  void OnBrowserRemoved(BrowserList* browser_list, Browser* browser) override;

 private:
  BrowserList* browser_list_;
  base::RepeatingClosure closure_;
  std::map<Browser*, std::unique_ptr<WebStateListObserver>> observers_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListSessionServiceBrowserListObserver);
};

BrowserListSessionServiceBrowserListObserver::
    BrowserListSessionServiceBrowserListObserver(
        BrowserList* browser_list,
        const base::RepeatingClosure& closure)
    : browser_list_(browser_list), closure_(closure) {
  DCHECK(!closure_.is_null());
  for (int index = 0; index < browser_list_->GetCount(); ++index)
    OnBrowserCreated(browser_list_, browser_list_->GetBrowserAtIndex(index));
  browser_list_->AddObserver(this);
}

BrowserListSessionServiceBrowserListObserver::
    ~BrowserListSessionServiceBrowserListObserver() {
  browser_list_->RemoveObserver(this);
}

void BrowserListSessionServiceBrowserListObserver::OnBrowserCreated(
    BrowserList* browser_list,
    Browser* browser) {
  DCHECK(browser);
  DCHECK_EQ(browser_list, browser_list_);
  DCHECK(observers_.find(browser) == observers_.end());
  observers_.insert(std::make_pair(
      browser, std::make_unique<BrowserListSessionServiceWebStateListObserver>(
                   &browser->web_state_list(), closure_)));
}

void BrowserListSessionServiceBrowserListObserver::OnBrowserRemoved(
    BrowserList* browser_list,
    Browser* browser) {
  DCHECK(browser);
  DCHECK_EQ(browser_list, browser_list_);
  DCHECK(observers_.find(browser) != observers_.end());
  observers_.erase(browser);
}

}  // namespace

BrowserListSessionServiceImpl::BrowserListSessionServiceImpl(
    BrowserList* browser_list,
    NSString* session_directory,
    SessionServiceIOS* session_service,
    const CreateWebStateCallback& create_web_state)
    : browser_list_(browser_list),
      session_directory_([session_directory copy]),
      session_service_(session_service),
      create_web_state_(create_web_state),
      weak_factory_(this) {
  DCHECK(browser_list_);
  DCHECK(session_directory_);
  DCHECK(session_service_);
  DCHECK(create_web_state_);
  // It is safe to use base::Unretained as the closure is indirectly owned by
  // the BrowserListSessionServiceImpl instance and will be deleted before it.
  observer_ = std::make_unique<BrowserListSessionServiceBrowserListObserver>(
      browser_list,
      base::BindRepeating(&BrowserListSessionServiceImpl::ScheduleSaveSession,
                          base::Unretained(this), false));
}

BrowserListSessionServiceImpl::~BrowserListSessionServiceImpl() = default;

bool BrowserListSessionServiceImpl::RestoreSession() {
  DCHECK(browser_list_) << "RestoreSession called after Shutdown.";
  SessionIOS* session =
      [session_service_ loadSessionFromDirectory:session_directory_];
  if (!session)
    return false;

  DCHECK_LE(session.sessionWindows.count, static_cast<NSUInteger>(INT_MAX));
  for (NSUInteger index = 0; index < session.sessionWindows.count; ++index) {
    Browser* browser =
        static_cast<int>(index) < browser_list_->GetCount()
            ? browser_list_->GetBrowserAtIndex(static_cast<int>(index))
            : browser_list_->CreateNewBrowser();

    SessionWindowIOS* session_window = session.sessionWindows[index];
    if (!session_window.sessions.count)
      continue;

    WebStateList& web_state_list = browser->web_state_list();
    const int old_count = web_state_list.count();
    DeserializeWebStateList(&web_state_list, session_window, create_web_state_);
    DCHECK_GT(web_state_list.count(), old_count);

    // If there was a single tab open without any navigation, then close it
    // after restoring the tabs.
    if (old_count != 1)
      continue;

    web::WebState* web_state = web_state_list.GetWebStateAt(0);
    web::NavigationManager* navigation_manager =
        web_state->GetNavigationManager();

    if (navigation_manager->CanGoBack())
      continue;

    web::NavigationItem* navigation_item =
        navigation_manager->GetLastCommittedItem();
    if (!navigation_item)
      continue;

    if (navigation_item->GetURL() != kChromeUINewTabURL)
      continue;

    web_state_list.CloseWebStateAt(0, WebStateList::CLOSE_USER_ACTION);
  }

  return true;
}

void BrowserListSessionServiceImpl::ScheduleLastSessionDeletion() {
  DCHECK(browser_list_) << "ScheduleLastSessionDeletion called after Shutdown.";
  [session_service_ deleteLastSessionFileInDirectory:session_directory_
                                          completion:base::BindBlockArc(^{
                                                     })];
}

void BrowserListSessionServiceImpl::ScheduleSaveSession(bool immediately) {
  DCHECK(browser_list_) << "ScheduleSaveSession called after Shutdown.";
  DCHECK_GE(browser_list_->GetCount(), 0);

  base::WeakPtr<BrowserListSessionServiceImpl> weak_ptr =
      weak_factory_.GetWeakPtr();
  SessionIOSFactory session_factory = ^SessionIOS*() {
    BrowserListSessionServiceImpl* service = weak_ptr.get();
    if (!weak_ptr)
      return nil;

    const int count = service->browser_list_->GetCount();
    NSMutableArray<SessionWindowIOS*>* windows =
        [NSMutableArray arrayWithCapacity:static_cast<NSUInteger>(count)];
    for (int index = 0; index < count; ++index) {
      Browser* browser = service->browser_list_->GetBrowserAtIndex(index);
      [windows addObject:SerializeWebStateList(&browser->web_state_list())];
    }
    return [[SessionIOS alloc] initWithWindows:windows];
  };

  [session_service_ saveSession:session_factory
                      directory:session_directory_
                    immediately:immediately];
}

void BrowserListSessionServiceImpl::Shutdown() {
  // Stop observing the BrowserList before it is destroyed (BrowserList is a
  // base::SupportsUserData::Data attached to ChromeBrowserState so it will
  // be destroyed after the KeyedService two-stage shutdown).
  browser_list_ = nullptr;
  observer_.reset();
}
