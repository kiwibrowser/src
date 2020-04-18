// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_RENOVATIONS_PAGE_RENOVATION_LOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_RENOVATIONS_PAGE_RENOVATION_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/offline_pages/core/renovations/page_renovation.h"

namespace offline_pages {

// Class for preparing page renovation scripts. Handles loading
// JavaScript from storage and creating script to run particular
// renovations.
class PageRenovationLoader {
 public:
  PageRenovationLoader();
  ~PageRenovationLoader();

  // Takes a list of renovation IDs and outputs a script to be run in
  // page. Returns whether loading was successful. The script is a
  // string16 to match the rest of Chrome.
  bool GetRenovationScript(const std::vector<std::string>& renovation_ids,
                           base::string16* script);

  // Returns the list of known renovations.
  const std::vector<std::unique_ptr<PageRenovation>>& renovations() {
    return renovations_;
  }

  // Methods for testing.
  void SetSourceForTest(base::string16 combined_source);
  void SetRenovationsForTest(
      std::vector<std::unique_ptr<PageRenovation>> renovations);

 private:
  // Called to load JavaScript source from storage.
  bool LoadSource();

  // List of registered page renovations
  std::vector<std::unique_ptr<PageRenovation>> renovations_;

  // Whether JavaScript source has been loaded.
  bool is_loaded_;
  // Contains JavaScript source.
  base::string16 combined_source_;

  DISALLOW_COPY_AND_ASSIGN(PageRenovationLoader);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_RENOVATIONS_PAGE_RENOVATION_LOADER_H_
