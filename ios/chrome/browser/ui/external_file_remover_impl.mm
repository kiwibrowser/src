// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_file_remover_impl.h"

#include "base/bind_helpers.h"
#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/url_and_title.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_util.h"
#include "ios/chrome/browser/sessions/ios_chrome_tab_restore_service_factory.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_list.h"
#import "ios/chrome/browser/ui/external_file_controller.h"
#include "ios/web/public/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Empty callback. The closure owned by |closure_runner| will be invoked as
// part of the destructor of base::ScopedClosureRunner (which takes care of
// checking for null closure).
void RunCallback(base::ScopedClosureRunner closure_runner) {}
}  // namespace

ExternalFileRemoverImpl::ExternalFileRemoverImpl(
    ios::ChromeBrowserState* browser_state,
    sessions::TabRestoreService* tab_restore_service)
    : tab_restore_service_(tab_restore_service),
      browser_state_(browser_state),
      weak_ptr_factory_(this) {
  DCHECK(tab_restore_service_);
  tab_restore_service_->AddObserver(this);
}

ExternalFileRemoverImpl::~ExternalFileRemoverImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void ExternalFileRemoverImpl::RemoveAfterDelay(base::TimeDelta delay,
                                               base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::ScopedClosureRunner closure_runner =
      base::ScopedClosureRunner(std::move(callback));
  bool remove_all_files = delay == base::TimeDelta::FromSeconds(0);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ExternalFileRemoverImpl::RemoveFiles,
                 weak_ptr_factory_.GetWeakPtr(), remove_all_files,
                 base::Passed(&closure_runner)),
      delay);
}

void ExternalFileRemoverImpl::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tab_restore_service_) {
    tab_restore_service_->RemoveObserver(this);
    tab_restore_service_ = nullptr;
  }
  delayed_file_remove_requests_.clear();
}

void ExternalFileRemoverImpl::TabRestoreServiceChanged(
    sessions::TabRestoreService* service) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (service->IsLoaded())
    return;

  tab_restore_service_->RemoveObserver(this);
  tab_restore_service_ = nullptr;

  std::vector<DelayedFileRemoveRequest> delayed_file_remove_requests;
  delayed_file_remove_requests = std::move(delayed_file_remove_requests_);
  for (DelayedFileRemoveRequest& request : delayed_file_remove_requests) {
    RemoveFiles(request.remove_all_files, std::move(request.closure_runner));
  }
}

void ExternalFileRemoverImpl::TabRestoreServiceDestroyed(
    sessions::TabRestoreService* service) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NOTREACHED() << "Should never happen as unregistration happen in Shutdown";
}

void ExternalFileRemoverImpl::Remove(bool all_files,
                                     base::ScopedClosureRunner closure_runner) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!tab_restore_service_) {
    RemoveFiles(all_files, std::move(closure_runner));
    return;
  }
  // Removal is delayed until tab restore loading completes.
  DCHECK(!tab_restore_service_->IsLoaded());
  DelayedFileRemoveRequest request = {all_files, std::move(closure_runner)};
  delayed_file_remove_requests_.push_back(std::move(request));
  if (delayed_file_remove_requests_.size() == 1)
    tab_restore_service_->LoadTabsFromLastSession();
}

void ExternalFileRemoverImpl::RemoveFiles(
    bool all_files,
    base::ScopedClosureRunner closure_runner) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NSSet* referenced_files = all_files ? GetReferencedExternalFiles() : nil;

  const NSInteger kMinimumAgeInDays = 30;
  NSInteger age_in_days = all_files ? 0 : kMinimumAgeInDays;

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindBlockArc(^{
        [ExternalFileController removeFilesExcluding:referenced_files
                                           olderThan:age_in_days];
      }),
      base::Bind(&RunCallback, base::Passed(&closure_runner)));
}

NSSet* ExternalFileRemoverImpl::GetReferencedExternalFiles() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Add files from all TabModels.
  NSMutableSet* referenced_external_files = [NSMutableSet set];
  for (TabModel* tab_model in TabModelList::GetTabModelsForChromeBrowserState(
           browser_state_)) {
    NSSet* tab_model_files = [tab_model currentlyReferencedExternalFiles];
    if (tab_model_files) {
      [referenced_external_files unionSet:tab_model_files];
    }
  }

  bookmarks::BookmarkModel* bookmark_model =
      ios::BookmarkModelFactory::GetForBrowserState(browser_state_);
  // Check if the bookmark model is loaded.
  if (!bookmark_model || !bookmark_model->loaded())
    return referenced_external_files;

  // Add files from Bookmarks.
  std::vector<bookmarks::UrlAndTitle> bookmarks;
  bookmark_model->GetBookmarks(&bookmarks);
  for (const auto& bookmark : bookmarks) {
    GURL bookmark_url = bookmark.url;
    if (UrlIsExternalFileReference(bookmark_url)) {
      [referenced_external_files
          addObject:base::SysUTF8ToNSString(bookmark_url.ExtractFileName())];
    }
  }
  return referenced_external_files;
}
