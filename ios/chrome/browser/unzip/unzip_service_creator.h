// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UNZIP_UNZIP_SERVICE_CREATOR_H_
#define IOS_CHROME_BROWSER_UNZIP_UNZIP_SERVICE_CREATOR_H_

#include "ios/web/public/browser_state.h"

// Registers an Unzip service factory function in |services|.
void RegisterUnzipService(web::BrowserState::StaticServiceMap* services);

#endif  // IOS_CHROME_BROWSER_UNZIP_UNZIP_SERVICE_CREATOR_H_
