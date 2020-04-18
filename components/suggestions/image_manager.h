// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_IMAGE_MANAGER_H_
#define COMPONENTS_SUGGESTIONS_IMAGE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/task_runner.h"
#include "base/threading/thread_checker.h"
#include "components/leveldb_proto/proto_database.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace gfx {
class Image;
}

namespace image_fetcher {
class ImageFetcher;
struct RequestMetadata;
}  // namespace image_fetcher

namespace suggestions {

class ImageData;
class SuggestionsProfile;

// A class used to fetch server images asynchronously and manage the caching
// layer (both in memory and on disk).
class ImageManager {
 public:
  typedef std::vector<ImageData> ImageDataVector;
  using ImageCallback =
      base::RepeatingCallback<void(const GURL&, const gfx::Image&)>;

  ImageManager(
      std::unique_ptr<image_fetcher::ImageFetcher> image_fetcher,
      std::unique_ptr<leveldb_proto::ProtoDatabase<ImageData>> database,
      const base::FilePath& database_dir);
  virtual ~ImageManager();

  virtual void Initialize(const SuggestionsProfile& suggestions);

  // Should be called from the UI thread.
  virtual void AddImageURL(const GURL& url, const GURL& image_url);

  // Should be called from the UI thread.
  virtual void GetImageForURL(const GURL& url, ImageCallback callback);

 private:
  friend class MockImageManager;
  friend class ImageManagerTest;
  FRIEND_TEST_ALL_PREFIXES(ImageManagerTest, InitializeTest);
  FRIEND_TEST_ALL_PREFIXES(ImageManagerTest, AddImageURL);
  FRIEND_TEST_ALL_PREFIXES(ImageManagerTest, GetImageForURLNetworkCacheHit);
  FRIEND_TEST_ALL_PREFIXES(ImageManagerTest,
                           GetImageForURLNetworkCacheNotInitialized);
  FRIEND_TEST_ALL_PREFIXES(ImageManagerTest, QueueImageRequest);

  // Used for testing.
  ImageManager();

  typedef std::vector<ImageCallback> CallbackVector;
  typedef base::hash_map<std::string, scoped_refptr<base::RefCountedMemory>>
      ImageMap;

  // State related to an image fetch (associated website url, image_url,
  // pending callbacks).
  struct ImageCacheRequest {
    ImageCacheRequest();
    ImageCacheRequest(const ImageCacheRequest& other);
    ~ImageCacheRequest();

    GURL url;
    GURL image_url;
    // Queue for pending callbacks, which may accumulate while the request is in
    // flight.
    CallbackVector callbacks;
  };

  typedef std::map<const GURL, ImageCacheRequest> ImageCacheRequestMap;

  // Looks up image URL for |url|. If found, writes the result to |image_url|
  // and returns true. Otherwise just returns false.
  bool GetImageURL(const GURL& url, GURL* image_url);

  void QueueCacheRequest(const GURL& url,
                         const GURL& image_url,
                         ImageCallback callback);

  void ServeFromCacheOrNetwork(const GURL& url,
                               const GURL& image_url,
                               ImageCallback callback);

  void OnCacheImageDecoded(const GURL& url,
                           const GURL& image_url,
                           const ImageCallback& callback,
                           std::unique_ptr<SkBitmap> bitmap);

  // Returns null if the |url| had no entry in the cache.
  scoped_refptr<base::RefCountedMemory> GetEncodedImageFromCache(
      const GURL& url);

  // Save the image bitmap in the cache and in the database.
  void SaveImage(const std::string& url, const SkBitmap& bitmap);

  // Database callback methods.
  // Will initiate loading the entries.
  void OnDatabaseInit(bool success);
  // Will transfer the loaded |entries| in memory (|image_map_|).
  void OnDatabaseLoad(bool success, std::unique_ptr<ImageDataVector> entries);
  void OnDatabaseSave(bool success);

  // Take entries from the database and put them in the local cache.
  void LoadEntriesInCache(std::unique_ptr<ImageDataVector> entries);

  void ServePendingCacheRequests();

  void SaveImageAndForward(const ImageCallback& image_callback,
                           const std::string& url,
                           const gfx::Image& image,
                           const image_fetcher::RequestMetadata& metadata);

  // Map from URL to image URL. Should be kept up to date when a new
  // SuggestionsProfile is available.
  std::map<GURL, GURL> image_url_map_;

  // Map from website URL to request information, used for pending cache
  // requests while the database hasn't loaded.
  ImageCacheRequestMap pending_cache_requests_;

  // Holding the bitmaps in memory, keyed by website URL string.
  ImageMap image_map_;

  std::unique_ptr<image_fetcher::ImageFetcher> image_fetcher_;

  std::unique_ptr<leveldb_proto::ProtoDatabase<ImageData>> database_;

  scoped_refptr<base::TaskRunner> background_task_runner_;

  bool database_ready_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<ImageManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageManager);
};

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_IMAGE_MANAGER_H_
