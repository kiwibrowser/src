// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/mock_browsertest_indexed_db_class_factory.h"

#include <stddef.h>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/indexed_db/indexed_db_factory.h"
#include "content/browser/indexed_db/indexed_db_metadata_coding.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"
#include "content/browser/indexed_db/leveldb/leveldb_transaction.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace {

class FunctionTracer {
 public:
  FunctionTracer(const std::string& class_name,
                 const std::string& method_name,
                 int instance_num)
      : class_name_(class_name),
        method_name_(method_name),
        instance_count_(instance_num),
        current_call_num_(0) {}

  void log_call() {
    current_call_num_++;
    VLOG(0) << class_name_ << '[' << instance_count_ << "]::" << method_name_
            << "()[" << current_call_num_ << ']';
  }

 private:
  std::string class_name_;
  std::string method_name_;
  int instance_count_;
  int current_call_num_;
};

}  // namespace

namespace content {

class IndexedDBTestDatabase : public IndexedDBDatabase {
 public:
  IndexedDBTestDatabase(
      const base::string16& name,
      scoped_refptr<IndexedDBBackingStore> backing_store,
      scoped_refptr<IndexedDBFactory> factory,
      std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
      const IndexedDBDatabase::Identifier& unique_identifier)
      : IndexedDBDatabase(name,
                          backing_store,
                          factory,
                          std::move(metadata_coding),
                          unique_identifier) {}

 protected:
  ~IndexedDBTestDatabase() override {}

  size_t GetMaxMessageSizeInBytes() const override {
    return 10 * 1024 * 1024;  // 10MB
  }
};

class IndexedDBTestTransaction : public IndexedDBTransaction {
 public:
  IndexedDBTestTransaction(
      int64_t id,
      IndexedDBConnection* connection,
      const std::set<int64_t>& scope,
      blink::WebIDBTransactionMode mode,
      IndexedDBBackingStore::Transaction* backing_store_transaction)
      : IndexedDBTransaction(id,
                             connection,
                             scope,
                             mode,
                             backing_store_transaction) {}

 protected:
  ~IndexedDBTestTransaction() override {}

  // Browser tests run under memory/address sanitizers (etc) may trip the
  // default 60s timeout, so relax it during tests.
  base::TimeDelta GetInactivityTimeout() const override {
    return base::TimeDelta::FromSeconds(60 * 60);
  }
};

class LevelDBTestTransaction : public LevelDBTransaction {
 public:
  LevelDBTestTransaction(LevelDBDatabase* db,
                        FailMethod fail_method,
                        int fail_on_call_num)
      : LevelDBTransaction(db),
        fail_method_(fail_method),
        fail_on_call_num_(fail_on_call_num),
        current_call_num_(0) {
    DCHECK(fail_method != FAIL_METHOD_NOTHING);
    DCHECK_GT(fail_on_call_num, 0);
  }

