// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/sync_merge_result.h"

namespace syncer {

SyncMergeResult::SyncMergeResult(ModelType type)
    : model_type_(type),
      num_items_before_association_(0),
      num_items_after_association_(0),
      num_items_added_(0),
      num_items_deleted_(0),
      num_items_modified_(0),
      pre_association_version_(0) {}

SyncMergeResult::SyncMergeResult(const SyncMergeResult& other) = default;

SyncMergeResult::~SyncMergeResult() {}

// Setters.
void SyncMergeResult::set_error(SyncError error) {
  DCHECK(!error.IsSet() || model_type_ == error.model_type());
  error_ = error;
}

void SyncMergeResult::set_num_items_before_association(
    int num_items_before_association) {
  num_items_before_association_ = num_items_before_association;
}

void SyncMergeResult::set_num_items_after_association(
    int num_items_after_association) {
  num_items_after_association_ = num_items_after_association;
}

void SyncMergeResult::set_num_items_added(int num_items_added) {
  num_items_added_ = num_items_added;
}

void SyncMergeResult::set_num_items_deleted(int num_items_deleted) {
  num_items_deleted_ = num_items_deleted;
}

void SyncMergeResult::set_num_items_modified(int num_items_modified) {
  num_items_modified_ = num_items_modified;
}

void SyncMergeResult::set_pre_association_version(int64_t version) {
  pre_association_version_ = version;
}

ModelType SyncMergeResult::model_type() const {
  return model_type_;
}

SyncError SyncMergeResult::error() const {
  return error_;
}

int SyncMergeResult::num_items_before_association() const {
  return num_items_before_association_;
}

int SyncMergeResult::num_items_after_association() const {
  return num_items_after_association_;
}

int SyncMergeResult::num_items_added() const {
  return num_items_added_;
}

int SyncMergeResult::num_items_deleted() const {
  return num_items_deleted_;
}

int SyncMergeResult::num_items_modified() const {
  return num_items_modified_;
}

int64_t SyncMergeResult::pre_association_version() const {
  return pre_association_version_;
}

}  // namespace syncer
