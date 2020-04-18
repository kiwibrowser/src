// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/feed_image_database.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/feed/core/proto/cached_image.pb.h"
#include "components/feed/core/time_serialization.h"
#include "components/leveldb_proto/proto_database_impl.h"

namespace feed {

namespace {
// Statistics are logged to UMA with this string as part of histogram name. They
// can all be found under LevelDB.*.FeedImageDatabase. Changing this needs to
// synchronize with histograms.xml, AND will also become incompatible with older
// browsers still reporting the previous values.
const char kImageDatabaseUMAClientName[] = "FeedImageDatabase";

const char kImageDatabaseFolder[] = "images";

const size_t kDatabaseWriteBufferSizeBytes = 512 * 1024;
const size_t kDatabaseWriteBufferSizeBytesForLowEndDevice = 128 * 1024;
}  // namespace

FeedImageDatabase::FeedImageDatabase(const base::FilePath& database_dir)
    : FeedImageDatabase(
          database_dir,
          std::make_unique<leveldb_proto::ProtoDatabaseImpl<CachedImageProto>>(
              base::CreateSequencedTaskRunnerWithTraits(
                  {base::MayBlock(), base::TaskPriority::BACKGROUND,
                   base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}))) {}

FeedImageDatabase::FeedImageDatabase(
    const base::FilePath& database_dir,
    std::unique_ptr<leveldb_proto::ProtoDatabase<CachedImageProto>>
        image_database)
    : database_status_(UNINITIALIZED),
      image_database_(std::move(image_database)),
      weak_ptr_factory_(this) {
  leveldb_env::Options options = leveldb_proto::CreateSimpleOptions();
  if (base::SysInfo::IsLowEndDevice()) {
    options.write_buffer_size = kDatabaseWriteBufferSizeBytesForLowEndDevice;
  } else {
    options.write_buffer_size = kDatabaseWriteBufferSizeBytes;
  }
  base::FilePath image_dir = database_dir.AppendASCII(kImageDatabaseFolder);
  image_database_->Init(
      kImageDatabaseUMAClientName, image_dir, options,
      base::BindOnce(&FeedImageDatabase::OnDatabaseInitialized,
                     weak_ptr_factory_.GetWeakPtr()));
}

FeedImageDatabase::~FeedImageDatabase() = default;

bool FeedImageDatabase::IsInitialized() {
  return INITIALIZED == database_status_;
}

void FeedImageDatabase::SaveImage(const std::string& url,
                                  const std::string& image_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If database is not ready, ignore the request.
  if (!IsInitialized())
    return;

  CachedImageProto image_proto;
  image_proto.set_url(url);
  image_proto.set_data(image_data);
  image_proto.set_last_used_time(ToDatabaseTime(base::Time::Now()));

  SaveImageImpl(url, std::move(image_proto));
}

void FeedImageDatabase::LoadImage(const std::string& url,
                                  FeedImageDatabaseCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (database_status_) {
    case INITIALIZED:
    case INIT_FAILURE:
      LoadImageImpl(url, std::move(callback));
      break;
    case UNINITIALIZED:
      pending_image_callbacks_.emplace_back(url, std::move(callback));
      break;
    default:
      NOTREACHED();
  }
}

void FeedImageDatabase::DeleteImage(const std::string& url) {
  DeleteImageImpl(url, base::BindOnce(&FeedImageDatabase::OnImageUpdated,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void FeedImageDatabase::GarbageCollectImages(
    base::Time expired_time,
    FeedImageDatabaseOperationCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If database is not initialized yet, ignore the request.
  if (!IsInitialized()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }

  image_database_->LoadEntries(base::BindOnce(
      &FeedImageDatabase::GarbageCollectImagesImpl,
      weak_ptr_factory_.GetWeakPtr(), expired_time, std::move(callback)));
}

void FeedImageDatabase::OnDatabaseInitialized(bool success) {
  DCHECK_EQ(database_status_, UNINITIALIZED);

  if (success) {
    database_status_ = INITIALIZED;
  } else {
    database_status_ = INIT_FAILURE;
    DVLOG(1) << "FeedImageDatabase init failed.";
  }

  ProcessPendingImageLoads();
}

void FeedImageDatabase::ProcessPendingImageLoads() {
  DCHECK_NE(database_status_, UNINITIALIZED);

  for (auto& image_callback : pending_image_callbacks_)
    LoadImageImpl(image_callback.first, std::move(image_callback.second));

  pending_image_callbacks_.clear();
}

void FeedImageDatabase::SaveImageImpl(const std::string& url,
                                      CachedImageProto image_proto) {
  auto entries_to_save = std::make_unique<ImageKeyEntryVector>();
  entries_to_save->emplace_back(url, std::move(image_proto));

  image_database_->UpdateEntries(
      std::move(entries_to_save), std::make_unique<std::vector<std::string>>(),
      base::BindOnce(&FeedImageDatabase::OnImageUpdated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FeedImageDatabase::OnImageLoaded(std::string url,
                                      FeedImageDatabaseCallback callback,
                                      bool success,
                                      std::unique_ptr<CachedImageProto> entry) {
  if (!success || !entry) {
    DVLOG_IF(1, !success) << "FeedImageDatabase load failed.";
    std::move(callback).Run(std::string());
    return;
  }

  DCHECK_EQ(url, entry->url());
  std::move(callback).Run(entry->data());

  // Update timestamp for image.
  entry->set_last_used_time(ToDatabaseTime(base::Time::Now()));
  SaveImageImpl(url, std::move(*entry));
}

void FeedImageDatabase::LoadImageImpl(const std::string& url,
                                      FeedImageDatabaseCallback callback) {
  DCHECK_NE(database_status_, UNINITIALIZED);

  if (IsInitialized()) {
    image_database_->GetEntry(
        url, base::BindOnce(&FeedImageDatabase::OnImageLoaded,
                            weak_ptr_factory_.GetWeakPtr(), url,
                            std::move(callback)));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::string()));
  }
}

void FeedImageDatabase::OnImageUpdated(bool success) {
  DVLOG_IF(1, !success) << "FeedImageDatabase update failed.";
}

void FeedImageDatabase::DeleteImageImpl(
    const std::string& url,
    FeedImageDatabaseOperationCallback callback) {
  image_database_->UpdateEntries(
      std::make_unique<ImageKeyEntryVector>(),
      std::make_unique<std::vector<std::string>>(1, url), std::move(callback));
}

void FeedImageDatabase::GarbageCollectImagesImpl(
    base::Time expired_time,
    FeedImageDatabaseOperationCallback callback,
    bool load_entries_success,
    std::unique_ptr<std::vector<CachedImageProto>> image_entries) {
  if (!load_entries_success) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FeedImageDatabase::OnGarbageCollectionDone,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  std::move(callback), false));
    return;
  }

  int64_t expired_database_time = ToDatabaseTime(expired_time);
  auto keys_to_remove = std::make_unique<std::vector<std::string>>();
  for (const CachedImageProto& image : *image_entries) {
    if (image.last_used_time() < expired_database_time) {
      keys_to_remove->emplace_back(image.url());
    }
  }

  if (keys_to_remove->empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), true));
    return;
  }

  image_database_->UpdateEntries(
      std::make_unique<ImageKeyEntryVector>(), std::move(keys_to_remove),
      base::BindOnce(&FeedImageDatabase::OnGarbageCollectionDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FeedImageDatabase::OnGarbageCollectionDone(
    FeedImageDatabaseOperationCallback callback,
    bool success) {
  DVLOG_IF(1, !success) << "FeedImageDatabase garbage collection failed.";
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), success));
}

}  // namespace feed
