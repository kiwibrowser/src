// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_MOST_VISITED_IFRAME_SOURCE_H_
#define CHROME_BROWSER_SEARCH_MOST_VISITED_IFRAME_SOURCE_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/search/iframe_source.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

// Serves HTML for displaying suggestions using iframes, e.g.
// chrome-search://most-visited/single.html
class MostVisitedIframeSource : public IframeSource {
 public:
  MostVisitedIframeSource();
  ~MostVisitedIframeSource() override;

  // Overridden from IframeSource. Public for testing.
  void StartDataRequest(
      const std::string& path_and_query,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;

 private:
  // Overridden from IframeSource:
  std::string GetSource() const override;

  bool ServesPath(const std::string& path) const override;

  DISALLOW_COPY_AND_ASSIGN(MostVisitedIframeSource);
};

#endif  // CHROME_BROWSER_SEARCH_MOST_VISITED_IFRAME_SOURCE_H_
