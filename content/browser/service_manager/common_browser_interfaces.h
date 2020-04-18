// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_MANAGER_COMMON_BROWSER_INTERFACES_H_
#define CONTENT_BROWSER_SERVICE_MANAGER_COMMON_BROWSER_INTERFACES_H_

namespace content {

class ServiceManagerConnection;

// Registers interface binders for browser-side interfaces that are common to
// all child process types.
void RegisterCommonBrowserInterfaces(ServiceManagerConnection* connection);

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_MANAGER_COMMON_BROWSER_INTERFACES_H_
