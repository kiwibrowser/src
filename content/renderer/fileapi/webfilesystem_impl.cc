// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fileapi/webfilesystem_impl.h"

#include <stddef.h>
#include <string>
#include <tuple>
#include <vector>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "content/common/fileapi/file_system_messages.h"
#include "content/renderer/file_info_util.h"
#include "content/renderer/fileapi/file_system_dispatcher.h"
#include "content/renderer/fileapi/webfilewriter_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "storage/common/fileapi/file_system_util.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/web_file_info.h"
#include "third_party/blink/public/platform/web_file_system_callbacks.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

using blink::WebFileInfo;
using blink::WebFileSystemCallbacks;
using blink::WebFileSystemEntry;
using blink::WebString;
using blink::WebURL;
using blink::WebVector;

namespace content {

class WebFileSystemImpl::WaitableCallbackResults
    : public base::RefCountedThreadSafe<WaitableCallbackResults> {
 public:
  WaitableCallbackResults()
      : results_available_event_(
            base::WaitableEvent::ResetPolicy::MANUAL,
            base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  void AddResultsAndSignal(const base::Closure& results_closure) {
    base::AutoLock lock(lock_);
    results_closures_.push_back(results_closure);
    results_available_event_.Signal();
  }

  void WaitAndRun() {
    results_available_event_.Wait();
    Run();
  }

  void Run() {
    std::vector<base::Closure> closures;
    {
      base::AutoLock lock(lock_);
      results_closures_.swap(closures);
      results_available_event_.Reset();
    }
    for (size_t i = 0; i < closures.size(); ++i)
      closures[i].Run();
  }

 private:
  friend class base::RefCountedThreadSafe<WaitableCallbackResults>;

  ~WaitableCallbackResults() {}

  base::Lock lock_;
  base::WaitableEvent results_available_event_;
  std::vector<base::Closure> results_closures_;
  DISALLOW_COPY_AND_ASSIGN(WaitableCallbackResults);
};

namespace {

typedef WebFileSystemImpl::WaitableCallbackResults WaitableCallbackResults;

base::LazyInstance<base::ThreadLocalPointer<WebFileSystemImpl>>::Leaky
    g_webfilesystem_tls = LAZY_INSTANCE_INITIALIZER;

void DidReceiveSnapshotFile(int request_id) {
  if (ChildThreadImpl::current())
    ChildThreadImpl::current()->Send(
        new FileSystemHostMsg_DidReceiveSnapshotFile(request_id));
}

template <typename Method, typename Params>
void CallDispatcherOnMainThread(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    Method method, const Params& params,
    WaitableCallbackResults* waitable_results) {
  if (!main_thread_task_runner->BelongsToCurrentThread()) {
    main_thread_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&CallDispatcherOnMainThread<Method, Params>,
                       main_thread_task_runner, method, params, nullptr));
    if (!waitable_results)
      return;
    waitable_results->WaitAndRun();
  }
  if (!RenderThreadImpl::current() ||
      !RenderThreadImpl::current()->file_system_dispatcher())
    return;

  DCHECK(!waitable_results);
  DispatchToMethod(RenderThreadImpl::current()->file_system_dispatcher(),
                   method, params);
}

enum CallbacksUnregisterMode {
  UNREGISTER_CALLBACKS,
  DO_NOT_UNREGISTER_CALLBACKS,
};

// Bridging functions that convert the arguments into Blink objects
// (e.g. WebFileInfo, WebString, WebVector<WebFileSystemEntry>)
// and call WebFileSystemCallbacks's methods.
// These are called by RunCallbacks after crossing threads to ensure
// thread safety, because the Blink objects cannot be passed across
// threads by base::Bind().
void DidSucceed(WebFileSystemCallbacks* callbacks) {
  callbacks->DidSucceed();
}

void DidReadMetadata(const base::File::Info& file_info,
                     WebFileSystemCallbacks* callbacks) {
  WebFileInfo web_file_info;
  FileInfoToWebFileInfo(file_info, &web_file_info);
  callbacks->DidReadMetadata(web_file_info);
}

void DidReadDirectory(
    const std::vector<filesystem::mojom::DirectoryEntry>& entries,
    bool has_more,
    WebFileSystemCallbacks* callbacks) {
  WebVector<WebFileSystemEntry> file_system_entries(entries.size());
  for (size_t i = 0; i < entries.size(); ++i) {
    file_system_entries[i].name =
        blink::FilePathToWebString(base::FilePath(entries[i].name));
    file_system_entries[i].is_directory =
        entries[i].type == filesystem::mojom::FsFileType::DIRECTORY;
  }
  callbacks->DidReadDirectory(file_system_entries, has_more);
}

void DidOpenFileSystem(const std::string& name,
                       const GURL& root,
                       WebFileSystemCallbacks* callbacks) {
  callbacks->DidOpenFileSystem(blink::WebString::FromUTF8(name), root);
}

void DidResolveURL(const std::string& name,
                   const GURL& root_url,
                   storage::FileSystemType mount_type,
                   const base::FilePath& file_path,
                   bool is_directory,
                   WebFileSystemCallbacks* callbacks) {
  callbacks->DidResolveURL(blink::WebString::FromUTF8(name), root_url,
                           static_cast<blink::WebFileSystemType>(mount_type),
                           blink::FilePathToWebString(file_path), is_directory);
}

void DidFail(base::File::Error error, WebFileSystemCallbacks* callbacks) {
  callbacks->DidFail(storage::FileErrorToWebFileError(error));
}

// Run WebFileSystemCallbacks's |method| with |params|.
void RunCallbacks(
    int callbacks_id,
    const base::Callback<void(WebFileSystemCallbacks*)>& callback,
    CallbacksUnregisterMode callbacks_unregister_mode) {
  WebFileSystemImpl* filesystem =
      WebFileSystemImpl::ThreadSpecificInstance(nullptr);
  if (!filesystem)
    return;
  WebFileSystemCallbacks callbacks = filesystem->GetCallbacks(callbacks_id);
  if (callbacks_unregister_mode == UNREGISTER_CALLBACKS)
    filesystem->UnregisterCallbacks(callbacks_id);
  callback.Run(&callbacks);
}

void DispatchResultsClosure(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const base::Closure& results_closure) {
  if (task_runner->BelongsToCurrentThread()) {
    results_closure.Run();
    return;
  }

  if (waitable_results) {
    // If someone is waiting, this should result in running the closure.
    waitable_results->AddResultsAndSignal(results_closure);
    // In case no one is waiting, post a task to run the closure.
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&WaitableCallbackResults::Run,
                                  base::WrapRefCounted(waitable_results)));
    return;
  }
  task_runner->PostTask(FROM_HERE, results_closure);
}

