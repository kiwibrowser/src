// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_database.h"

#include <inttypes.h>
#include <stdint.h>

#include <algorithm>
#include <cerrno>
#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/time/default_clock.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "build/build_config.h"
#include "content/browser/indexed_db/indexed_db_class_factory.h"
#include "content/browser/indexed_db/indexed_db_tracing.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_env.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"
#include "content/browser/indexed_db/leveldb/leveldb_write_batch.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/filter_policy.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"

using base::StringPiece;
using leveldb_env::DBTracker;

namespace content {

namespace {

// Forcing flushes to disk at the end of a transaction guarantees that the
// data hit disk, but drastically impacts throughput when the filesystem is
// busy with background compactions. Not syncing trades off reliability for
// performance. Note that background compactions which move data from the
// log to SSTs are always done with reliable writes.
//
// Sync writes are necessary on Windows for quota calculations; POSIX
// calculates file sizes correctly even when not synced to disk.
#if defined(OS_WIN)
const bool kSyncWrites = true;
#else
// TODO(dgrogan): Either remove the #if block or change this back to false.
// See http://crbug.com/338385.
const bool kSyncWrites = true;
#endif

class LockImpl : public LevelDBLock {
 public:
  explicit LockImpl(leveldb::Env* env, leveldb::FileLock* lock)
      : env_(env), lock_(lock) {}
  ~LockImpl() override { env_->UnlockFile(lock_); }

 private:
  leveldb::Env* env_;
  leveldb::FileLock* lock_;

  DISALLOW_COPY_AND_ASSIGN(LockImpl);
};

class ComparatorAdapter : public leveldb::Comparator {
 public:
  explicit ComparatorAdapter(const LevelDBComparator* comparator)
      : comparator_(comparator) {}

  int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override {
    return comparator_->Compare(leveldb_env::MakeStringPiece(a),
                                leveldb_env::MakeStringPiece(b));
  }

  const char* Name() const override { return comparator_->Name(); }

  // TODO(jsbell): Support the methods below in the future.
  void FindShortestSeparator(std::string* start,
                             const leveldb::Slice& limit) const override {}

  void FindShortSuccessor(std::string* key) const override {}

