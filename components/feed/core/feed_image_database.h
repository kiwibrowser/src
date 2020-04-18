// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEED_CORE_FEED_IMAGE_DATABASE_H_
#define COMPONENTS_FEED_CORE_FEED_IMAGE_DATABASE_H_

#include "base/memory/weak_ptr.h"
#include "components/leveldb_proto/proto_database.h"

namespace feed {

class CachedImageProto;

// FeedImageDatabase is leveldb backed store for feed's image data.
// FeedImageDatabase keeps images identified by URLs.
// Save and Load operations are asynchronous, every load operation will update
// last_used_time for the image for garbage collection purpose.
class FeedImageDatabase {
 public:
  enum State {
    UNINITIALIZED,
    INITIALIZED,
    INIT_FAILURE,
  };

  // Returns the resulting raw image data as std::string of a |LoadImage| call.
  using FeedImageDatabaseCallback = base::OnceCallback<void(std::string)>;

  using FeedImageDatabaseOperationCallback = base::OnceCallback<void(bool)>;

  // Initializes the database with |database_dir|.
  explicit FeedImageDatabase(const base::FilePath& database_dir);
  // Initializes the database with |database_dir|. Creates storage using the
  // given |image_database| for local storage. Useful for testing.
  FeedImageDatabase(
      const base::FilePath& database_dir,
      std::unique_ptr<leveldb_proto::ProtoDatabase<CachedImageProto>>
          image_database);
  ~FeedImageDatabase();

  // Returns true if initialization has finished successfully, else false.
  // While this is false, initialization may already started, or initialization
  // failed.
  bool IsInitialized();

  // Adds or updates the image data for the |url|.
  // If the database is not initialized or in some error status, the call will
  // be ignored.
  void SaveImage(const std::string& url, const std::string& image_data);

  // Loads the image data for the |url| and passes it to |callback|.
  // |callback| will be called in the same thread as this function called.
  // If the image cannot be found in database, or database error, returns an
  // empty CachedImageProto. If the database is not initialized yet, the
  // request will be pending until the database has been initialized.
  void LoadImage(const std::string& url, FeedImageDatabaseCallback callback);

  // Deletes the image data for the |url|.
  void DeleteImage(const std::string& url);

  // Delete all images whose |last_used_time| is older than |expired_time| and
  // passes the result to |callback|. |callback| will be called in the same
  // thread as this function called. If database is not initialized, or failed
  // to delete expired entry, false will be passed to |callback|.
  void GarbageCollectImages(base::Time expired_time,
                            FeedImageDatabaseOperationCallback callback);

 private:
  friend class FeedImageDatabaseTest;

  using ImageKeyEntryVector =
      leveldb_proto::ProtoDatabase<CachedImageProto>::KeyEntryVector;

  // Initialization
  void OnDatabaseInitialized(bool success);
  void ProcessPendingImageLoads();

  // Saving
  void SaveImageImpl(const std::string& url, CachedImageProto image_proto);
  void OnImageUpdated(bool success);

  // Loading
  void LoadImageImpl(const std::string& url,
                     FeedImageDatabaseCallback callback);
  void OnImageLoaded(std::string url,
                     FeedImageDatabaseCallback callback,
                     bool success,
                     std::unique_ptr<CachedImageProto> entry);

  // Deleting
  void DeleteImageImpl(const std::string& url,
                       FeedImageDatabaseOperationCallback callback);

  // Garbage collection
  void GarbageCollectImagesImpl(
      base::Time expired_time,
      FeedImageDatabaseOperationCallback callback,
      bool load_entries_success,
      std::unique_ptr<std::vector<CachedImageProto>> image_entries);
  void OnGarbageCollectionDone(FeedImageDatabaseOperationCallback callback,
                               bool success);

  State database_status_;

  std::unique_ptr<leveldb_proto::ProtoDatabase<CachedImageProto>>
      image_database_;
  std::vector<std::pair<std::string, FeedImageDatabaseCallback>>
      pending_image_callbacks_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<FeedImageDatabase> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FeedImageDatabase);
};

}  // namespace feed

#endif  // COMPONENTS_FEED_CORE_FEED_IMAGE_DATABASE_H_
