// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_IFRAME_SOURCE_H_
#define CHROME_BROWSER_SEARCH_IFRAME_SOURCE_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/browser/url_data_source.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

// Base class for URL data sources for chrome-search:// iframed content.
// TODO(treib): This has only one subclass outside of tests,
// MostVisitedIframeSource. Merge the two classes?
class IframeSource : public content::URLDataSource {
 public:
  IframeSource();
  ~IframeSource() override;

 protected:
  // Overridden from content::URLDataSource:
  std::string GetMimeType(const std::string& path_and_query) const override;
  bool AllowCaching() const override;
  bool ShouldDenyXFrameOptions() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;

  // Returns whether this source should serve data for a particular path.
  virtual bool ServesPath(const std::string& path) const = 0;

  // Sends unmodified resource bytes.
  void SendResource(
      int resource_id,
      const content::URLDataSource::GotDataCallback& callback);

  // Sends Javascript with an expected postMessage origin interpolated.
  void SendJSWithOrigin(
      int resource_id,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback);

  // This is exposed for testing and should not be overridden.
  // Sets |origin| to the URL of the WebContents identified by |wc_getter|.
  // Returns true if successful and false if not, for example if the WebContents
  // does not exist
  virtual bool GetOrigin(
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      std::string* origin) const;

  DISALLOW_COPY_AND_ASSIGN(IframeSource);
};

#endif  // CHROME_BROWSER_SEARCH_IFRAME_SOURCE_H_
