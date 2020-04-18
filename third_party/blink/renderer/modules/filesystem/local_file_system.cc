/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/filesystem/local_file_system.h"

#include <memory>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_file_system.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_client.h"
#include "third_party/blink/renderer/platform/async_file_system_callbacks.h"
#include "third_party/blink/renderer/platform/content_setting_callbacks.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

void ReportFailure(std::unique_ptr<AsyncFileSystemCallbacks> callbacks,
                   FileError::ErrorCode error) {
  callbacks->DidFail(error);
}

}  // namespace

class CallbackWrapper final
    : public GarbageCollectedFinalized<CallbackWrapper> {
 public:
  CallbackWrapper(std::unique_ptr<AsyncFileSystemCallbacks> c)
      : callbacks_(std::move(c)) {}
  virtual ~CallbackWrapper() = default;
  std::unique_ptr<AsyncFileSystemCallbacks> Release() {
    return std::move(callbacks_);
  }

  void Trace(blink::Visitor* visitor) {}

 private:
  std::unique_ptr<AsyncFileSystemCallbacks> callbacks_;
};

LocalFileSystem::~LocalFileSystem() = default;

void LocalFileSystem::ResolveURL(
    ExecutionContext* context,
    const KURL& file_system_url,
    std::unique_ptr<AsyncFileSystemCallbacks> callbacks) {
  CallbackWrapper* wrapper = new CallbackWrapper(std::move(callbacks));
  RequestFileSystemAccessInternal(
      context,
      WTF::Bind(&LocalFileSystem::ResolveURLInternal,
                WrapCrossThreadPersistent(this), WrapPersistent(context),
                file_system_url, WrapPersistent(wrapper)),
      WTF::Bind(&LocalFileSystem::FileSystemNotAllowedInternal,
                WrapCrossThreadPersistent(this), WrapPersistent(context),
                WrapPersistent(wrapper)));
}

void LocalFileSystem::RequestFileSystem(
    ExecutionContext* context,
    FileSystemType type,
    long long size,
    std::unique_ptr<AsyncFileSystemCallbacks> callbacks) {
  CallbackWrapper* wrapper = new CallbackWrapper(std::move(callbacks));
  RequestFileSystemAccessInternal(
      context,
      WTF::Bind(&LocalFileSystem::FileSystemAllowedInternal,
                WrapCrossThreadPersistent(this), WrapPersistent(context), type,
                WrapPersistent(wrapper)),
      WTF::Bind(&LocalFileSystem::FileSystemNotAllowedInternal,
                WrapCrossThreadPersistent(this), WrapPersistent(context),
                WrapPersistent(wrapper)));
}

WebFileSystem* LocalFileSystem::GetFileSystem() const {
  Platform* platform = Platform::Current();
  if (!platform)
    return nullptr;

  return platform->FileSystem();
}

void LocalFileSystem::RequestFileSystemAccessInternal(
    ExecutionContext* context,
    base::OnceClosure allowed,
    base::OnceClosure denied) {
  if (!context->IsDocument()) {
    if (!Client().RequestFileSystemAccessSync(context)) {
      std::move(denied).Run();
      return;
    }
    std::move(allowed).Run();
    return;
  }
  Client().RequestFileSystemAccessAsync(
      context,
      ContentSettingCallbacks::Create(std::move(allowed), std::move(denied)));
}

void LocalFileSystem::FileSystemNotAvailable(ExecutionContext* context,
                                             CallbackWrapper* callbacks) {
  context->GetTaskRunner(TaskType::kFileReading)
      ->PostTask(FROM_HERE,
                 WTF::Bind(&ReportFailure, WTF::Passed(callbacks->Release()),
                           FileError::kAbortErr));
}

void LocalFileSystem::FileSystemNotAllowedInternal(ExecutionContext* context,
                                                   CallbackWrapper* callbacks) {
  context->GetTaskRunner(TaskType::kFileReading)
      ->PostTask(FROM_HERE,
                 WTF::Bind(&ReportFailure, WTF::Passed(callbacks->Release()),
                           FileError::kAbortErr));
}

void LocalFileSystem::FileSystemAllowedInternal(ExecutionContext* context,
                                                FileSystemType type,
                                                CallbackWrapper* callbacks) {
  WebFileSystem* file_system = GetFileSystem();
  if (!file_system) {
    FileSystemNotAvailable(context, callbacks);
    return;
  }
  KURL storage_partition =
      KURL(NullURL(), context->GetSecurityOrigin()->ToString());
  file_system->OpenFileSystem(storage_partition,
                              static_cast<WebFileSystemType>(type),
                              callbacks->Release());
}

void LocalFileSystem::ResolveURLInternal(ExecutionContext* context,
                                         const KURL& file_system_url,
                                         CallbackWrapper* callbacks) {
  WebFileSystem* file_system = GetFileSystem();
  if (!file_system) {
    FileSystemNotAvailable(context, callbacks);
    return;
  }
  file_system->ResolveURL(file_system_url, callbacks->Release());
}

LocalFileSystem::LocalFileSystem(LocalFrame& frame,
                                 std::unique_ptr<FileSystemClient> client)
    : Supplement<LocalFrame>(frame), client_(std::move(client)) {
  DCHECK(client_);
}

LocalFileSystem::LocalFileSystem(WorkerClients& worker_clients,
                                 std::unique_ptr<FileSystemClient> client)
    : Supplement<WorkerClients>(worker_clients), client_(std::move(client)) {
  DCHECK(client_);
}

void LocalFileSystem::Trace(blink::Visitor* visitor) {
  Supplement<LocalFrame>::Trace(visitor);
  Supplement<WorkerClients>::Trace(visitor);
}

void LocalFileSystem::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  Supplement<LocalFrame>::TraceWrappers(visitor);
  Supplement<WorkerClients>::TraceWrappers(visitor);
}

const char LocalFileSystem::kSupplementName[] = "LocalFileSystem";

LocalFileSystem* LocalFileSystem::From(ExecutionContext& context) {
  if (context.IsDocument()) {
    LocalFileSystem* file_system =
        Supplement<LocalFrame>::From<LocalFileSystem>(
            ToDocument(context).GetFrame());
    DCHECK(file_system);
    return file_system;
  }

  WorkerClients* clients = ToWorkerGlobalScope(context).Clients();
  DCHECK(clients);
  LocalFileSystem* file_system =
      Supplement<WorkerClients>::From<LocalFileSystem>(clients);
  DCHECK(file_system);
  return file_system;
}

void ProvideLocalFileSystemTo(LocalFrame& frame,
                              std::unique_ptr<FileSystemClient> client) {
  frame.ProvideSupplement(new LocalFileSystem(frame, std::move(client)));
}

void ProvideLocalFileSystemToWorker(WorkerClients* worker_clients,
                                    std::unique_ptr<FileSystemClient> client) {
  Supplement<WorkerClients>::ProvideTo(
      *worker_clients, new LocalFileSystem(*worker_clients, std::move(client)));
}

}  // namespace blink