  leveldb::Status Get(const base::StringPiece& key,
                      std::string* value,
                      bool* found) override {
    if (fail_method_ != FAIL_METHOD_GET ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBTransaction::Get(key, value, found);

    *found = false;
    return leveldb::Status::Corruption("Corrupted for the test");
  }

  leveldb::Status Commit() override {
    if ((fail_method_ != FAIL_METHOD_COMMIT &&
         fail_method_ != FAIL_METHOD_COMMIT_DISK_FULL) ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBTransaction::Commit();

    // TODO(jsbell): Consider parameterizing the failure mode.
    if (fail_method_ == FAIL_METHOD_COMMIT_DISK_FULL) {
      return leveldb_env::MakeIOError("dummy filename", "Disk Full",
                                      leveldb_env::kWritableFileAppend,
                                      base::File::FILE_ERROR_NO_SPACE);
    }

    return leveldb::Status::Corruption("Corrupted for the test");
  }

 private:
  ~LevelDBTestTransaction() override {}

  FailMethod fail_method_;
  int fail_on_call_num_;
  int current_call_num_;
};

class LevelDBTraceTransaction : public LevelDBTransaction {
 public:
  LevelDBTraceTransaction(LevelDBDatabase* db, int tx_num)
      : LevelDBTransaction(db),
        commit_tracer_(s_class_name, "Commit", tx_num),
        get_tracer_(s_class_name, "Get", tx_num) {}

  leveldb::Status Get(const base::StringPiece& key,
                      std::string* value,
                      bool* found) override {
    get_tracer_.log_call();
    return LevelDBTransaction::Get(key, value, found);
  }

  leveldb::Status Commit() override {
    commit_tracer_.log_call();
    return LevelDBTransaction::Commit();
  }

 private:
  static const std::string s_class_name;

  ~LevelDBTraceTransaction() override {}

  FunctionTracer commit_tracer_;
  FunctionTracer get_tracer_;
};

const std::string LevelDBTraceTransaction::s_class_name = "LevelDBTransaction";

class LevelDBTraceIteratorImpl : public LevelDBIteratorImpl {
 public:
  LevelDBTraceIteratorImpl(std::unique_ptr<leveldb::Iterator> iterator,
                           LevelDBDatabase* db,
                           const leveldb::Snapshot* snapshot,
                           int inst_num)
      : LevelDBIteratorImpl(std::move(iterator), db, snapshot),
        is_valid_tracer_(s_class_name, "IsValid", inst_num),
        seek_to_last_tracer_(s_class_name, "SeekToLast", inst_num),
        seek_tracer_(s_class_name, "Seek", inst_num),
        next_tracer_(s_class_name, "Next", inst_num),
        prev_tracer_(s_class_name, "Prev", inst_num),
        key_tracer_(s_class_name, "Key", inst_num),
        value_tracer_(s_class_name, "Value", inst_num) {}
  ~LevelDBTraceIteratorImpl() override {}

 private:
  static const std::string s_class_name;

  bool IsValid() const override {
    is_valid_tracer_.log_call();
    return LevelDBIteratorImpl::IsValid();
  }
  leveldb::Status SeekToLast() override {
    seek_to_last_tracer_.log_call();
    return LevelDBIteratorImpl::SeekToLast();
  }
  leveldb::Status Seek(const base::StringPiece& target) override {
    seek_tracer_.log_call();
    return LevelDBIteratorImpl::Seek(target);
  }
  leveldb::Status Next() override {
    next_tracer_.log_call();
    return LevelDBIteratorImpl::Next();
  }
  leveldb::Status Prev() override {
    prev_tracer_.log_call();
    return LevelDBIteratorImpl::Prev();
  }
  base::StringPiece Key() const override {
    key_tracer_.log_call();
    return LevelDBIteratorImpl::Key();
  }
  base::StringPiece Value() const override {
    value_tracer_.log_call();
    return LevelDBIteratorImpl::Value();
  }

  mutable FunctionTracer is_valid_tracer_;
  mutable FunctionTracer seek_to_last_tracer_;
  mutable FunctionTracer seek_tracer_;
  mutable FunctionTracer next_tracer_;
  mutable FunctionTracer prev_tracer_;
  mutable FunctionTracer key_tracer_;
  mutable FunctionTracer value_tracer_;
};

const std::string LevelDBTraceIteratorImpl::s_class_name = "LevelDBIterator";

class LevelDBTestIteratorImpl : public content::LevelDBIteratorImpl {
 public:
  LevelDBTestIteratorImpl(std::unique_ptr<leveldb::Iterator> iterator,
                          LevelDBDatabase* db,
                          const leveldb::Snapshot* snapshot,
                          FailMethod fail_method,
                          int fail_on_call_num)
      : LevelDBIteratorImpl(std::move(iterator), db, snapshot),
        fail_method_(fail_method),
        fail_on_call_num_(fail_on_call_num),
        current_call_num_(0) {}
  ~LevelDBTestIteratorImpl() override {}

 private:
  leveldb::Status Seek(const base::StringPiece& target) override {
    if (fail_method_ != FAIL_METHOD_SEEK ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBIteratorImpl::Seek(target);
    return leveldb::Status::Corruption("Corrupted for test");
  }

  FailMethod fail_method_;
  int fail_on_call_num_;
  int current_call_num_;
};

MockBrowserTestIndexedDBClassFactory::MockBrowserTestIndexedDBClassFactory()
    : failure_class_(FAIL_CLASS_NOTHING),
      failure_method_(FAIL_METHOD_NOTHING),
      only_trace_calls_(false) {
}

MockBrowserTestIndexedDBClassFactory::~MockBrowserTestIndexedDBClassFactory() {
}

scoped_refptr<IndexedDBDatabase>
MockBrowserTestIndexedDBClassFactory::CreateIndexedDBDatabase(
    const base::string16& name,
    scoped_refptr<IndexedDBBackingStore> backing_store,
    scoped_refptr<IndexedDBFactory> factory,
    std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
    const IndexedDBDatabase::Identifier& unique_identifier) {
  return new IndexedDBTestDatabase(name, backing_store, factory,
                                   std::move(metadata_coding),
                                   unique_identifier);
}

std::unique_ptr<IndexedDBTransaction>
MockBrowserTestIndexedDBClassFactory::CreateIndexedDBTransaction(
    int64_t id,
    IndexedDBConnection* connection,
    const std::set<int64_t>& scope,
    blink::WebIDBTransactionMode mode,
    IndexedDBBackingStore::Transaction* backing_store_transaction) {
  return std::unique_ptr<IndexedDBTransaction>(new IndexedDBTestTransaction(
      id, connection, scope, mode, backing_store_transaction));
}

scoped_refptr<LevelDBTransaction>
MockBrowserTestIndexedDBClassFactory::CreateLevelDBTransaction(
    LevelDBDatabase* db) {
  instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] =
      instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] + 1;
  if (only_trace_calls_) {
    return new LevelDBTraceTransaction(
        db, instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION]);
  } else {
    if (failure_class_ == FAIL_CLASS_LEVELDB_TRANSACTION &&
        instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] ==
            fail_on_instance_num_[FAIL_CLASS_LEVELDB_TRANSACTION]) {
      return new LevelDBTestTransaction(
          db,
          failure_method_,
          fail_on_call_num_[FAIL_CLASS_LEVELDB_TRANSACTION]);
    } else {
      return IndexedDBClassFactory::CreateLevelDBTransaction(db);
    }
  }
}

