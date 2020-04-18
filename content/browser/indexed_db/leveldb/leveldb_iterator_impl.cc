// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"

static leveldb::Slice MakeSlice(const base::StringPiece& s) {
  return leveldb::Slice(s.begin(), s.size());
}

static base::StringPiece MakeStringPiece(const leveldb::Slice& s) {
  return base::StringPiece(s.data(), s.size());
}

namespace content {

LevelDBIteratorImpl::~LevelDBIteratorImpl() {
  db_->OnIteratorDestroyed(this);
}

LevelDBIteratorImpl::LevelDBIteratorImpl(std::unique_ptr<leveldb::Iterator> it,
                                         LevelDBDatabase* db,
                                         const leveldb::Snapshot* snapshot)
    : iterator_(std::move(it)), db_(db), snapshot_(snapshot) {}

leveldb::Status LevelDBIteratorImpl::CheckStatus() {
  DCHECK(!IsDetached());
  const leveldb::Status& s = iterator_->status();
  if (!s.ok())
    LOG(ERROR) << "LevelDB iterator error: " << s.ToString();
  return s;
}

bool LevelDBIteratorImpl::IsValid() const {
  switch (iterator_state_) {
    case IteratorState::EVICTED_AND_VALID:
      return true;
    case IteratorState::EVICTED_AND_INVALID:
      return false;
    case IteratorState::ACTIVE:
      return iterator_->Valid();
  }
  NOTREACHED();
  return false;
}

leveldb::Status LevelDBIteratorImpl::SeekToLast() {
  WillUseDBIterator();
  DCHECK(iterator_);
  iterator_->SeekToLast();
  return CheckStatus();
}

leveldb::Status LevelDBIteratorImpl::Seek(const base::StringPiece& target) {
  WillUseDBIterator();
  DCHECK(iterator_);
  iterator_->Seek(MakeSlice(target));
  return CheckStatus();
}

leveldb::Status LevelDBIteratorImpl::Next() {
  DCHECK(IsValid());
  WillUseDBIterator();
  DCHECK(iterator_);
  iterator_->Next();
  return CheckStatus();
}

leveldb::Status LevelDBIteratorImpl::Prev() {
  DCHECK(IsValid());
  WillUseDBIterator();
  DCHECK(iterator_);
  iterator_->Prev();
  return CheckStatus();
}

base::StringPiece LevelDBIteratorImpl::Key() const {
  DCHECK(IsValid());
  if (IsDetached())
    return key_before_eviction_;
  return MakeStringPiece(iterator_->key());
}

base::StringPiece LevelDBIteratorImpl::Value() const {
  DCHECK(IsValid());
  // Always need to update the LRU, so we always call this. Const-cast needed,
  // as we're implementing a caching layer.
  LevelDBIteratorImpl* non_const = const_cast<LevelDBIteratorImpl*>(this);
  non_const->WillUseDBIterator();
  return MakeStringPiece(iterator_->value());
}

void LevelDBIteratorImpl::Detach() {
  DCHECK(!IsDetached());
  if (iterator_->Valid()) {
    iterator_state_ = IteratorState::EVICTED_AND_VALID;
    key_before_eviction_ = iterator_->key().ToString();
  } else {
    iterator_state_ = IteratorState::EVICTED_AND_INVALID;
  }
  iterator_.reset();
}

bool LevelDBIteratorImpl::IsDetached() const {
  return iterator_state_ != IteratorState::ACTIVE;
}

void LevelDBIteratorImpl::WillUseDBIterator() {
  db_->OnIteratorUsed(this);
  if (!IsDetached())
    return;

  iterator_ = db_->CreateLevelDBIterator(snapshot_);
  if (iterator_state_ == IteratorState::EVICTED_AND_VALID) {
    iterator_->Seek(key_before_eviction_);
    key_before_eviction_.clear();
    DCHECK(IsValid());
  } else {
    DCHECK(!iterator_->Valid());
  }
  iterator_state_ = IteratorState::ACTIVE;
}

}  // namespace content
