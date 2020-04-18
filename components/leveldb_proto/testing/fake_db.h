// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_PROTO_TESTING_FAKE_DB_H_
#define COMPONENTS_LEVELDB_PROTO_TESTING_FAKE_DB_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "components/leveldb_proto/proto_database.h"

namespace leveldb_proto {
namespace test {

template <typename T>
class FakeDB : public ProtoDatabase<T> {
  using Callback = base::OnceCallback<void(bool)>;

 public:
  using EntryMap = std::map<std::string, T>;

  explicit FakeDB(EntryMap* db);
  ~FakeDB() override;

  // ProtoDatabase implementation.
  void Init(const char* client_name,
            const base::FilePath& database_dir,
            const leveldb_env::Options& options,
            typename ProtoDatabase<T>::InitCallback callback) override;
  void UpdateEntries(
      std::unique_ptr<typename ProtoDatabase<T>::KeyEntryVector>
          entries_to_save,
      std::unique_ptr<std::vector<std::string>> keys_to_remove,
      typename ProtoDatabase<T>::UpdateCallback callback) override;
  void LoadEntries(typename ProtoDatabase<T>::LoadCallback callback) override;
  void LoadEntriesWithFilter(
      const LevelDB::KeyFilter& key_filter,
      typename ProtoDatabase<T>::LoadCallback callback) override;
  void LoadKeys(typename ProtoDatabase<T>::LoadKeysCallback callback) override;
  void GetEntry(const std::string& key,
                typename ProtoDatabase<T>::GetCallback callback) override;
  void Destroy(typename ProtoDatabase<T>::DestroyCallback callback) override;

  base::FilePath& GetDirectory();

  void InitCallback(bool success);

  void LoadCallback(bool success);

  void LoadKeysCallback(bool success);

  void GetCallback(bool success);

  void UpdateCallback(bool success);

  void DestroyCallback(bool success);

  static base::FilePath DirectoryForTestDB();

 private:
  static void RunLoadCallback(typename ProtoDatabase<T>::LoadCallback callback,
                              std::unique_ptr<typename std::vector<T>> entries,
                              bool success);

  static void RunLoadKeysCallback(
      typename ProtoDatabase<T>::LoadKeysCallback callback,
      std::unique_ptr<std::vector<std::string>> keys,
      bool success);

  static void RunGetCallback(typename ProtoDatabase<T>::GetCallback callback,
                             std::unique_ptr<T> entry,
                             bool success);

  base::FilePath dir_;
  EntryMap* db_;

  Callback init_callback_;
  Callback load_callback_;
  Callback load_keys_callback_;
  Callback get_callback_;
  Callback update_callback_;
  Callback destroy_callback_;
};

template <typename T>
FakeDB<T>::FakeDB(EntryMap* db)
    : db_(db) {}

template <typename T>
FakeDB<T>::~FakeDB() {}

template <typename T>
void FakeDB<T>::Init(const char* client_name,
                     const base::FilePath& database_dir,
                     const leveldb_env::Options& options,
                     typename ProtoDatabase<T>::InitCallback callback) {
  dir_ = database_dir;
  init_callback_ = std::move(callback);
}

template <typename T>
void FakeDB<T>::UpdateEntries(
    std::unique_ptr<typename ProtoDatabase<T>::KeyEntryVector> entries_to_save,
    std::unique_ptr<std::vector<std::string>> keys_to_remove,
    typename ProtoDatabase<T>::UpdateCallback callback) {
  for (const auto& pair : *entries_to_save)
    (*db_)[pair.first] = pair.second;

  for (const auto& key : *keys_to_remove)
    db_->erase(key);

  update_callback_ = std::move(callback);
}

template <typename T>
void FakeDB<T>::LoadEntries(typename ProtoDatabase<T>::LoadCallback callback) {
  LoadEntriesWithFilter(LevelDB::KeyFilter(), std::move(callback));
}

template <typename T>
void FakeDB<T>::LoadEntriesWithFilter(
    const LevelDB::KeyFilter& key_filter,
    typename ProtoDatabase<T>::LoadCallback callback) {
  std::unique_ptr<std::vector<T>> entries(new std::vector<T>());
  for (const auto& pair : *db_) {
    if (key_filter.is_null() || key_filter.Run(pair.first))
      entries->push_back(pair.second);
  }

  load_callback_ =
      base::BindOnce(RunLoadCallback, std::move(callback), std::move(entries));
}

template <typename T>
void FakeDB<T>::LoadKeys(typename ProtoDatabase<T>::LoadKeysCallback callback) {
  std::unique_ptr<std::vector<std::string>> keys(
      new std::vector<std::string>());
  for (const auto& pair : *db_)
    keys->push_back(pair.first);

  load_keys_callback_ =
      base::BindOnce(RunLoadKeysCallback, std::move(callback), std::move(keys));
}

template <typename T>
void FakeDB<T>::GetEntry(const std::string& key,
                         typename ProtoDatabase<T>::GetCallback callback) {
  std::unique_ptr<T> entry;
  auto it = db_->find(key);
  if (it != db_->end())
    entry.reset(new T(it->second));

  get_callback_ =
      base::BindOnce(RunGetCallback, std::move(callback), std::move(entry));
}

template <typename T>
void FakeDB<T>::Destroy(typename ProtoDatabase<T>::DestroyCallback callback) {
  db_->clear();
  destroy_callback_ = std::move(callback);
}

template <typename T>
base::FilePath& FakeDB<T>::GetDirectory() {
  return dir_;
}

template <typename T>
void FakeDB<T>::InitCallback(bool success) {
  std::move(init_callback_).Run(success);
}

template <typename T>
void FakeDB<T>::LoadCallback(bool success) {
  std::move(load_callback_).Run(success);
}

template <typename T>
void FakeDB<T>::LoadKeysCallback(bool success) {
  std::move(load_keys_callback_).Run(success);
}

template <typename T>
void FakeDB<T>::GetCallback(bool success) {
  std::move(get_callback_).Run(success);
}

template <typename T>
void FakeDB<T>::UpdateCallback(bool success) {
  std::move(update_callback_).Run(success);
}

template <typename T>
void FakeDB<T>::DestroyCallback(bool success) {
  std::move(destroy_callback_).Run(success);
}

// static
template <typename T>
void FakeDB<T>::RunLoadCallback(
    typename ProtoDatabase<T>::LoadCallback callback,
    std::unique_ptr<typename std::vector<T>> entries,
    bool success) {
  std::move(callback).Run(success, std::move(entries));
}

// static
template <typename T>
void FakeDB<T>::RunLoadKeysCallback(
    typename ProtoDatabase<T>::LoadKeysCallback callback,
    std::unique_ptr<std::vector<std::string>> keys,
    bool success) {
  std::move(callback).Run(success, std::move(keys));
}

// static
template <typename T>
void FakeDB<T>::RunGetCallback(typename ProtoDatabase<T>::GetCallback callback,
                               std::unique_ptr<T> entry,
                               bool success) {
  std::move(callback).Run(success, std::move(entry));
}

// static
template <typename T>
base::FilePath FakeDB<T>::DirectoryForTestDB() {
  return base::FilePath(FILE_PATH_LITERAL("/fake/path"));
}

}  // namespace test
}  // namespace leveldb_proto

#endif  // COMPONENTS_LEVELDB_PROTO_TESTING_FAKE_DB_H_
