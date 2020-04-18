// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_USER_DATA_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_USER_DATA_H_

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"

// A base class for classes attached to, and scoped to, the lifetime of a
// Browser. For example:
//
// --- in foo.h ---
// class Foo : public BrowserUserData<Foo> {
//  public:
//   ~Foo() override;
//   // ... more public stuff here ...
//  private:
//   explicit Foo(Browser* browser);
//   friend class BrowserUserData<Foo>;
//   // ... more private stuff here ...
// }
// --- in foo.mm ---
// DEFINE_BROWSER_USER_DATA_KEY(Foo);
//
template <typename T>
class BrowserUserData : public base::SupportsUserData::Data {
 public:
  // Creates an object of type T, and attaches it to the specified Browser.
  // If an instance is already attached, does nothing.
  static void CreateForBrowser(Browser* browser) {
    DCHECK(browser);
    if (!FromBrowser(browser)) {
      browser->SetUserData(UserDataKey(), base::WrapUnique(new T(browser)));
    }
  }

  // Retrieves the instance of type T that was attached to the specified
  // Browser (via CreateForBrowser above) and returns it. If no
  // instance of the type was attached, returns nullptr.
  static T* FromBrowser(Browser* browser) {
    return static_cast<T*>(browser->GetUserData(UserDataKey()));
  }
  static const T* FromBrowser(const Browser* browser) {
    return static_cast<const T*>(browser->GetUserData(UserDataKey()));
  }
  // Removes the instance attached to the specified WebState.
  static void RemoveFromBrowser(Browser* browser) {
    browser->RemoveUserData(UserDataKey());
  }

 protected:
  static inline void* UserDataKey() { return &kLocatorKey; }

 private:
  // The user data key.
  static int kLocatorKey;
};

// The macro to define the locator key. This key should be defined in the
// implementation file of the derived class.
//
// The "= 0" is surprising, but is required to effect a definition rather than
// a declaration. Without it, this would be merely a declaration of a template
// specialization. (C++98: 14.7.3.15; C++11: 14.7.3.13)
//
#define DEFINE_BROWSER_USER_DATA_KEY(TYPE) \
  template <>                              \
  int BrowserUserData<TYPE>::kLocatorKey = 0

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_USER_DATA_H_
