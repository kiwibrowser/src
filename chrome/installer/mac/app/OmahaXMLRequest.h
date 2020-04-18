// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_OMAHAXMLREQUEST_H_
#define CHROME_INSTALLER_MAC_APP_OMAHAXMLREQUEST_H_

#import <Foundation/Foundation.h>

@interface OmahaXMLRequest : NSObject

// Creates the body of the request being prepared to send to Omaha.
+ (NSXMLDocument*)createXMLRequestBody;

@end

#endif  // CHROME_INSTALLER_MAC_APP_OMAHAXMLREQUEST_H_
