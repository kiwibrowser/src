// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/leveldb/public/cpp/remote_iterator.h"

#include "components/services/leveldb/public/cpp/util.h"

namespace leveldb {

RemoteIterator::RemoteIterator(mojom::LevelDBDatabase* database,
                               const base::UnguessableToken& iterator)
    : database_(database),
      iterator_(iterator),
      valid_(false),
      status_(mojom::DatabaseError::OK) {}

RemoteIterator::~RemoteIterator() {
  database_->ReleaseIterator(iterator_);
}

bool RemoteIterator::Valid() const {
  return valid_;
}

void RemoteIterator::SeekToFirst() {
  database_->IteratorSeekToFirst(iterator_, &valid_, &status_, &key_, &value_);
}

void RemoteIterator::SeekToLast() {
  database_->IteratorSeekToLast(iterator_, &valid_, &status_, &key_, &value_);
}

void RemoteIterator::Seek(const Slice& target) {
  database_->IteratorSeek(iterator_, GetVectorFor(target), &valid_, &status_,
                          &key_, &value_);
}

void RemoteIterator::Next() {
  database_->IteratorNext(iterator_, &valid_, &status_, &key_, &value_);
}

void RemoteIterator::Prev() {
  database_->IteratorPrev(iterator_, &valid_, &status_, &key_, &value_);
}

Slice RemoteIterator::key() const {
  if (!key_)
    return leveldb::Slice();
  return GetSliceFor(*key_);
}

Slice RemoteIterator::value() const {
  if (!value_)
    return leveldb::Slice();
  return GetSliceFor(*value_);
}

Status RemoteIterator::status() const {
  return DatabaseErrorToStatus(status_, key(), value());
}

}  // namespace leveldb
