// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_
#define CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_delegate.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {
class BrowserContext;
}  // namespace content

class BitmapFetcher;
class BitmapFetcherRequest;
class GURL;
class SkBitmap;

// Service to retrieve images for Answers in Suggest.
class BitmapFetcherService : public KeyedService, public BitmapFetcherDelegate {
 public:
  typedef int RequestId;
  static const RequestId REQUEST_ID_INVALID = 0;

  class Observer {
   public:
    virtual ~Observer() {}

    // Called whenever the image changes. Called with an empty image if the
    // fetch failed or the request ended for any reason.
    // TODO(dschuyler) The comment differs from what the code does, follow-up.
    virtual void OnImageChanged(RequestId request_id,
                                const SkBitmap& answers_image) = 0;
  };

  explicit BitmapFetcherService(content::BrowserContext* context);
  ~BitmapFetcherService() override;

  // Cancels a request, if it is still in-flight.
  void CancelRequest(RequestId requestId);

  // Requests a new image. Will either trigger download or satisfy from cache.
  // Takes ownership of |observer|. If there are too many outstanding requests,
  // the request will fail and |observer| will be called to signal failure.
  // Otherwise, |observer| will be called with either the cached image or the
  // downloaded one.
  // NOTE: The observer might be called back synchronously from RequestImage if
  // the image is already in the cache.
  RequestId RequestImage(
      const GURL& url,
      Observer* observer,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

  // Start fetching the image at the given |url|.
  void Prefetch(const GURL& url,
                const net::NetworkTrafficAnnotationTag& traffic_annotation);

 protected:
  // Create a bitmap fetcher for the given |url| and start it. Virtual method
  // so tests can override this for different behavior.
  virtual std::unique_ptr<BitmapFetcher> CreateFetcher(
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

 private:
  friend class BitmapFetcherServiceTest;

  // Gets the existing fetcher for |url| or constructs a new one if it doesn't
  // exist.
  const BitmapFetcher* EnsureFetcherForUrl(
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

  // Find a fetcher with a given |url|. Return NULL if none is found.
  const BitmapFetcher* FindFetcherForUrl(const GURL& url);

  // Remove |fetcher| from list of active fetchers. |fetcher| MUST be part of
  // the list.
  void RemoveFetcher(const BitmapFetcher* fetcher);

  // BitmapFetcherDelegate implementation.
  void OnFetchComplete(const GURL& url, const SkBitmap* bitmap) override;

  // Currently active image fetchers.
  std::vector<std::unique_ptr<BitmapFetcher>> active_fetchers_;

  // Currently active requests.
  std::vector<std::unique_ptr<BitmapFetcherRequest>> requests_;

  // Cache of retrieved images.
  struct CacheEntry {
    CacheEntry();
    ~CacheEntry();

    std::unique_ptr<const SkBitmap> bitmap;
  };
  base::MRUCache<GURL, std::unique_ptr<CacheEntry>> cache_;

  // Current request ID to be used.
  int current_request_id_;

  // Browser context this service is active for.
  content::BrowserContext* context_;

  DISALLOW_COPY_AND_ASSIGN(BitmapFetcherService);
};

#endif  // CHROME_BROWSER_BITMAP_FETCHER_BITMAP_FETCHER_SERVICE_H_
