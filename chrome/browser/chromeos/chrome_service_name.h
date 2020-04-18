// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CHROME_SERVICE_NAME_H_
#define CHROME_BROWSER_CHROMEOS_CHROME_SERVICE_NAME_H_

namespace chromeos {

// This is the service name used for services exposed by Chrome. Interfaces
// exported (and potentially used inside Chrome) are registered under this name.
extern const char kChromeServiceName[];

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CHROME_SERVICE_NAME_H_
