// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_PROTO_LEVELDB_DATABASE_H_
#define COMPONENTS_LEVELDB_PROTO_LEVELDB_DATABASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_collision_warner.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace base {
class FilePath;
class HistogramBase;
}  // namespace base

namespace leveldb {
class Cache;
class DB;
class Env;
}  // namespace leveldb

namespace leveldb_proto {

// Interacts with the LevelDB third party module.
// Once constructed, function calls and destruction should all occur on the
// same thread (not necessarily the same as the constructor).
class LevelDB {
 public:
  // Constructor. Does *not* open a leveldb - only initialize this class.
  // |client_name| is the name of the "client" that owns this instance. Used
  // for UMA statics as so: LevelDB.<value>.<client name>. It is best to not
  // change once shipped.
  explicit LevelDB(const char* client_name);
  virtual ~LevelDB();

  using KeyFilter = base::RepeatingCallback<bool(const std::string& key)>;

  // Initializes a leveldb with the given options. If |database_dir| is
  // empty, this opens an in-memory db.
  virtual bool Init(const base::FilePath& database_dir,
                    const leveldb_env::Options& options);

  virtual bool Save(const base::StringPairs& pairs_to_save,
                    const std::vector<std::string>& keys_to_remove);
  virtual bool Load(std::vector<std::string>* entries);
  virtual bool LoadWithFilter(const KeyFilter& filter,
                              std::vector<std::string>* entries);
  virtual bool LoadKeys(std::vector<std::string>* keys);
  virtual bool Get(const std::string& key, bool* found, std::string* entry);
  // Close (if currently open) and then destroy (i.e. delete) the database
  // directory.
  virtual bool Destroy();

 private:
  FRIEND_TEST_ALL_PREFIXES(ProtoDatabaseImplLevelDBTest, TestDBInitFail);

  DFAKE_MUTEX(thread_checker_);

  // The declaration order of these members matters: |db_| depends on |env_| and
  // therefore has to be destructed first.
  std::unique_ptr<leveldb::Env> env_;
  std::unique_ptr<leveldb::DB> db_;
  base::FilePath database_dir_;
  leveldb_env::Options open_options_;
  base::HistogramBase* open_histogram_;
  base::HistogramBase* destroy_histogram_;

  DISALLOW_COPY_AND_ASSIGN(LevelDB);
};

}  // namespace leveldb_proto

#endif  // COMPONENTS_LEVELDB_PROTO_LEVELDB_DATABASE_H_
