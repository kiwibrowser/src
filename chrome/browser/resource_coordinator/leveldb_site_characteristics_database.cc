// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/leveldb_site_characteristics_database.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_restrictions.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace resource_coordinator {

// Helper class used to run all the blocking operations posted by
// LocalSiteCharacteristicDatabase on a TaskScheduler sequence with the
// |MayBlock()| trait.
//
// Instances of this class should only be destructed once all the posted tasks
// have been run, in practice it means that they should ideally be stored in a
// std::unique_ptr<AsyncHelper, base::OnTaskRunnerDeleter>.
class LevelDBSiteCharacteristicsDatabase::AsyncHelper {
 public:
  explicit AsyncHelper(const base::FilePath& db_path) : db_path_(db_path) {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    // Setting |sync| to false might cause some data loss if the system crashes
    // but it'll make the write operations faster (no data will be lost if only
    // the process crashes).
    write_options_.sync = false;
  }
  ~AsyncHelper() = default;

  // Open the database from |db_path_| after creating it if it didn't exist.
  void OpenOrCreateDatabase();

  // Implementations of the DB manipulation functions of
  // LevelDBSiteCharacteristicsDatabase that run on a blocking sequence.
  base::Optional<SiteCharacteristicsProto> ReadSiteCharacteristicsFromDB(
      const std::string& site_origin);
  void WriteSiteCharacteristicsIntoDB(
      const std::string& site_origin,
      const SiteCharacteristicsProto& site_characteristic_proto);
  void RemoveSiteCharacteristicsFromDB(
      const std::vector<std::string>& site_origin);
  void ClearDatabase();

 private:
  // The on disk location of the database.
  const base::FilePath db_path_;
  // The connection to the LevelDB database.
  std::unique_ptr<leveldb::DB> db_;
  // The options to be used for all database read operations.
  leveldb::ReadOptions read_options_;
  // The options to be used for all database write operations.
  leveldb::WriteOptions write_options_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(AsyncHelper);
};

void LevelDBSiteCharacteristicsDatabase::AsyncHelper::OpenOrCreateDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertBlockingAllowed();

  leveldb_env::Options options;
  options.create_if_missing = true;
  leveldb::Status status =
      leveldb_env::OpenDB(options, db_path_.AsUTF8Unsafe(), &db_);
  // TODO(sebmarchand): Do more validation here, try to repair the database if
  // it's corrupt and report some metrics.
  if (!status.ok()) {
    LOG(ERROR) << "Unable to open the Site Characteristics database: "
               << status.ToString();
    db_.reset();
  }
}

base::Optional<SiteCharacteristicsProto>
LevelDBSiteCharacteristicsDatabase::AsyncHelper::ReadSiteCharacteristicsFromDB(
    const std::string& site_origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertBlockingAllowed();

  std::string protobuf_value;
  leveldb::Status s = db_->Get(read_options_, site_origin, &protobuf_value);
  base::Optional<SiteCharacteristicsProto> site_characteristic_proto;
  if (s.ok()) {
    site_characteristic_proto = SiteCharacteristicsProto();
    if (!site_characteristic_proto->ParseFromString(protobuf_value)) {
      site_characteristic_proto = base::nullopt;
      LOG(ERROR) << "Error while trying to parse a SiteCharacteristicsProto "
                 << "protobuf.";
    }
  }
  return site_characteristic_proto;
}

void LevelDBSiteCharacteristicsDatabase::AsyncHelper::
    WriteSiteCharacteristicsIntoDB(
        const std::string& site_origin,
        const SiteCharacteristicsProto& site_characteristic_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertBlockingAllowed();
  leveldb::Status s = db_->Put(write_options_, site_origin,
                               site_characteristic_proto.SerializeAsString());
  if (!s.ok()) {
    LOG(ERROR) << "Error while inserting an element in the site characteristic "
               << "database: " << s.ToString();
  }
}

void LevelDBSiteCharacteristicsDatabase::AsyncHelper::
    RemoveSiteCharacteristicsFromDB(
        const std::vector<std::string>& site_origins) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertBlockingAllowed();
  leveldb::WriteBatch batch;
  for (const auto iter : site_origins)
    batch.Delete(iter);
  leveldb::Status status = db_->Write(write_options_, &batch);
  if (!status.ok()) {
    LOG(WARNING) << "Failed to remove some entries from the site "
                 << "characteristics database: " << status.ToString();
  }
}

void LevelDBSiteCharacteristicsDatabase::AsyncHelper::ClearDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertBlockingAllowed();
  leveldb_env::Options options;
  db_.reset();
  leveldb::Status status = leveldb::DestroyDB(db_path_.AsUTF8Unsafe(), options);
  if (status.ok()) {
    OpenOrCreateDatabase();
  } else {
    LOG(WARNING) << "Failed to destroy the site characteristics database: "
                 << status.ToString();
  }
}

LevelDBSiteCharacteristicsDatabase::LevelDBSiteCharacteristicsDatabase(
    const base::FilePath& db_path)
    : blocking_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          // The |BLOCK_SHUTDOWN| trait is required to ensure that a clearing of
          // the database won't be skipped.
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      async_helper_(new AsyncHelper(db_path),
                    base::OnTaskRunnerDeleter(blocking_task_runner_)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  blocking_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&LevelDBSiteCharacteristicsDatabase::
                                    AsyncHelper::OpenOrCreateDatabase,
                                base::Unretained(async_helper_.get())));
}

LevelDBSiteCharacteristicsDatabase::~LevelDBSiteCharacteristicsDatabase() =
    default;

void LevelDBSiteCharacteristicsDatabase::ReadSiteCharacteristicsFromDB(
    const std::string& site_origin,
    LocalSiteCharacteristicsDatabase::ReadSiteCharacteristicsFromDBCallback
        callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Trigger the asynchronous task and make it run the callback on this thread
  // once it returns.
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LevelDBSiteCharacteristicsDatabase::AsyncHelper::
                         ReadSiteCharacteristicsFromDB,
                     base::Unretained(async_helper_.get()), site_origin),
      base::BindOnce(std::move(callback)));
}

void LevelDBSiteCharacteristicsDatabase::WriteSiteCharacteristicsIntoDB(
    const std::string& site_origin,
    const SiteCharacteristicsProto& site_characteristic_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&LevelDBSiteCharacteristicsDatabase::AsyncHelper::
                         WriteSiteCharacteristicsIntoDB,
                     base::Unretained(async_helper_.get()), site_origin,
                     std::move(site_characteristic_proto)));
}

void LevelDBSiteCharacteristicsDatabase::RemoveSiteCharacteristicsFromDB(
    const std::vector<std::string>& site_origins) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&LevelDBSiteCharacteristicsDatabase::AsyncHelper::
                         RemoveSiteCharacteristicsFromDB,
                     base::Unretained(async_helper_.get()),
                     std::move(site_origins)));
}

void LevelDBSiteCharacteristicsDatabase::ClearDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &LevelDBSiteCharacteristicsDatabase::AsyncHelper::ClearDatabase,
          base::Unretained(async_helper_.get())));
}

}  // namespace resource_coordinator
