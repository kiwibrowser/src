// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/dom_distiller_service.h"

#include <memory>
#include <utility>

#include "base/guid.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/dom_distiller/core/distilled_content_store.h"
#include "components/dom_distiller/core/dom_distiller_store.h"
#include "components/dom_distiller/core/proto/distilled_article.pb.h"
#include "components/dom_distiller/core/task_tracker.h"
#include "url/gurl.h"

namespace dom_distiller {

namespace {

ArticleEntry CreateSkeletonEntryForUrl(const GURL& url) {
  ArticleEntry skeleton;
  skeleton.set_entry_id(base::GenerateGUID());
  ArticleEntryPage* page = skeleton.add_pages();
  page->set_url(url.spec());

  DCHECK(IsEntryValid(skeleton));
  return skeleton;
}

void RunArticleAvailableCallback(
    const DomDistillerService::ArticleAvailableCallback& article_cb,
    const ArticleEntry& entry,
    const DistilledArticleProto* article_proto,
    bool distillation_succeeded) {
  article_cb.Run(distillation_succeeded);
}

}  // namespace

DomDistillerService::DomDistillerService(
    std::unique_ptr<DomDistillerStoreInterface> store,
    std::unique_ptr<DistillerFactory> distiller_factory,
    std::unique_ptr<DistillerPageFactory> distiller_page_factory,
    std::unique_ptr<DistilledPagePrefs> distilled_page_prefs)
    : store_(std::move(store)),
      content_store_(new InMemoryContentStore(kDefaultMaxNumCachedEntries)),
      distiller_factory_(std::move(distiller_factory)),
      distiller_page_factory_(std::move(distiller_page_factory)),
      distilled_page_prefs_(std::move(distilled_page_prefs)) {}

DomDistillerService::~DomDistillerService() {
}

syncer::SyncableService* DomDistillerService::GetSyncableService() const {
  if (!store_) {
    return nullptr;
  }
  return store_->GetSyncableService();
}

std::unique_ptr<DistillerPage> DomDistillerService::CreateDefaultDistillerPage(
    const gfx::Size& render_view_size) {
  return distiller_page_factory_->CreateDistillerPage(render_view_size);
}

std::unique_ptr<DistillerPage>
DomDistillerService::CreateDefaultDistillerPageWithHandle(
    std::unique_ptr<SourcePageHandle> handle) {
  return distiller_page_factory_->CreateDistillerPageWithHandle(
      std::move(handle));
}

const std::string DomDistillerService::AddToList(
    const GURL& url,
    std::unique_ptr<DistillerPage> distiller_page,
    const ArticleAvailableCallback& article_cb) {
  ArticleEntry entry;
  const bool is_already_added = store_ && store_->GetEntryByUrl(url, &entry);

  TaskTracker* task_tracker = nullptr;
  if (is_already_added) {
    task_tracker = GetTaskTrackerForEntry(entry);
    if (task_tracker == nullptr) {
      // Entry is in the store but there is no task tracker. This could
      // happen when distillation has already completed. For now just return
      // true.
      // TODO(shashishekhar): Change this to check if article is available,
      // An article may not be available for a variety of reasons, e.g.
      // distillation failure or blobs not available locally.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(article_cb, true));
      return entry.entry_id();
    }
  } else {
    GetOrCreateTaskTrackerForUrl(url, &task_tracker);
  }

  if (!article_cb.is_null()) {
    task_tracker->AddSaveCallback(
        base::Bind(&RunArticleAvailableCallback, article_cb));
  }

  if (!is_already_added) {
    task_tracker->AddSaveCallback(base::Bind(
        &DomDistillerService::AddDistilledPageToList, base::Unretained(this)));
    task_tracker->StartDistiller(distiller_factory_.get(),
                                 std::move(distiller_page));
    task_tracker->StartBlobFetcher();
  }

  return task_tracker->GetEntryId();
}

bool DomDistillerService::HasEntry(const std::string& entry_id) {
  return store_ && store_->GetEntryById(entry_id, nullptr);
}

std::string DomDistillerService::GetUrlForEntry(const std::string& entry_id) {
  ArticleEntry entry;
  if (store_ && store_->GetEntryById(entry_id, &entry)) {
    return entry.pages().Get(0).url();
  }
  return "";
}

std::vector<ArticleEntry> DomDistillerService::GetEntries() const {
  if (!store_) {
    return std::vector<ArticleEntry>();
  }
  return store_->GetEntries();
}

std::unique_ptr<ArticleEntry> DomDistillerService::RemoveEntry(
    const std::string& entry_id) {
  std::unique_ptr<ArticleEntry> entry(new ArticleEntry);
  entry->set_entry_id(entry_id);
  TaskTracker* task_tracker = GetTaskTrackerForEntry(*entry);
  if (task_tracker != nullptr) {
    task_tracker->CancelSaveCallbacks();
  }

  if (!store_ || !store_->GetEntryById(entry_id, entry.get())) {
    return std::unique_ptr<ArticleEntry>();
  }

  if (store_->RemoveEntry(*entry)) {
    return entry;
  }
  return std::unique_ptr<ArticleEntry>();
}

std::unique_ptr<ViewerHandle> DomDistillerService::ViewEntry(
    ViewRequestDelegate* delegate,
    std::unique_ptr<DistillerPage> distiller_page,
    const std::string& entry_id) {
  ArticleEntry entry;
  if (!store_ || !store_->GetEntryById(entry_id, &entry)) {
    return std::unique_ptr<ViewerHandle>();
  }

  TaskTracker* task_tracker = nullptr;
  bool was_created = GetOrCreateTaskTrackerForEntry(entry, &task_tracker);
  std::unique_ptr<ViewerHandle> viewer_handle =
      task_tracker->AddViewer(delegate);
  if (was_created) {
    task_tracker->StartDistiller(distiller_factory_.get(),
                                 std::move(distiller_page));
    task_tracker->StartBlobFetcher();
  }

  return viewer_handle;
}

std::unique_ptr<ViewerHandle> DomDistillerService::ViewUrl(
    ViewRequestDelegate* delegate,
    std::unique_ptr<DistillerPage> distiller_page,
    const GURL& url) {
  if (!url.is_valid()) {
    return std::unique_ptr<ViewerHandle>();
  }

  TaskTracker* task_tracker = nullptr;
  bool was_created = GetOrCreateTaskTrackerForUrl(url, &task_tracker);
  std::unique_ptr<ViewerHandle> viewer_handle =
      task_tracker->AddViewer(delegate);
  // If a distiller is already running for one URL, don't start another.
  if (was_created) {
    task_tracker->StartDistiller(distiller_factory_.get(),
                                 std::move(distiller_page));
    task_tracker->StartBlobFetcher();
  }

  return viewer_handle;
}

bool DomDistillerService::GetOrCreateTaskTrackerForUrl(
    const GURL& url,
    TaskTracker** task_tracker) {
  ArticleEntry entry;
  if (store_ && store_->GetEntryByUrl(url, &entry)) {
    return GetOrCreateTaskTrackerForEntry(entry, task_tracker);
  }

  *task_tracker = GetTaskTrackerForUrl(url);
  if (*task_tracker) {
    return false;
  }

  ArticleEntry skeleton_entry = CreateSkeletonEntryForUrl(url);
  *task_tracker = CreateTaskTracker(skeleton_entry);
  return true;
}

TaskTracker* DomDistillerService::GetTaskTrackerForUrl(const GURL& url) const {
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    if ((*it)->HasUrl(url)) {
      return (*it).get();
    }
  }
  return nullptr;
}

