// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/snapshot_controller.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_feature.h"

namespace {
const bool kDocumentAvailableTriggersSnapshot = true;

// Default delay, in milliseconds, between the main document parsed event and
// snapshot. Note: this snapshot might not occur if the OnLoad event and
// OnLoad delay elapses first to trigger a final snapshot.
const int64_t kDefaultDelayAfterDocumentAvailableMs = 7000;

// Default delay, in milliseconds, between the main document OnLoad event and
// snapshot.
const int64_t kDelayAfterDocumentOnLoadCompletedMsForeground = 1000;
const int64_t kDelayAfterDocumentOnLoadCompletedMsBackground = 2000;

// Default delay, in milliseconds, between renovations finishing and
// taking a snapshot. Allows for page to update in response to the
// renovations.
const int64_t kDelayAfterRenovationsCompletedMs = 2000;

// Delay for testing to keep polling times reasonable.
const int64_t kDelayForTests = 0;

}  // namespace

namespace offline_pages {

// static
std::unique_ptr<SnapshotController>
SnapshotController::CreateForForegroundOfflining(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client) {
  return std::unique_ptr<SnapshotController>(new SnapshotController(
      task_runner, client, kDefaultDelayAfterDocumentAvailableMs,
      kDelayAfterDocumentOnLoadCompletedMsForeground,
      kDelayAfterRenovationsCompletedMs, kDocumentAvailableTriggersSnapshot,
      false));
}

// static
std::unique_ptr<SnapshotController>
SnapshotController::CreateForBackgroundOfflining(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client,
    bool renovations_enabled) {
  return std::unique_ptr<SnapshotController>(new SnapshotController(
      task_runner, client, kDefaultDelayAfterDocumentAvailableMs,
      kDelayAfterDocumentOnLoadCompletedMsBackground,
      kDelayAfterRenovationsCompletedMs, !kDocumentAvailableTriggersSnapshot,
      renovations_enabled));
}

SnapshotController::SnapshotController(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    SnapshotController::Client* client,
    int64_t delay_after_document_available_ms,
    int64_t delay_after_document_on_load_completed_ms,
    int64_t delay_after_renovations_completed_ms,
    bool document_available_triggers_snapshot,
    bool renovations_enabled)
    : task_runner_(task_runner),
      client_(client),
      state_(State::READY),
      delay_after_document_available_ms_(delay_after_document_available_ms),
      delay_after_document_on_load_completed_ms_(
          delay_after_document_on_load_completed_ms),
      delay_after_renovations_completed_ms_(
          delay_after_renovations_completed_ms),
      document_available_triggers_snapshot_(
          document_available_triggers_snapshot),
      renovations_enabled_(renovations_enabled),
      weak_ptr_factory_(this) {
  if (offline_pages::ShouldUseTestingSnapshotDelay()) {
    delay_after_document_available_ms_ = kDelayForTests;
    delay_after_document_on_load_completed_ms_ = kDelayForTests;
    delay_after_renovations_completed_ms_ = kDelayForTests;
  }
}

SnapshotController::~SnapshotController() {}

void SnapshotController::Reset() {
  // Cancel potentially delayed tasks that relate to the previous 'session'.
  weak_ptr_factory_.InvalidateWeakPtrs();
  state_ = State::READY;
  current_page_quality_ = PageQuality::POOR;
}

void SnapshotController::Stop() {
  state_ = State::STOPPED;
}

void SnapshotController::PendingSnapshotCompleted() {
  // Unless the controller is "stopped", enable the subsequent snapshots.
  // Stopped state prevents any further snapshots form being started.
  if (state_ == State::STOPPED)
    return;
  state_ = State::READY;
}

void SnapshotController::RenovationsCompleted() {
  if (renovations_enabled_) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(
            delay_after_renovations_completed_ms_));
  }
}

void SnapshotController::DocumentAvailableInMainFrame() {
  if (document_available_triggers_snapshot_) {
    DCHECK_EQ(PageQuality::POOR, current_page_quality_);
    // Post a delayed task to snapshot.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshot,
                   weak_ptr_factory_.GetWeakPtr(),
                   PageQuality::FAIR_AND_IMPROVING),
        base::TimeDelta::FromMilliseconds(delay_after_document_available_ms_));
  }
}

void SnapshotController::DocumentOnLoadCompletedInMainFrame() {
  if (renovations_enabled_) {
    // Run renovations. After renovations complete, a snapshot will be
    // triggered after a delay.
    client_->RunRenovations();
  } else {
    // Post a delayed task to snapshot and then stop this controller.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SnapshotController::MaybeStartSnapshotThenStop,
                   weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(
            delay_after_document_on_load_completed_ms_));
  }
}

void SnapshotController::MaybeStartSnapshot(PageQuality updated_page_quality) {
  if (state_ != State::READY)
    return;
  DCHECK_LT(current_page_quality_, updated_page_quality);
  current_page_quality_ = updated_page_quality;
  state_ = State::SNAPSHOT_PENDING;
  client_->StartSnapshot();
}

void SnapshotController::MaybeStartSnapshotThenStop() {
  MaybeStartSnapshot(PageQuality::HIGH);
  Stop();
}

int64_t SnapshotController::GetDelayAfterDocumentAvailableForTest() {
  return delay_after_document_available_ms_;
}

int64_t SnapshotController::GetDelayAfterDocumentOnLoadCompletedForTest() {
  return delay_after_document_on_load_completed_ms_;
}

int64_t SnapshotController::GetDelayAfterRenovationsCompletedForTest() {
  return delay_after_renovations_completed_ms_;
}

}  // namespace offline_pages
