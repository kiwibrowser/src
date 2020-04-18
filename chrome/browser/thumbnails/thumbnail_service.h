// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_H_

#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/image/image.h"

class GURL;

namespace base {
class RefCountedMemory;
}

namespace thumbnails {

struct ThumbnailingContext;

// An interface abstracting access to thumbnails. Intended as a temporary
// bridge facilitating switch from TopSites as the thumbnail source to a more
// robust way of handling these artefacts.
class ThumbnailService : public RefcountedKeyedService {
 public:
  // Sets the given thumbnail for the given URL. Returns true if the thumbnail
  // was updated. False means either the URL wasn't known to us, or we felt
  // that our current thumbnail was superior to the given one.
  virtual bool SetPageThumbnail(const ThumbnailingContext& context,
                                const gfx::Image& thumbnail) = 0;

  // Gets a thumbnail for a given page. Returns true iff we have the thumbnail.
  // This may be invoked on any thread.
  // If an exact thumbnail URL match fails, |prefix_match| specifies whether or
  // not to try harder by matching the query thumbnail URL as URL prefix (as
  // defined by UrlIsPrefix()).
  // As this method may be invoked on any thread the ref count needs to be
  // incremented before this method returns, so this takes a scoped_refptr*.
  virtual bool GetPageThumbnail(
      const GURL& url,
      bool prefix_match,
      scoped_refptr<base::RefCountedMemory>* bytes) = 0;

  // Add or updates a |url| for which we should force the capture of a thumbnail
  // next time it's visited.
  virtual void AddForcedURL(const GURL& url) = 0;

  // Returns true if the page thumbnail should be updated.
  virtual bool ShouldAcquirePageThumbnail(const GURL& url,
                                          ui::PageTransition transition) = 0;

 protected:
  ~ThumbnailService() override {}
};

}  // namespace thumbnails

#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_SERVICE_H_