 private:
  const LevelDBComparator* comparator_;
};

leveldb::Status OpenDB(
    leveldb::Comparator* comparator,
    leveldb::Env* env,
    const base::FilePath& path,
    std::unique_ptr<leveldb::DB>* db,
    std::unique_ptr<const leveldb::FilterPolicy>* filter_policy) {
  filter_policy->reset(leveldb::NewBloomFilterPolicy(10));
  leveldb_env::Options options;
  options.comparator = comparator;
  options.create_if_missing = true;
  options.paranoid_checks = true;
  options.filter_policy = filter_policy->get();
  options.compression = leveldb::kSnappyCompression;
  options.write_buffer_size =
      leveldb_env::WriteBufferSize(base::SysInfo::AmountOfTotalDiskSpace(path));

  // For info about the troubles we've run into with this parameter, see:
  // https://code.google.com/p/chromium/issues/detail?id=227313#c11
  options.max_open_files = 80;
  options.env = env;
  options.block_cache = leveldb_chrome::GetSharedWebBlockCache();

  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  return leveldb_env::OpenDB(options, path.AsUTF8Unsafe(), db);
}

int CheckFreeSpace(const char* const type, const base::FilePath& file_name) {
  std::string name =
      std::string("WebCore.IndexedDB.LevelDB.Open") + type + "FreeDiskSpace";
  int64_t free_disk_space_in_k_bytes =
      base::SysInfo::AmountOfFreeDiskSpace(file_name) / 1024;
  if (free_disk_space_in_k_bytes < 0) {
    base::Histogram::FactoryGet(
        "WebCore.IndexedDB.LevelDB.FreeDiskSpaceFailure",
        1,
        2 /*boundary*/,
        2 /*boundary*/ + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(1 /*sample*/);
    return -1;
  }
  int clamped_disk_space_k_bytes = free_disk_space_in_k_bytes > INT_MAX
                                       ? INT_MAX
                                       : free_disk_space_in_k_bytes;
  const uint64_t histogram_max = static_cast<uint64_t>(1e9);
  static_assert(histogram_max <= INT_MAX, "histogram_max too big");
  base::Histogram::FactoryGet(name,
                              1,
                              histogram_max,
                              11 /*buckets*/,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(clamped_disk_space_k_bytes);
  return clamped_disk_space_k_bytes;
}

void ParseAndHistogramIOErrorDetails(const std::string& histogram_name,
                                     const leveldb::Status& s) {
  leveldb_env::MethodID method;
  base::File::Error error = base::File::FILE_OK;
  leveldb_env::ErrorParsingResult result =
      leveldb_env::ParseMethodAndError(s, &method, &error);
  if (result == leveldb_env::NONE)
    return;
  std::string method_histogram_name(histogram_name);
  method_histogram_name.append(".EnvMethod");
  base::LinearHistogram::FactoryGet(
      method_histogram_name,
      1,
      leveldb_env::kNumEntries,
      leveldb_env::kNumEntries + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)->Add(method);

  std::string error_histogram_name(histogram_name);

  if (result == leveldb_env::METHOD_AND_BFE) {
    DCHECK_LT(error, 0);
    error_histogram_name.append(std::string(".BFE.") +
                                leveldb_env::MethodIDToString(method));
    base::LinearHistogram::FactoryGet(
        error_histogram_name,
        1,
        -base::File::FILE_ERROR_MAX,
        -base::File::FILE_ERROR_MAX + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(-error);
  }
}

void ParseAndHistogramCorruptionDetails(const std::string& histogram_name,
                                        const leveldb::Status& status) {
  int error = leveldb_env::GetCorruptionCode(status);
  DCHECK_GE(error, 0);
  std::string corruption_histogram_name(histogram_name);
  corruption_histogram_name.append(".Corruption");
  const int kNumPatterns = leveldb_env::GetNumCorruptionCodes();
  base::LinearHistogram::FactoryGet(
      corruption_histogram_name,
      1,
      kNumPatterns,
      kNumPatterns + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)->Add(error);
}

void HistogramLevelDBError(const std::string& histogram_name,
                           const leveldb::Status& s) {
  if (s.ok()) {
    NOTREACHED();
    return;
  }
  enum {
    LEVEL_DB_NOT_FOUND,
    LEVEL_DB_CORRUPTION,
    LEVEL_DB_IO_ERROR,
    LEVEL_DB_OTHER,
    LEVEL_DB_MAX_ERROR
  };
  int leveldb_error = LEVEL_DB_OTHER;
  if (s.IsNotFound())
    leveldb_error = LEVEL_DB_NOT_FOUND;
  else if (s.IsCorruption())
    leveldb_error = LEVEL_DB_CORRUPTION;
  else if (s.IsIOError())
    leveldb_error = LEVEL_DB_IO_ERROR;
  base::Histogram::FactoryGet(histogram_name,
                              1,
                              LEVEL_DB_MAX_ERROR,
                              LEVEL_DB_MAX_ERROR + 1,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(leveldb_error);
  if (s.IsIOError())
    ParseAndHistogramIOErrorDetails(histogram_name, s);
  else
    ParseAndHistogramCorruptionDetails(histogram_name, s);
}

}  // namespace

LevelDBSnapshot::LevelDBSnapshot(LevelDBDatabase* db)
    : db_(db->db_.get()), snapshot_(db_->GetSnapshot()) {}

LevelDBSnapshot::~LevelDBSnapshot() {
  db_->ReleaseSnapshot(snapshot_);
}

LevelDBDatabase::LevelDBDatabase(size_t max_open_iterators)
    : clock_(new base::DefaultClock()), iterator_lru_(max_open_iterators) {
  DCHECK(max_open_iterators);
}

LevelDBDatabase::~LevelDBDatabase() {
  LOCAL_HISTOGRAM_COUNTS_10000("Storage.IndexedDB.LevelDB.MaxIterators",
                               max_iterators_);
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      this);
  // db_'s destructor uses comparator_adapter_; order of deletion is important.
  CloseDatabase();
  comparator_adapter_.reset();
  env_.reset();
}

void LevelDBDatabase::CloseDatabase() {
  if (db_) {
    base::TimeTicks begin_time = base::TimeTicks::Now();
    db_.reset();
    UMA_HISTOGRAM_MEDIUM_TIMES("WebCore.IndexedDB.LevelDB.CloseTime",
                               base::TimeTicks::Now() - begin_time);
  }
}

// static
leveldb::Status LevelDBDatabase::Destroy(const base::FilePath& file_name) {
  leveldb_env::Options options;
  options.env = LevelDBEnv::Get();
  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  return leveldb::DestroyDB(file_name.AsUTF8Unsafe(), options);
}

// static
std::unique_ptr<LevelDBLock> LevelDBDatabase::LockForTesting(
    const base::FilePath& file_name) {
  leveldb::Env* env = LevelDBEnv::Get();
  base::FilePath lock_path = file_name.AppendASCII("LOCK");
  leveldb::FileLock* lock = nullptr;
  leveldb::Status status = env->LockFile(lock_path.AsUTF8Unsafe(), &lock);
  if (!status.ok())
    return std::unique_ptr<LevelDBLock>();
  DCHECK(lock);
  return std::make_unique<LockImpl>(env, lock);
}

// static
leveldb::Status LevelDBDatabase::Open(const base::FilePath& file_name,
                                      const LevelDBComparator* comparator,
                                      size_t max_open_cursors,
                                      std::unique_ptr<LevelDBDatabase>* result,
                                      bool* is_disk_full) {
  IDB_TRACE("LevelDBDatabase::Open");
  base::TimeTicks begin_time = base::TimeTicks::Now();

  std::unique_ptr<ComparatorAdapter> comparator_adapter(
      std::make_unique<ComparatorAdapter>(comparator));

  std::unique_ptr<leveldb::DB> db;
  std::unique_ptr<const leveldb::FilterPolicy> filter_policy;
  const leveldb::Status s = OpenDB(comparator_adapter.get(), LevelDBEnv::Get(),
                                   file_name, &db, &filter_policy);

  if (!s.ok()) {
    HistogramLevelDBError("WebCore.IndexedDB.LevelDBOpenErrors", s);
    int free_space_k_bytes = CheckFreeSpace("Failure", file_name);
    // Disks with <100k of free space almost never succeed in opening a
    // leveldb database.
    if (is_disk_full)
      *is_disk_full = free_space_k_bytes >= 0 && free_space_k_bytes < 100;

    LOG(ERROR) << "Failed to open LevelDB database from "
               << file_name.AsUTF8Unsafe() << "," << s.ToString();
    return s;
  }

  UMA_HISTOGRAM_MEDIUM_TIMES("WebCore.IndexedDB.LevelDB.OpenTime",
                             base::TimeTicks::Now() - begin_time);

  CheckFreeSpace("Success", file_name);

  (*result) = base::WrapUnique(new LevelDBDatabase(max_open_cursors));
  (*result)->db_ = std::move(db);
  (*result)->comparator_adapter_ = std::move(comparator_adapter);
  (*result)->comparator_ = comparator;
  (*result)->filter_policy_ = std::move(filter_policy);
  (*result)->file_name_for_tracing = file_name.BaseName().AsUTF8Unsafe();

  return s;
}

// static
std::unique_ptr<LevelDBDatabase> LevelDBDatabase::OpenInMemory(
    const LevelDBComparator* comparator) {
  std::unique_ptr<ComparatorAdapter> comparator_adapter(
      std::make_unique<ComparatorAdapter>(comparator));
  std::unique_ptr<leveldb::Env> in_memory_env(
      leveldb_chrome::NewMemEnv("indexed-db", LevelDBEnv::Get()));

  std::unique_ptr<leveldb::DB> db;
  std::unique_ptr<const leveldb::FilterPolicy> filter_policy;
  const leveldb::Status s = OpenDB(comparator_adapter.get(),
                                   in_memory_env.get(),
                                   base::FilePath(),
                                   &db,
                                   &filter_policy);

  if (!s.ok()) {
    LOG(ERROR) << "Failed to open in-memory LevelDB database: " << s.ToString();
    return std::unique_ptr<LevelDBDatabase>();
  }

  std::unique_ptr<LevelDBDatabase> result = base::WrapUnique(
      new LevelDBDatabase(kDefaultMaxOpenIteratorsPerDatabase));
  result->env_ = std::move(in_memory_env);
  result->db_ = std::move(db);
  result->comparator_adapter_ = std::move(comparator_adapter);
  result->comparator_ = comparator;
  result->filter_policy_ = std::move(filter_policy);
  result->file_name_for_tracing = "in-memory-database";

  return result;
}

leveldb::Status LevelDBDatabase::Put(const StringPiece& key,
                                     std::string* value) {
  base::TimeTicks begin_time = base::TimeTicks::Now();

  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s = db_->Put(write_options, leveldb_env::MakeSlice(key),
                                     leveldb_env::MakeSlice(*value));
  if (!s.ok())
    LOG(ERROR) << "LevelDB put failed: " << s.ToString();
  else
    UMA_HISTOGRAM_TIMES("WebCore.IndexedDB.LevelDB.PutTime",
                        base::TimeTicks::Now() - begin_time);
  last_modified_ = clock_->Now();
  return s;
}

leveldb::Status LevelDBDatabase::Remove(const StringPiece& key) {
  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s =
      db_->Delete(write_options, leveldb_env::MakeSlice(key));
  if (!s.IsNotFound())
    LOG(ERROR) << "LevelDB remove failed: " << s.ToString();
  last_modified_ = clock_->Now();
  return s;
}

leveldb::Status LevelDBDatabase::Get(const StringPiece& key,
                                     std::string* value,
                                     bool* found,
                                     const LevelDBSnapshot* snapshot) {
  *found = false;
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : nullptr;

  const leveldb::Status s =
      db_->Get(read_options, leveldb_env::MakeSlice(key), value);
  if (s.ok()) {
    *found = true;
    return s;
  }
  if (s.IsNotFound())
    return leveldb::Status::OK();
  HistogramLevelDBError("WebCore.IndexedDB.LevelDBReadErrors", s);
  LOG(ERROR) << "LevelDB get failed: " << s.ToString();
  return s;
}

leveldb::Status LevelDBDatabase::Write(const LevelDBWriteBatch& write_batch) {
  base::TimeTicks begin_time = base::TimeTicks::Now();
  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s =
      db_->Write(write_options, write_batch.write_batch_.get());
  if (!s.ok()) {
    HistogramLevelDBError("WebCore.IndexedDB.LevelDBWriteErrors", s);
    LOG(ERROR) << "LevelDB write failed: " << s.ToString();
  } else {
    UMA_HISTOGRAM_TIMES("WebCore.IndexedDB.LevelDB.WriteTime",
                        base::TimeTicks::Now() - begin_time);
  }
  last_modified_ = clock_->Now();
  return s;
}

std::unique_ptr<LevelDBIterator> LevelDBDatabase::CreateIterator(
    const leveldb::ReadOptions& options) {
  num_iterators_++;
  max_iterators_ = std::max(max_iterators_, num_iterators_);
  // Iterator isn't added to lru cache until it is used, as memory isn't loaded
  // for the iterator until it's first Seek call.
  std::unique_ptr<leveldb::Iterator> i(db_->NewIterator(options));
  return std::unique_ptr<LevelDBIterator>(
      IndexedDBClassFactory::Get()->CreateIteratorImpl(std::move(i), this,
                                                       options.snapshot));
}

const LevelDBComparator* LevelDBDatabase::Comparator() const {
  return comparator_;
}

void LevelDBDatabase::Compact(const base::StringPiece& start,
                              const base::StringPiece& stop) {
  IDB_TRACE("LevelDBDatabase::Compact");
  const leveldb::Slice start_slice = leveldb_env::MakeSlice(start);
  const leveldb::Slice stop_slice = leveldb_env::MakeSlice(stop);
  // NULL batch means just wait for earlier writes to be done
  db_->Write(leveldb::WriteOptions(), nullptr);
  db_->CompactRange(&start_slice, &stop_slice);
}

void LevelDBDatabase::CompactAll() {
  db_->CompactRange(nullptr, nullptr);
}

leveldb::ReadOptions LevelDBDatabase::DefaultReadOptions() {
  return DefaultReadOptions(nullptr);
}

leveldb::ReadOptions LevelDBDatabase::DefaultReadOptions(
    const LevelDBSnapshot* snapshot) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : nullptr;
  return read_options;
}

bool LevelDBDatabase::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  if (!db_)
    return false;
  // All leveldb databases are already dumped by leveldb_env::DBTracker. Add
  // an edge to the existing database.
  auto* db_tracker_dump =
      leveldb_env::DBTracker::GetOrCreateAllocatorDump(pmd, db_.get());
  if (!db_tracker_dump)
    return true;

  auto* db_dump = pmd->CreateAllocatorDump(
      base::StringPrintf("site_storage/index_db/db_0x%" PRIXPTR,
                         reinterpret_cast<uintptr_t>(db_.get())));
  db_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                     base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                     db_tracker_dump->GetSizeInternal());
  pmd->AddOwnershipEdge(db_dump->guid(), db_tracker_dump->guid());

