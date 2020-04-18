// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_

#include <string>

#include "url/gurl.h"

namespace offline_pages {

// Objects implementing this interface represent individual renovations
// that can be run in a page pre-snapshot.
class PageRenovation {
 public:
  virtual ~PageRenovation() {}

  // Returns |true| if this renovation should run in the page from |url|.
  virtual bool ShouldRun(const GURL& url) const = 0;
  // Returns an ID that identifies this renovation's script. This ID
  // can be passed to the PageRenovationLoader.
  virtual std::string GetID() const = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_
