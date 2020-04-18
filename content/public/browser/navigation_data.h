// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_NAVIGATION_DATA_H_
#define CONTENT_PUBLIC_BROWSER_NAVIGATION_DATA_H_

#include <memory>

namespace content {

// Copyable interface for embedders to pass opaque data to content/. It is
// expected to be created on the IO thread, and content/ will transfer it to the
// UI thread as a clone.
class NavigationData {
 public:
  virtual ~NavigationData(){};

  // Creates a new NavigationData that is a deep copy of the original
  virtual std::unique_ptr<NavigationData> Clone() const = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NAVIGATION_DATA_H_