  if (env_ && leveldb_chrome::IsMemEnv(env_.get())) {
    // All leveldb env's are already dumped by leveldb_env::DBTracker. Add
    // an edge to the existing env.
    auto* env_tracker_dump =
        DBTracker::GetOrCreateAllocatorDump(pmd, env_.get());
    auto* env_dump = pmd->CreateAllocatorDump(
        base::StringPrintf("site_storage/index_db/memenv_0x%" PRIXPTR,
                           reinterpret_cast<uintptr_t>(env_.get())));
    env_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                        base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                        env_tracker_dump->GetSizeInternal());
    pmd->AddOwnershipEdge(env_dump->guid(), env_tracker_dump->guid());
  }

  // Dumps in BACKGROUND mode can only have whitelisted strings (and there are
  // currently none) so return early.
  if (args.level_of_detail ==
      base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND) {
    return true;
  }

  db_dump->AddString("file_name", "", file_name_for_tracing);

  return true;
}

void LevelDBDatabase::SetClockForTesting(std::unique_ptr<base::Clock> clock) {
  clock_ = std::move(clock);
}

std::unique_ptr<leveldb::Iterator> LevelDBDatabase::CreateLevelDBIterator(
    const leveldb::Snapshot* snapshot) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  read_options.snapshot = snapshot;
  return std::unique_ptr<leveldb::Iterator>(db_->NewIterator(read_options));
}

LevelDBDatabase::DetachIteratorOnDestruct::~DetachIteratorOnDestruct() {
  if (it_)
    it_->Detach();
}

void LevelDBDatabase::OnIteratorUsed(LevelDBIterator* iter) {
  // This line updates the LRU if the item exists.
  if (iterator_lru_.Get(iter) != iterator_lru_.end())
    return;
  DetachIteratorOnDestruct purger(iter);
  iterator_lru_.Put(iter, std::move(purger));
}

void LevelDBDatabase::OnIteratorDestroyed(LevelDBIterator* iter) {
  DCHECK_GT(num_iterators_, 0u);
  --num_iterators_;
  auto it = iterator_lru_.Peek(iter);
  if (it == iterator_lru_.end())
    return;
  iterator_lru_.Erase(it);
}

}  // namespace content
