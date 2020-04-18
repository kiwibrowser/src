// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_PROTO_PROTO_DATABASE_IMPL_H_
#define COMPONENTS_LEVELDB_PROTO_PROTO_DATABASE_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_checker.h"
#include "components/leveldb_proto/leveldb_database.h"
#include "components/leveldb_proto/proto_database.h"

namespace leveldb_proto {

using KeyValueVector = base::StringPairs;
using KeyVector = std::vector<std::string>;

// When the ProtoDatabaseImpl instance is deleted, in-progress asynchronous
// operations will be completed and the corresponding callbacks will be called.
// Construction/calls/destruction should all happen on the same thread.
template <typename T>
class ProtoDatabaseImpl : public ProtoDatabase<T> {
 public:
  // All blocking calls/disk access will happen on the provided |task_runner|.
  explicit ProtoDatabaseImpl(
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);

  ~ProtoDatabaseImpl() override;

  // ProtoDatabase implementation.
  // TODO(cjhopman): Perhaps Init() shouldn't be exposed to users and not just
  //     part of the constructor
  void Init(const char* client_name,
            const base::FilePath& database_dir,
            const leveldb_env::Options& options,
            typename ProtoDatabase<T>::InitCallback callback) override;
  void UpdateEntries(
      std::unique_ptr<typename ProtoDatabase<T>::KeyEntryVector>
          entries_to_save,
      std::unique_ptr<KeyVector> keys_to_remove,
      typename ProtoDatabase<T>::UpdateCallback callback) override;
  void LoadEntries(typename ProtoDatabase<T>::LoadCallback callback) override;
  void LoadEntriesWithFilter(
      const LevelDB::KeyFilter& key_filter,
      typename ProtoDatabase<T>::LoadCallback callback) override;
  void LoadKeys(typename ProtoDatabase<T>::LoadKeysCallback callback) override;
  void GetEntry(const std::string& key,
                typename ProtoDatabase<T>::GetCallback callback) override;
  void Destroy(typename ProtoDatabase<T>::DestroyCallback callback) override;

  // Allow callers to provide their own Database implementation.
  void InitWithDatabase(std::unique_ptr<LevelDB> database,
                        const base::FilePath& database_dir,
                        const leveldb_env::Options& options,
                        typename ProtoDatabase<T>::InitCallback callback);

 private:
  base::ThreadChecker thread_checker_;

  // Used to run blocking tasks in-order.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::unique_ptr<LevelDB> db_;

  DISALLOW_COPY_AND_ASSIGN(ProtoDatabaseImpl);
};

namespace {

template <typename T>
void RunInitCallback(typename ProtoDatabase<T>::InitCallback callback,
                     const bool* success) {
  std::move(callback).Run(*success);
}

template <typename T>
void RunUpdateCallback(typename ProtoDatabase<T>::UpdateCallback callback,
                       const bool* success) {
  std::move(callback).Run(*success);
}

template <typename T>
void RunLoadCallback(typename ProtoDatabase<T>::LoadCallback callback,
                     bool* success,
                     std::unique_ptr<std::vector<T>> entries) {
  std::move(callback).Run(*success, std::move(entries));
}

template <typename T>
void RunLoadKeysCallback(typename ProtoDatabase<T>::LoadKeysCallback callback,
                         std::unique_ptr<bool> success,
                         std::unique_ptr<std::vector<std::string>> keys) {
  std::move(callback).Run(*success, std::move(keys));
}

template <typename T>
void RunGetCallback(typename ProtoDatabase<T>::GetCallback callback,
                    const bool* success,
                    const bool* found,
                    std::unique_ptr<T> entry) {
  std::move(callback).Run(*success, *found ? std::move(entry) : nullptr);
}

template <typename T>
void RunDestroyCallback(typename ProtoDatabase<T>::DestroyCallback callback,
                        const bool* success) {
  std::move(callback).Run(*success);
}

inline void InitFromTaskRunner(LevelDB* database,
                               const base::FilePath& database_dir,
                               const leveldb_env::Options& options,
                               bool* success) {
  DCHECK(success);

  // TODO(cjhopman): Histogram for database size.
  *success = database->Init(database_dir, options);
}

inline void DestroyFromTaskRunner(std::unique_ptr<LevelDB> leveldb,
                                  bool* success) {
  CHECK(success);

  *success = leveldb->Destroy();
}

template <typename T>
void UpdateEntriesFromTaskRunner(
    LevelDB* database,
    std::unique_ptr<typename ProtoDatabase<T>::KeyEntryVector> entries_to_save,
    std::unique_ptr<KeyVector> keys_to_remove,
    bool* success) {
  DCHECK(success);

  // Serialize the values from Proto to string before passing on to database.
  KeyValueVector pairs_to_save;
  for (const auto& pair : *entries_to_save) {
    pairs_to_save.push_back(
        std::make_pair(pair.first, pair.second.SerializeAsString()));
  }

  *success = database->Save(pairs_to_save, *keys_to_remove);
}

template <typename T>
void LoadEntriesFromTaskRunner(LevelDB* database,
                               const LevelDB::KeyFilter& filter,
                               std::vector<T>* entries,
                               bool* success) {
  DCHECK(success);
  DCHECK(entries);

  entries->clear();

  std::vector<std::string> loaded_entries;
  *success = database->LoadWithFilter(filter, &loaded_entries);

  for (const auto& serialized_entry : loaded_entries) {
    T entry;
    if (!entry.ParseFromString(serialized_entry)) {
      DLOG(WARNING) << "Unable to parse leveldb_proto entry";
      // TODO(cjhopman): Decide what to do about un-parseable entries.
    }

    entries->push_back(entry);
  }
}

inline void LoadKeysFromTaskRunner(LevelDB* database,
                                   std::vector<std::string>* keys,
                                   bool* success) {
  DCHECK(success);
  DCHECK(keys);
  keys->clear();
  *success = database->LoadKeys(keys);
}

template <typename T>
void GetEntryFromTaskRunner(LevelDB* database,
                            const std::string& key,
                            T* entry,
                            bool* found,
                            bool* success) {
  DCHECK(success);
  DCHECK(found);
  DCHECK(entry);

  std::string serialized_entry;
  *success = database->Get(key, found, &serialized_entry);

  if (!*success) {
    *found = false;
    return;
  }

  if (!*found)
    return;

  if (!entry->ParseFromString(serialized_entry)) {
    *found = false;
    DLOG(WARNING) << "Unable to parse leveldb_proto entry";
    // TODO(cjhopman): Decide what to do about un-parseable entries.
  }
}

}  // namespace

template <typename T>
ProtoDatabaseImpl<T>::ProtoDatabaseImpl(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : task_runner_(task_runner) {}

template <typename T>
ProtoDatabaseImpl<T>::~ProtoDatabaseImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (db_.get() && !task_runner_->DeleteSoon(FROM_HERE, db_.release()))
    DLOG(WARNING) << "Proto database will not be deleted.";
}

template <typename T>
void ProtoDatabaseImpl<T>::Init(
    const char* client_name,
    const base::FilePath& database_dir,
    const leveldb_env::Options& options,
    typename ProtoDatabase<T>::InitCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  InitWithDatabase(base::WrapUnique(new LevelDB(client_name)), database_dir,
                   options, std::move(callback));
}

template <typename T>
void ProtoDatabaseImpl<T>::Destroy(
    typename ProtoDatabase<T>::DestroyCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(db_);

  bool* success = new bool(false);
  task_runner_->PostTaskAndReply(
      FROM_HERE, base::BindOnce(DestroyFromTaskRunner, std::move(db_), success),
      base::BindOnce(RunDestroyCallback<T>, std::move(callback),
                     base::Owned(success)));
}

template <typename T>
void ProtoDatabaseImpl<T>::InitWithDatabase(
    std::unique_ptr<LevelDB> database,
    const base::FilePath& database_dir,
    const leveldb_env::Options& options,
    typename ProtoDatabase<T>::InitCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!db_);
  DCHECK(database);
  db_ = std::move(database);
  bool* success = new bool(false);
  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(InitFromTaskRunner, base::Unretained(db_.get()), database_dir,
                 options, success),
      base::BindOnce(RunInitCallback<T>, std::move(callback),
                     base::Owned(success)));
}