void CallbackFileSystemCallbacks(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const base::Callback<void(WebFileSystemCallbacks*)>& callback,
    CallbacksUnregisterMode callbacksunregister_mode) {
  DispatchResultsClosure(task_runner, callbacks_id, waitable_results,
                         base::Bind(&RunCallbacks, callbacks_id, callback,
                                    callbacksunregister_mode));
}

//-----------------------------------------------------------------------------
// Callback adapters. Callbacks must be called on the original calling thread,
// so these callback adapters relay back the results to the calling thread
// if necessary.

void OpenFileSystemCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const std::string& name,
    const GURL& root) {
  CallbackFileSystemCallbacks(task_runner, callbacks_id, waitable_results,
                              base::Bind(&DidOpenFileSystem, name, root),
                              UNREGISTER_CALLBACKS);
}

void ResolveURLCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const storage::FileSystemInfo& info,
    const base::FilePath& file_path,
    bool is_directory) {
  base::FilePath normalized_path(
      storage::VirtualPath::GetNormalizedFilePath(file_path));
  CallbackFileSystemCallbacks(
      task_runner, callbacks_id, waitable_results,
      base::Bind(&DidResolveURL, info.name, info.root_url, info.mount_type,
                 normalized_path, is_directory),
      UNREGISTER_CALLBACKS);
}

void StatusCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    base::File::Error error) {
  if (error == base::File::FILE_OK) {
    CallbackFileSystemCallbacks(task_runner, callbacks_id, waitable_results,
                                base::Bind(&DidSucceed), UNREGISTER_CALLBACKS);
  } else {
    CallbackFileSystemCallbacks(task_runner, callbacks_id, waitable_results,
                                base::Bind(&DidFail, error),
                                UNREGISTER_CALLBACKS);
  }
}

void ReadMetadataCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const base::File::Info& file_info) {
  CallbackFileSystemCallbacks(task_runner, callbacks_id, waitable_results,
                              base::Bind(&DidReadMetadata, file_info),
                              UNREGISTER_CALLBACKS);
}

void ReadDirectoryCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const std::vector<filesystem::mojom::DirectoryEntry>& entries,
    bool has_more) {
  CallbackFileSystemCallbacks(
      task_runner, callbacks_id, waitable_results,
      base::Bind(&DidReadDirectory, entries, has_more),
      has_more ? DO_NOT_UNREGISTER_CALLBACKS : UNREGISTER_CALLBACKS);
}

void DidCreateFileWriter(
    int callbacks_id,
    const GURL& path,
    blink::WebFileWriterClient* client,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const base::File::Info& file_info) {
  WebFileSystemImpl* filesystem =
      WebFileSystemImpl::ThreadSpecificInstance(nullptr);
  if (!filesystem)
    return;

  WebFileSystemCallbacks callbacks = filesystem->GetCallbacks(callbacks_id);
  filesystem->UnregisterCallbacks(callbacks_id);

  if (file_info.is_directory || file_info.size < 0) {
    callbacks.DidFail(blink::kWebFileErrorInvalidState);
    return;
  }
  WebFileWriterImpl::Type type = callbacks.ShouldBlockUntilCompletion()
                                     ? WebFileWriterImpl::TYPE_SYNC
                                     : WebFileWriterImpl::TYPE_ASYNC;
  callbacks.DidCreateFileWriter(
      new WebFileWriterImpl(path, client, type, main_thread_task_runner),
      file_info.size);
}

void CreateFileWriterCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const GURL& path,
    blink::WebFileWriterClient* client,
    const base::File::Info& file_info) {
  DispatchResultsClosure(
      task_runner, callbacks_id, waitable_results,
      base::Bind(&DidCreateFileWriter, callbacks_id, path, client,
                 main_thread_task_runner, file_info));
}

void DidCreateSnapshotFile(
    int callbacks_id,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    int request_id) {
  WebFileSystemImpl* filesystem =
      WebFileSystemImpl::ThreadSpecificInstance(nullptr);
  if (!filesystem)
    return;

  WebFileSystemCallbacks callbacks = filesystem->GetCallbacks(callbacks_id);
  filesystem->UnregisterCallbacks(callbacks_id);

  WebFileInfo web_file_info;
  FileInfoToWebFileInfo(file_info, &web_file_info);
  web_file_info.platform_path = blink::FilePathToWebString(platform_path);
  callbacks.DidCreateSnapshotFile(web_file_info);

  // TODO(michaeln,kinuko): Use ThreadSafeSender when Blob becomes
  // non-bridge model.
  main_thread_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&DidReceiveSnapshotFile, request_id));
}

void CreateSnapshotFileCallbackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    int callbacks_id,
    WaitableCallbackResults* waitable_results,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    int request_id) {
  DispatchResultsClosure(
      task_runner, callbacks_id, waitable_results,
      base::Bind(&DidCreateSnapshotFile, callbacks_id, main_thread_task_runner,
                 file_info, platform_path, request_id));
}

}  // namespace

//-----------------------------------------------------------------------------
// WebFileSystemImpl

WebFileSystemImpl* WebFileSystemImpl::ThreadSpecificInstance(
    const scoped_refptr<base::SingleThreadTaskRunner>&
        main_thread_task_runner) {
  if (g_webfilesystem_tls.Pointer()->Get() || !main_thread_task_runner.get())
    return g_webfilesystem_tls.Pointer()->Get();
  WebFileSystemImpl* filesystem =
      new WebFileSystemImpl(main_thread_task_runner);
  if (WorkerThread::GetCurrentId())
    WorkerThread::AddObserver(filesystem);
  return filesystem;
}

void WebFileSystemImpl::DeleteThreadSpecificInstance() {
  DCHECK(!WorkerThread::GetCurrentId());
  if (g_webfilesystem_tls.Pointer()->Get())
    delete g_webfilesystem_tls.Pointer()->Get();
}

WebFileSystemImpl::WebFileSystemImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread_task_runner)
    : main_thread_task_runner_(main_thread_task_runner),
      next_callbacks_id_(1) {
  g_webfilesystem_tls.Pointer()->Set(this);
}

WebFileSystemImpl::~WebFileSystemImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  g_webfilesystem_tls.Pointer()->Set(nullptr);
}

void WebFileSystemImpl::WillStopCurrentWorkerThread() {
  delete this;
}

