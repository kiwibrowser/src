// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"

@protocol WebStatePrinter;
class GURL;

namespace base {
class DictionaryValue;
}  // namespace base

// Handles print requests from JavaScript window.print.
class PrintTabHelper : public web::WebStateObserver,
                       public web::WebStateUserData<PrintTabHelper> {
 public:
  ~PrintTabHelper() override;

  // Creates a PrintTabHelper and attaches it to |web_state|. The |printer|
  // must be non-nil.
  static void CreateForWebState(web::WebState* web_state,
                                id<WebStatePrinter> printer);

 private:
  PrintTabHelper(web::WebState* web_state, id<WebStatePrinter> printer);

  // web::WebStateObserver overrides:
  void WebStateDestroyed(web::WebState* web_state) override;

  // Called when print message is sent by the web page.
  bool OnPrintCommand(web::WebState* web_state,
                      const base::DictionaryValue& command,
                      const GURL& page_url,
                      bool user_initiated);

  __weak id<WebStatePrinter> printer_;

  DISALLOW_COPY_AND_ASSIGN(PrintTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_PRINT_TAB_HELPER_H_
