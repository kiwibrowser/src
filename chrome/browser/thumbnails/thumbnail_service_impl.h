// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_IMPL_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/thumbnails/thumbnail_service.h"
#include "components/history/core/browser/top_sites.h"
#include "ui/base/page_transition_types.h"

namespace base {
class RefCountedMemory;
}

namespace thumbnails {

// An implementation of ThumbnailService which delegates storage and most of
// logic to an underlying TopSites instances.
class ThumbnailServiceImpl : public ThumbnailService {
 public:
  explicit ThumbnailServiceImpl(Profile* profile);

  // Implementation of ThumbnailService.
  bool SetPageThumbnail(const ThumbnailingContext& context,
                        const gfx::Image& thumbnail) override;
  bool GetPageThumbnail(const GURL& url,
                        bool prefix_match,
                        scoped_refptr<base::RefCountedMemory>* bytes) override;
  void AddForcedURL(const GURL& url) override;
  bool ShouldAcquirePageThumbnail(const GURL& url,
                                  ui::PageTransition transition) override;

  // Implementation of RefcountedKeyedService.
  void ShutdownOnUIThread() override;

 private:
  ~ThumbnailServiceImpl() override;

  scoped_refptr<history::TopSites> top_sites_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailServiceImpl);
};

}  // namespace thumbnails

#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_IMPL_H_
