// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/top_sites_backend.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/history/core/browser/top_sites_database.h"
#include "sql/connection.h"

namespace history {

TopSitesBackend::TopSitesBackend()
    : db_(new TopSitesDatabase()),
      db_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN, base::MayBlock()})) {
  DCHECK(db_task_runner_);
}

void TopSitesBackend::Init(const base::FilePath& path) {
  db_path_ = path;
  db_task_runner_->PostTask(
      FROM_HERE, base::Bind(&TopSitesBackend::InitDBOnDBThread, this, path));
}

void TopSitesBackend::Shutdown() {
  db_task_runner_->PostTask(
      FROM_HERE, base::Bind(&TopSitesBackend::ShutdownDBOnDBThread, this));
}

void TopSitesBackend::GetMostVisitedThumbnails(
    const GetMostVisitedThumbnailsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  scoped_refptr<MostVisitedThumbnails> thumbnails = new MostVisitedThumbnails();
  tracker->PostTaskAndReply(
      db_task_runner_.get(), FROM_HERE,
      base::Bind(&TopSitesBackend::GetMostVisitedThumbnailsOnDBThread, this,
                 thumbnails),
      base::Bind(callback, thumbnails));
}

void TopSitesBackend::UpdateTopSites(const TopSitesDelta& delta,
                                     const RecordHistogram record_or_not) {
  db_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&TopSitesBackend::UpdateTopSitesOnDBThread, this, delta,
                 record_or_not));
}

void TopSitesBackend::SetPageThumbnail(const MostVisitedURL& url,
                                       int url_rank,
                                       const Images& thumbnail) {
  db_task_runner_->PostTask(
      FROM_HERE, base::Bind(&TopSitesBackend::SetPageThumbnailOnDBThread, this,
                            url, url_rank, thumbnail));
}

void TopSitesBackend::ResetDatabase() {
  db_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&TopSitesBackend::ResetDatabaseOnDBThread, this, db_path_));
}

void TopSitesBackend::DoEmptyRequest(const base::Closure& reply,
                                     base::CancelableTaskTracker* tracker) {
  tracker->PostTaskAndReply(db_task_runner_.get(), FROM_HERE, base::DoNothing(),
                            reply);
}

TopSitesBackend::~TopSitesBackend() {
  DCHECK(!db_);  // Shutdown should have happened first (which results in
                 // nulling out db).
}

void TopSitesBackend::InitDBOnDBThread(const base::FilePath& path) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  if (!db_->Init(path)) {
    LOG(ERROR) << "Failed to initialize database.";
    db_.reset();
  }
}

void TopSitesBackend::ShutdownDBOnDBThread() {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  db_.reset();
}

void TopSitesBackend::GetMostVisitedThumbnailsOnDBThread(
    scoped_refptr<MostVisitedThumbnails> thumbnails) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());

  if (db_) {
    db_->GetPageThumbnails(&(thumbnails->most_visited),
                           &(thumbnails->url_to_images_map));
  }
}

void TopSitesBackend::UpdateTopSitesOnDBThread(
    const TopSitesDelta& delta, const RecordHistogram record_or_not) {
  TRACE_EVENT0("startup", "history::TopSitesBackend::UpdateTopSitesOnDBThread");

  if (!db_)
    return;

  base::TimeTicks begin_time = base::TimeTicks::Now();

  db_->ApplyDelta(delta);

  if (record_or_not == RECORD_HISTOGRAM_YES) {
    UMA_HISTOGRAM_TIMES("History.FirstUpdateTime",
                        base::TimeTicks::Now() - begin_time);
  }
}

void TopSitesBackend::SetPageThumbnailOnDBThread(const MostVisitedURL& url,
                                                 int url_rank,
                                                 const Images& thumbnail) {
  if (!db_)
    return;

  db_->SetPageThumbnail(url, url_rank, thumbnail);
}

void TopSitesBackend::ResetDatabaseOnDBThread(const base::FilePath& file_path) {
  DCHECK(db_task_runner_->RunsTasksInCurrentSequence());
  db_.reset(nullptr);
  sql::Connection::Delete(db_path_);
  db_.reset(new TopSitesDatabase());
  InitDBOnDBThread(db_path_);
}

}  // namespace history
