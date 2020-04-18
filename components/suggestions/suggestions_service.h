// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_SUGGESTIONS_SERVICE_H_
#define COMPONENTS_SUGGESTIONS_SUGGESTIONS_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "url/gurl.h"

namespace gfx {
class Image;
}  // namespace gfx

namespace suggestions {

// An interface to fetch server suggestions asynchronously.
class SuggestionsService : public KeyedService {
 public:
  using ResponseCallback = base::Callback<void(const SuggestionsProfile&)>;
  using BitmapCallback = base::Callback<void(const GURL&, const gfx::Image&)>;

  using ResponseCallbackList =
      base::CallbackList<void(const SuggestionsProfile&)>;

  // Initiates a network request for suggestions if sync state allows and there
  // is no pending request. Returns true iff sync state allowed for a request,
  // whether a new request was actually sent or not.
  virtual bool FetchSuggestionsData() = 0;

  // Returns the current set of suggestions from the cache.
  virtual base::Optional<SuggestionsProfile> GetSuggestionsDataFromCache()
      const = 0;

  // Adds a callback that is called when the suggestions are updated.
  virtual std::unique_ptr<ResponseCallbackList::Subscription> AddCallback(
      const ResponseCallback& callback) WARN_UNUSED_RESULT = 0;

  // Retrieves stored thumbnail for website |url| asynchronously. Calls
  // |callback| with Bitmap pointer if found, and NULL otherwise.
  virtual void GetPageThumbnail(const GURL& url,
                                const BitmapCallback& callback) = 0;

  // A version of |GetPageThumbnail| that explicitly supplies the download URL
  // for the thumbnail. Replaces any pre-existing thumbnail URL with the
  // supplied one.
  virtual void GetPageThumbnailWithURL(const GURL& url,
                                       const GURL& thumbnail_url,
                                       const BitmapCallback& callback) = 0;

  // Adds a URL to the blacklist cache, returning true on success or false on
  // failure. The URL will eventually be uploaded to the server.
  virtual bool BlacklistURL(const GURL& candidate_url) = 0;

  // Removes a URL from the local blacklist, returning true on success or false
  // on failure.
  virtual bool UndoBlacklistURL(const GURL& url) = 0;

  // Removes all URLs from the blacklist.
  virtual void ClearBlacklist() = 0;
};

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_SUGGESTIONS_SERVICE_H_
