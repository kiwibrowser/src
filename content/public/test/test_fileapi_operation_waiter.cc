// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_fileapi_operation_waiter.h"

#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/observer_list.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/sandbox_file_system_backend_delegate.h"

namespace content {

using storage::FileSystemContext;
using storage::FileSystemURL;
using storage::FileUpdateObserver;

namespace {

// Because of how fileapi internally creates copies of its observer lists,
// removing an observer is not a supported operation. So to support temporary,
// test-style observers, we create one long-lived global observer instance that
// dispatches to a list of short-lived observers.
//
// This object operates on the UI thread, though it registers itself as an
// observer on the IO thread.
class FileUpdateObserverMultiplexer : public FileUpdateObserver {
 public:
  FileUpdateObserverMultiplexer() {}

  void AddObserver(FileSystemContext* context, FileUpdateObserver* observer) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    // On first initialization, install ourself as an observer. We never
    // uninstall, because we expect to leak.
    if (!context_) {
      // Currently we only listen to kFileSystemTypeTemporary; it should be fine
      // to add other filesystem types as needed.
      context_ = context;
      base::Closure task = base::Bind(
          &storage::SandboxFileSystemBackendDelegate::AddFileUpdateObserver,
          base::Unretained(context_->sandbox_delegate()),
          storage::kFileSystemTypeTemporary, base::Unretained(this),
          base::RetainedRef(
              BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));
      BrowserThread::PostTask(BrowserThread::IO, FROM_HERE, std::move(task));
    }

    CHECK_EQ(context, context_) << "Multiprofile is not implemented";

    observers_.AddObserver(observer);
  }

  void RemoveObserver(FileUpdateObserver* observer) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    observers_.RemoveObserver(observer);
  }

  // FileUpdateObserver overrides:
  void OnStartUpdate(const FileSystemURL& url) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto& observer : observers_)
      observer.OnStartUpdate(url);
  }
  void OnUpdate(const FileSystemURL& url, int64_t delta) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto& observer : observers_)
      observer.OnUpdate(url, delta);
  }
  void OnEndUpdate(const FileSystemURL& url) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto& observer : observers_)
      observer.OnEndUpdate(url);
  }

 private:
  FileSystemContext* context_ = nullptr;
  base::ObserverList<FileUpdateObserver> observers_;
  DISALLOW_COPY_AND_ASSIGN(FileUpdateObserverMultiplexer);
};

static base::LazyInstance<FileUpdateObserverMultiplexer>::Leaky g_multiplexer =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

TestFileapiOperationWaiter::TestFileapiOperationWaiter(
    FileSystemContext* context) {
  g_multiplexer.Get().AddObserver(context, this);
}

TestFileapiOperationWaiter::~TestFileapiOperationWaiter() {
  g_multiplexer.Get().RemoveObserver(this);
}

void TestFileapiOperationWaiter::WaitForEndUpdate() {
  run_loop_.Run();
}

void TestFileapiOperationWaiter::OnStartUpdate(const FileSystemURL& url) {
  did_start_update_ = true;
}

void TestFileapiOperationWaiter::OnUpdate(const FileSystemURL& url,
                                          int64_t delta) {}

void TestFileapiOperationWaiter::OnEndUpdate(const FileSystemURL& url) {
  run_loop_.Quit();
}

}  // namespace content
