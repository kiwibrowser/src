// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

namespace net {
class URLRequestContextGetter;
}  // namespace net

class GURL;

// UIViewController for the notification about auto sign-in.
@interface NotifyUserAutoSigninViewController : UIViewController

- (instancetype)initWithUsername:(NSString*)username
                         iconURL:(GURL)iconURL
                   contextGetter:(net::URLRequestContextGetter*)contextGetter
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_NOTIFY_AUTO_SIGNIN_VIEW_CONTROLLER_H_