TaskTracker* DomDistillerService::GetTaskTrackerForEntry(
    const ArticleEntry& entry) const {
  const std::string& entry_id = entry.entry_id();
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    if ((*it)->HasEntryId(entry_id)) {
      return (*it).get();
    }
  }
  return nullptr;
}

bool DomDistillerService::GetOrCreateTaskTrackerForEntry(
    const ArticleEntry& entry,
    TaskTracker** task_tracker) {
  *task_tracker = GetTaskTrackerForEntry(entry);
  if (!*task_tracker) {
    *task_tracker = CreateTaskTracker(entry);
    return true;
  }
  return false;
}

TaskTracker* DomDistillerService::CreateTaskTracker(const ArticleEntry& entry) {
  TaskTracker::CancelCallback cancel_callback =
      base::Bind(&DomDistillerService::CancelTask, base::Unretained(this));
  tasks_.push_back(std::make_unique<TaskTracker>(entry, cancel_callback,
                                                 content_store_.get()));
  return tasks_.back().get();
}

void DomDistillerService::CancelTask(TaskTracker* task) {
  auto it = std::find_if(tasks_.begin(), tasks_.end(),
                         [task](const std::unique_ptr<TaskTracker>& t) {
                           return task == t.get();
                         });
  if (it != tasks_.end()) {
    it->release();
    tasks_.erase(it);
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, task);
  }
}

void DomDistillerService::AddDistilledPageToList(
    const ArticleEntry& entry,
    const DistilledArticleProto* article_proto,
    bool distillation_succeeded) {
  DCHECK(IsEntryValid(entry));
  if (store_ && distillation_succeeded) {
    DCHECK(article_proto);
    DCHECK_GT(article_proto->pages_size(), 0);
    store_->AddEntry(entry);
    DCHECK_EQ(article_proto->pages_size(), entry.pages_size());
  }
}

void DomDistillerService::AddObserver(DomDistillerObserver* observer) {
  DCHECK(observer);
  if (store_) {
    store_->AddObserver(observer);
  }
}

void DomDistillerService::RemoveObserver(DomDistillerObserver* observer) {
  DCHECK(observer);
  if (store_) {
    store_->RemoveObserver(observer);
  }
}

DistilledPagePrefs* DomDistillerService::GetDistilledPagePrefs() {
  return distilled_page_prefs_.get();
}

}  // namespace dom_distiller
