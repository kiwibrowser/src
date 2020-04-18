// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SSL_IOS_SECURITY_STATE_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SSL_IOS_SECURITY_STATE_TAB_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_user_data.h"

namespace security_state {
struct SecurityInfo;
struct VisibleSecurityState;
}  // namespace security_state

namespace web {
class WebState;
}  // namespace web

// Tab helper that provides the page's security status. Uses a WebState to
// provide a security_state::GetSecurityInfo() with the relevant
// VisibleSecurityState information.
class IOSSecurityStateTabHelper
    : public web::WebStateUserData<IOSSecurityStateTabHelper> {
 public:
  ~IOSSecurityStateTabHelper() override;

  void GetSecurityInfo(security_state::SecurityInfo* result) const;

 private:
  explicit IOSSecurityStateTabHelper(web::WebState* web_state);
  friend class web::WebStateUserData<IOSSecurityStateTabHelper>;

  std::unique_ptr<security_state::VisibleSecurityState>
  GetVisibleSecurityState() const;

  web::WebState* web_state_;

  DISALLOW_COPY_AND_ASSIGN(IOSSecurityStateTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SSL_IOS_SECURITY_STATE_TAB_HELPER_H_
