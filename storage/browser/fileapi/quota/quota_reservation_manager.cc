// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/quota/quota_reservation_manager.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "storage/browser/fileapi/quota/quota_reservation.h"
#include "storage/browser/fileapi/quota/quota_reservation_buffer.h"

namespace storage {

QuotaReservationManager::QuotaReservationManager(
    std::unique_ptr<QuotaBackend> backend)
    : backend_(std::move(backend)), weak_ptr_factory_(this) {
  sequence_checker_.DetachFromSequence();
}

QuotaReservationManager::~QuotaReservationManager() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
}

void QuotaReservationManager::ReserveQuota(
    const GURL& origin,
    FileSystemType type,
    int64_t size,
    const ReserveQuotaCallback& callback) {
  DCHECK(origin.is_valid());
  backend_->ReserveQuota(origin, type, size, callback);
}

void QuotaReservationManager::ReleaseReservedQuota(const GURL& origin,
                                                   FileSystemType type,
                                                   int64_t size) {
  DCHECK(origin.is_valid());
  backend_->ReleaseReservedQuota(origin, type, size);
}

void QuotaReservationManager::CommitQuotaUsage(const GURL& origin,
                                               FileSystemType type,
                                               int64_t delta) {
  DCHECK(origin.is_valid());
  backend_->CommitQuotaUsage(origin, type, delta);
}

void QuotaReservationManager::IncrementDirtyCount(const GURL& origin,
                                                 FileSystemType type) {
  DCHECK(origin.is_valid());
  backend_->IncrementDirtyCount(origin, type);
}

void QuotaReservationManager::DecrementDirtyCount(const GURL& origin,
                                                 FileSystemType type) {
  DCHECK(origin.is_valid());
  backend_->DecrementDirtyCount(origin, type);
}

scoped_refptr<QuotaReservationBuffer>
QuotaReservationManager::GetReservationBuffer(
    const GURL& origin,
    FileSystemType type) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(origin.is_valid());
  QuotaReservationBuffer** buffer =
      &reservation_buffers_[std::make_pair(origin, type)];
  if (!*buffer) {
    *buffer = new QuotaReservationBuffer(
        weak_ptr_factory_.GetWeakPtr(), origin, type);
  }
  return base::WrapRefCounted(*buffer);
}

void QuotaReservationManager::ReleaseReservationBuffer(
    QuotaReservationBuffer* reservation_buffer) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  std::pair<GURL, FileSystemType> key(reservation_buffer->origin(),
                                      reservation_buffer->type());
  DCHECK_EQ(reservation_buffers_[key], reservation_buffer);
  reservation_buffers_.erase(key);
}

scoped_refptr<QuotaReservation> QuotaReservationManager::CreateReservation(
    const GURL& origin,
    FileSystemType type) {
  DCHECK(origin.is_valid());
  return GetReservationBuffer(origin, type)->CreateReservation();
}

}  // namespace storage