void WebFileSystemImpl::OpenFileSystem(const blink::WebURL& storage_partition,
                                       blink::WebFileSystemType type,
                                       WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::OpenFileSystem,
      std::make_tuple(
          GURL(storage_partition), static_cast<storage::FileSystemType>(type),
          base::Bind(&OpenFileSystemCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results)),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::ResolveURL(const blink::WebURL& filesystem_url,
                                   WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::ResolveURL,
      std::make_tuple(
          GURL(filesystem_url),
          base::Bind(&ResolveURLCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results)),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::Move(const blink::WebURL& src_path,
                             const blink::WebURL& dest_path,
                             WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Move,
      std::make_tuple(
          GURL(src_path), GURL(dest_path),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::Copy(const blink::WebURL& src_path,
                             const blink::WebURL& dest_path,
                             WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Copy,
      std::make_tuple(
          GURL(src_path), GURL(dest_path),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::Remove(const blink::WebURL& path,
                               WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Remove,
      std::make_tuple(
          GURL(path), false /* recursive */,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::RemoveRecursively(const blink::WebURL& path,
                                          WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Remove,
      std::make_tuple(
          GURL(path), true /* recursive */,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::ReadMetadata(const blink::WebURL& path,
                                     WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::ReadMetadata,
      std::make_tuple(
          GURL(path),
          base::Bind(&ReadMetadataCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results)),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::CreateFile(const blink::WebURL& path,
                                   bool exclusive,
                                   WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::CreateFile,
      std::make_tuple(
          GURL(path), exclusive,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::CreateDirectory(const blink::WebURL& path,
                                        bool exclusive,
                                        WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::CreateDirectory,
      std::make_tuple(
          GURL(path), exclusive, false /* recursive */,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::FileExists(const blink::WebURL& path,
                                   WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Exists,
      std::make_tuple(
          GURL(path), false /* directory */,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::DirectoryExists(const blink::WebURL& path,
                                        WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::Exists,
      std::make_tuple(
          GURL(path), true /* directory */,
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

int WebFileSystemImpl::ReadDirectory(const blink::WebURL& path,
                                     WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::ReadDirectory,
      std::make_tuple(
          GURL(path),
          base::Bind(&ReadDirectoryCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results)),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
  return callbacks_id;
}

void WebFileSystemImpl::CreateFileWriter(const WebURL& path,
                                         blink::WebFileWriterClient* client,
                                         WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::ReadMetadata,
      std::make_tuple(
          GURL(path),
          base::Bind(&CreateFileWriterCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results),
                     main_thread_task_runner_, GURL(path), client),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

void WebFileSystemImpl::CreateSnapshotFileAndReadMetadata(
    const blink::WebURL& path,
    WebFileSystemCallbacks callbacks) {
  int callbacks_id = RegisterCallbacks(callbacks);
  scoped_refptr<WaitableCallbackResults> waitable_results =
      MaybeCreateWaitableResults(callbacks, callbacks_id);
  CallDispatcherOnMainThread(
      main_thread_task_runner_, &FileSystemDispatcher::CreateSnapshotFile,
      std::make_tuple(
          GURL(path),
          base::Bind(&CreateSnapshotFileCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results),
                     main_thread_task_runner_),
          base::Bind(&StatusCallbackAdapter,
                     base::ThreadTaskRunnerHandle::Get(), callbacks_id,
                     base::RetainedRef(waitable_results))),
      waitable_results.get());
}

bool WebFileSystemImpl::WaitForAdditionalResult(int callbacksId) {
  WaitableCallbackResultsMap::iterator found =
      waitable_results_.find(callbacksId);
  if (found == waitable_results_.end())
    return false;

  found->second->WaitAndRun();
  return true;
}

int WebFileSystemImpl::RegisterCallbacks(
    const WebFileSystemCallbacks& callbacks) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  int id = next_callbacks_id_++;
  callbacks_[id] = callbacks;
  return id;
}

WebFileSystemCallbacks WebFileSystemImpl::GetCallbacks(int callbacks_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CallbacksMap::iterator found = callbacks_.find(callbacks_id);
  DCHECK(found != callbacks_.end());
  return found->second;
}

void WebFileSystemImpl::UnregisterCallbacks(int callbacks_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CallbacksMap::iterator found = callbacks_.find(callbacks_id);
  DCHECK(found != callbacks_.end());
  callbacks_.erase(found);

  waitable_results_.erase(callbacks_id);
}

WaitableCallbackResults* WebFileSystemImpl::MaybeCreateWaitableResults(
    const WebFileSystemCallbacks& callbacks, int callbacks_id) {
  if (!callbacks.ShouldBlockUntilCompletion())
    return nullptr;
  WaitableCallbackResults* results = new WaitableCallbackResults();
  waitable_results_[callbacks_id] = results;
  return results;
}

}  // namespace content