template <typename T>
void ProtoDatabaseImpl<T>::UpdateEntries(
    std::unique_ptr<typename ProtoDatabase<T>::KeyEntryVector> entries_to_save,
    std::unique_ptr<KeyVector> keys_to_remove,
    typename ProtoDatabase<T>::UpdateCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  bool* success = new bool(false);
  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(UpdateEntriesFromTaskRunner<T>,
                     base::Unretained(db_.get()), std::move(entries_to_save),
                     std::move(keys_to_remove), success),
      base::BindOnce(RunUpdateCallback<T>, std::move(callback),
                     base::Owned(success)));
}

template <typename T>
void ProtoDatabaseImpl<T>::LoadEntries(
    typename ProtoDatabase<T>::LoadCallback callback) {
  LoadEntriesWithFilter(LevelDB::KeyFilter(), std::move(callback));
}

template <typename T>
void ProtoDatabaseImpl<T>::LoadEntriesWithFilter(
    const LevelDB::KeyFilter& key_filter,
    typename ProtoDatabase<T>::LoadCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  bool* success = new bool(false);

  std::unique_ptr<std::vector<T>> entries(new std::vector<T>());
  // Get this pointer before entries is std::move()'d so we can use it below.
  std::vector<T>* entries_ptr = entries.get();

  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(LoadEntriesFromTaskRunner<T>, base::Unretained(db_.get()),
                     key_filter, entries_ptr, success),
      base::BindOnce(RunLoadCallback<T>, std::move(callback),
                     base::Owned(success), std::move(entries)));
}

template <typename T>
void ProtoDatabaseImpl<T>::LoadKeys(
    typename ProtoDatabase<T>::LoadKeysCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  auto success = std::make_unique<bool>(false);
  auto keys = std::make_unique<std::vector<std::string>>();
  auto load_task =
      base::Bind(LoadKeysFromTaskRunner, base::Unretained(db_.get()),
                 keys.get(), success.get());
  task_runner_->PostTaskAndReply(
      FROM_HERE, load_task,
      base::BindOnce(RunLoadKeysCallback<T>, std::move(callback),
                     std::move(success), std::move(keys)));
}

template <typename T>
void ProtoDatabaseImpl<T>::GetEntry(
    const std::string& key,
    typename ProtoDatabase<T>::GetCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  bool* success = new bool(false);
  bool* found = new bool(false);

  std::unique_ptr<T> entry(new T());
  // Get this pointer before entry is std::move()'d so we can use it below.
  T* entry_ptr = entry.get();

  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(GetEntryFromTaskRunner<T>, base::Unretained(db_.get()),
                     key, entry_ptr, found, success),
      base::BindOnce(RunGetCallback<T>, std::move(callback),
                     base::Owned(success), base::Owned(found),
                     std::move(entry)));
}

}  // namespace leveldb_proto

#endif  // COMPONENTS_LEVELDB_PROTO_PROTO_DATABASE_IMPL_H_
