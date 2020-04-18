// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fileapi/webfilewriter_impl.h"

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/worker_thread.h"
#include "content/renderer/fileapi/file_system_dispatcher.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

namespace {

FileSystemDispatcher* GetFileSystemDispatcher() {
  return RenderThreadImpl::current()
             ? RenderThreadImpl::current()->file_system_dispatcher()
             : nullptr;
}

}  // namespace

typedef FileSystemDispatcher::StatusCallback StatusCallback;
typedef FileSystemDispatcher::WriteCallback WriteCallback;

// This instance may be created outside main thread but runs mainly
// on main thread.
class WebFileWriterImpl::WriterBridge
    : public base::RefCountedThreadSafe<WriterBridge> {
 public:
  WriterBridge(WebFileWriterImpl::Type type)
      : request_id_(0),
        running_on_worker_(WorkerThread::GetCurrentId() > 0),
        task_runner_(running_on_worker_ ? base::ThreadTaskRunnerHandle::Get()
                                        : nullptr),
        written_bytes_(0) {
    if (type == WebFileWriterImpl::TYPE_SYNC) {
      waitable_event_.reset(new base::WaitableEvent(
          base::WaitableEvent::ResetPolicy::AUTOMATIC,
          base::WaitableEvent::InitialState::NOT_SIGNALED));
    }
  }

  void Truncate(const GURL& path,
                int64_t offset,
                const StatusCallback& status_callback) {
    status_callback_ = status_callback;
    if (!GetFileSystemDispatcher())
      return;
    RenderThreadImpl::current()->file_system_dispatcher()->Truncate(
        path, offset, &request_id_,
        base::Bind(&WriterBridge::DidFinish, this));
  }

  void Write(const GURL& path,
             const std::string& id,
             int64_t offset,
             const WriteCallback& write_callback,
             const StatusCallback& error_callback) {
    write_callback_ = write_callback;
    status_callback_ = error_callback;
    if (!GetFileSystemDispatcher())
      return;
    RenderThreadImpl::current()->file_system_dispatcher()->Write(
        path, id, offset, &request_id_,
        base::Bind(&WriterBridge::DidWrite, this),
        base::Bind(&WriterBridge::DidFinish, this));
  }

  void Cancel(const StatusCallback& status_callback) {
    status_callback_ = status_callback;
    if (!GetFileSystemDispatcher())
      return;
    RenderThreadImpl::current()->file_system_dispatcher()->Cancel(
        request_id_,
        base::Bind(&WriterBridge::DidFinish, this));
  }

  base::WaitableEvent* waitable_event() {
    return waitable_event_.get();
  }

  void WaitAndRun() {
    waitable_event_->Wait();
    DCHECK(!results_closure_.is_null());
    results_closure_.Run();
  }

 private:
  friend class base::RefCountedThreadSafe<WriterBridge>;
  virtual ~WriterBridge() {}

  void DidWrite(int64_t bytes, bool complete) {
    written_bytes_ += bytes;
    if (waitable_event_ && !complete)
      return;
    PostTaskToWorker(base::Bind(write_callback_, written_bytes_, complete));
  }

  void DidFinish(base::File::Error status) {
    PostTaskToWorker(base::Bind(status_callback_, status));
  }

  void PostTaskToWorker(const base::Closure& closure) {
    written_bytes_ = 0;
    if (!running_on_worker_) {
      DCHECK(!waitable_event_);
      closure.Run();
      return;
    }
    DCHECK(task_runner_);
    if (waitable_event_) {
      results_closure_ = closure;
      waitable_event_->Signal();
      return;
    }
    task_runner_->PostTask(FROM_HERE, closure);
  }

  StatusCallback status_callback_;
  WriteCallback write_callback_;
  int request_id_;
  const bool running_on_worker_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  int written_bytes_;
  std::unique_ptr<base::WaitableEvent> waitable_event_;
  base::Closure results_closure_;
};

WebFileWriterImpl::WebFileWriterImpl(
     const GURL& path, blink::WebFileWriterClient* client,
     Type type,
     const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner)
  : WebFileWriterBase(path, client),
    main_thread_task_runner_(main_thread_task_runner),
    bridge_(new WriterBridge(type)) {
}

WebFileWriterImpl::~WebFileWriterImpl() {
}

void WebFileWriterImpl::DoTruncate(const GURL& path, int64_t offset) {
  RunOnMainThread(base::Bind(&WriterBridge::Truncate, bridge_,
      path, offset,
      base::Bind(&WebFileWriterImpl::DidFinish, AsWeakPtr())));
}

void WebFileWriterImpl::DoWrite(const GURL& path,
                                const std::string& blob_id,
                                int64_t offset) {
  RunOnMainThread(base::Bind(&WriterBridge::Write, bridge_,
      path, blob_id, offset,
      base::Bind(&WebFileWriterImpl::DidWrite, AsWeakPtr()),
      base::Bind(&WebFileWriterImpl::DidFinish, AsWeakPtr())));
}

void WebFileWriterImpl::DoCancel() {
  RunOnMainThread(base::Bind(&WriterBridge::Cancel, bridge_,
      base::Bind(&WebFileWriterImpl::DidFinish, AsWeakPtr())));
}

void WebFileWriterImpl::RunOnMainThread(const base::Closure& closure) {
  if (main_thread_task_runner_->RunsTasksInCurrentSequence()) {
    DCHECK(!bridge_->waitable_event());
    closure.Run();
    return;
  }
  main_thread_task_runner_->PostTask(FROM_HERE, closure);
  if (bridge_->waitable_event())
    bridge_->WaitAndRun();
}

}  // namespace content
