// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_LIST_SOURCE_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_LIST_SOURCE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "components/history/core/browser/history_types.h"
#include "content/public/browser/url_data_source.h"

class Profile;

namespace history {
class TopSites;
}

namespace thumbnails {
class ThumbnailService;
}

// ThumbnailListSource renders a webpage to list all thumbnails stored in
// TopSites, along with all associated URLs. The thumbnail images are embedded
// into the HTML via Base64, so we can easily produce a dump of TopSites and
// the thumbnails it stores.
class ThumbnailListSource : public content::URLDataSource {
 public:
  explicit ThumbnailListSource(Profile* profile);

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  // Called on the IO thread.
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;

  std::string GetMimeType(const std::string& path) const override;
  scoped_refptr<base::SingleThreadTaskRunner> TaskRunnerForRequestPath(
      const std::string& path) const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;
  bool ShouldReplaceExistingSource() const override;

 private:
  ~ThumbnailListSource() override;

  // Called on the IO thread.
  void OnMostVisitedURLsAvailable(
    const content::URLDataSource::GotDataCallback& callback,
    const history::MostVisitedURLList& mvurl_list);

  // ThumbnailService.
  scoped_refptr<thumbnails::ThumbnailService> thumbnail_service_;

  scoped_refptr<history::TopSites> top_sites_;

  // For callbacks may be run after destruction.
  base::WeakPtrFactory<ThumbnailListSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailListSource);
};


#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_LIST_SOURCE_H_