std::unique_ptr<LevelDBIteratorImpl>
MockBrowserTestIndexedDBClassFactory::CreateIteratorImpl(
    std::unique_ptr<leveldb::Iterator> iterator,
    LevelDBDatabase* db,
    const leveldb::Snapshot* snapshot) {
  instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] =
      instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] + 1;
  if (only_trace_calls_) {
    return std::make_unique<LevelDBTraceIteratorImpl>(
        std::move(iterator), db, snapshot,
        instance_count_[FAIL_CLASS_LEVELDB_ITERATOR]);
  } else {
    if (failure_class_ == FAIL_CLASS_LEVELDB_ITERATOR &&
        instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] ==
            fail_on_instance_num_[FAIL_CLASS_LEVELDB_ITERATOR]) {
      return std::make_unique<LevelDBTestIteratorImpl>(
          std::move(iterator), db, snapshot, failure_method_,
          fail_on_call_num_[FAIL_CLASS_LEVELDB_ITERATOR]);
    } else {
      return base::WrapUnique(
          new LevelDBIteratorImpl(std::move(iterator), db, snapshot));
    }
  }
}

void MockBrowserTestIndexedDBClassFactory::FailOperation(
    FailClass failure_class,
    FailMethod failure_method,
    int fail_on_instance_num,
    int fail_on_call_num) {
  VLOG(0) << "FailOperation: class=" << failure_class
          << ", method=" << failure_method
          << ", instanceNum=" << fail_on_instance_num
          << ", callNum=" << fail_on_call_num;
  DCHECK(failure_class != FAIL_CLASS_NOTHING);
  DCHECK(failure_method != FAIL_METHOD_NOTHING);
  failure_class_ = failure_class;
  failure_method_ = failure_method;
  fail_on_instance_num_[failure_class_] = fail_on_instance_num;
  fail_on_call_num_[failure_class_] = fail_on_call_num;
  instance_count_.clear();
}

void MockBrowserTestIndexedDBClassFactory::Reset() {
  failure_class_ = FAIL_CLASS_NOTHING;
  failure_method_ = FAIL_METHOD_NOTHING;
  instance_count_.clear();
  fail_on_instance_num_.clear();
  fail_on_call_num_.clear();
}

}  // namespace content
