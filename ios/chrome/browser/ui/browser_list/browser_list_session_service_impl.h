// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_IMPL_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_IMPL_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_session_service.h"
#import "ios/web/public/web_state/web_state.h"

class BrowserList;
class BrowserListObserver;
@class CRWSessionStorage;
@class SessionServiceIOS;

namespace web {
class WebState;
}

// Implementation of BrowserListSessionService that automatically track the
// active WebStates of |browser_list| and save the session when the active
// WebState changes or a new navigation item is committed.
class BrowserListSessionServiceImpl : public BrowserListSessionService {
 public:
  // Callback used to create WebStates when restoring a session.
  using CreateWebStateCallback =
      base::RepeatingCallback<std::unique_ptr<web::WebState>(
          CRWSessionStorage*)>;

  BrowserListSessionServiceImpl(BrowserList* browser_list,
                                NSString* session_directory,
                                SessionServiceIOS* session_service,
                                const CreateWebStateCallback& create_web_state);
  ~BrowserListSessionServiceImpl() override;

  // BrowserListSessionService implementation.
  bool RestoreSession() override;
  void ScheduleLastSessionDeletion() override;
  void ScheduleSaveSession(bool immediately) override;

  // KeyedService implementation.
  void Shutdown() override;

 private:
  BrowserList* browser_list_;
  __strong NSString* session_directory_;
  __strong SessionServiceIOS* session_service_;
  CreateWebStateCallback create_web_state_;
  std::unique_ptr<BrowserListObserver> observer_;

  // Used to ensure that the block passed to SessionServiceIOS does not access
  // this object once it has been destroyed.
  base::WeakPtrFactory<BrowserListSessionServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListSessionServiceImpl);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_IMPL_H_
