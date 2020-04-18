// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_MEDIATOR_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#import "ios/chrome/browser/ui/download/download_manager_consumer.h"
#include "ios/web/public/download/download_task_observer.h"

@protocol DownloadManagerConsumer;

namespace base {
class FilePath;
}  // namespace base

namespace net {
class URLFetcherFileWriter;
}  // namespace net

namespace web {
class DownloadTask;
}  // namespace web

// Manages a single download task by providing means to start the download and
// update consumer if download task was changed.
class DownloadManagerMediator : public web::DownloadTaskObserver {
 public:
  DownloadManagerMediator();
  ~DownloadManagerMediator() override;

  // Sets download manager consumer. Not retained by mediator.
  void SetConsumer(id<DownloadManagerConsumer> consumer);

  // Sets download task. Must be set to null when task is destroyed.
  void SetDownloadTask(web::DownloadTask* task);

  // Asynchronously starts download operation.
  void StartDowloading();

 private:
  // Asynchronously starts download operation in the given directory.
  void DownloadWithDestinationDir(const base::FilePath& destination_dir,
                                  web::DownloadTask* task,
                                  bool directory_created);

  // Asynchronously starts download operation with the given writer.
  void DownloadWithWriter(std::unique_ptr<net::URLFetcherFileWriter> writer,
                          web::DownloadTask* task,
                          int writer_initialization_status);

  // Updates consumer from web::DownloadTask.
  void UpdateConsumer();

  // Converts web::DownloadTask::State to DownloadManagerState.
  DownloadManagerState GetDownloadManagerState() const;

  // Converts DownloadTask progress [0;100] to float progress [0.0f;1.0f].
  float GetDownloadManagerProgress() const;

  // Returns accessibility announcement for download state change. -1 if there
  // is no announcement.
  int GetDownloadManagerA11yAnnouncement() const;

  // web::DownloadTaskObserver overrides:
  void OnDownloadUpdated(web::DownloadTask* task) override;
  void OnDownloadDestroyed(web::DownloadTask* task) override;

  web::DownloadTask* task_ = nullptr;
  __weak id<DownloadManagerConsumer> consumer_ = nil;
  base::WeakPtrFactory<DownloadManagerMediator> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DownloadManagerMediator);
};

#endif  // IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_MEDIATOR_H_
