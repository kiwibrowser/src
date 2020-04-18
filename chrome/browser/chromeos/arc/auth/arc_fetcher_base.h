// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_FETCHER_BASE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_FETCHER_BASE_H_

namespace arc {

// Base class for Arc*Fetcher classes, only used to manage the lifetime of their
// instances.
class ArcFetcherBase {
 public:
  virtual ~ArcFetcherBase() = default;
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_FETCHER_BASE_H_
