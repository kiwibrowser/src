// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FILEAPI_WEBFILESYSTEM_IMPL_H_
#define CONTENT_RENDERER_FILEAPI_WEBFILESYSTEM_IMPL_H_

#include <map>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "content/public/renderer/worker_thread.h"
#include "third_party/blink/public/platform/web_file_system.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace blink {
class WebURL;
class WebFileWriterClient;
}

namespace content {

class WebFileSystemImpl : public blink::WebFileSystem,
                          public WorkerThread::Observer {
 public:
  class WaitableCallbackResults;

  // Returns thread-specific instance.  If non-null |main_thread_loop|
  // is given and no thread-specific instance has been created it may
  // create a new instance.
  static WebFileSystemImpl* ThreadSpecificInstance(
      const scoped_refptr<base::SingleThreadTaskRunner>&
          main_thread_task_runner);

  // Deletes thread-specific instance (if exists). For workers it deletes
  // itself in WillStopCurrentWorkerThread(), but for an instance created on the
  // main thread this method must be called.
  static void DeleteThreadSpecificInstance();

  explicit WebFileSystemImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>&
          main_thread_task_runner);
  ~WebFileSystemImpl() override;

  // WorkerThread::Observer implementation.
  void WillStopCurrentWorkerThread() override;

  // WebFileSystem implementation.
  void OpenFileSystem(const blink::WebURL& storage_partition,
                      const blink::WebFileSystemType type,
                      blink::WebFileSystemCallbacks) override;
  void ResolveURL(const blink::WebURL& filesystem_url,
                  blink::WebFileSystemCallbacks) override;
  void Move(const blink::WebURL& src_path,
            const blink::WebURL& dest_path,
            blink::WebFileSystemCallbacks) override;
  void Copy(const blink::WebURL& src_path,
            const blink::WebURL& dest_path,
            blink::WebFileSystemCallbacks) override;
  void Remove(const blink::WebURL& path,
              blink::WebFileSystemCallbacks) override;
  void RemoveRecursively(const blink::WebURL& path,
                         blink::WebFileSystemCallbacks) override;
  void ReadMetadata(const blink::WebURL& path,
                    blink::WebFileSystemCallbacks) override;
  void CreateFile(const blink::WebURL& path,
                  bool exclusive,
                  blink::WebFileSystemCallbacks) override;
  void CreateDirectory(const blink::WebURL& path,
                       bool exclusive,
                       blink::WebFileSystemCallbacks) override;
  void FileExists(const blink::WebURL& path,
                  blink::WebFileSystemCallbacks) override;
  void DirectoryExists(const blink::WebURL& path,
                       blink::WebFileSystemCallbacks) override;
  int ReadDirectory(const blink::WebURL& path,
                    blink::WebFileSystemCallbacks) override;
  void CreateFileWriter(const blink::WebURL& path,
                        blink::WebFileWriterClient*,
                        blink::WebFileSystemCallbacks) override;
  void CreateSnapshotFileAndReadMetadata(
      const blink::WebURL& path,
      blink::WebFileSystemCallbacks) override;
  bool WaitForAdditionalResult(int callbacksId) override;

  int RegisterCallbacks(const blink::WebFileSystemCallbacks& callbacks);
  blink::WebFileSystemCallbacks GetCallbacks(int callbacks_id);
  void UnregisterCallbacks(int callbacks_id);

 private:
  typedef std::map<int, blink::WebFileSystemCallbacks> CallbacksMap;
  typedef std::map<int, scoped_refptr<WaitableCallbackResults> >
      WaitableCallbackResultsMap;

  WaitableCallbackResults* MaybeCreateWaitableResults(
      const blink::WebFileSystemCallbacks& callbacks, int callbacks_id);

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  CallbacksMap callbacks_;
  int next_callbacks_id_;
  WaitableCallbackResultsMap waitable_results_;

  // Thread-affine per use of TLS in impl.
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(WebFileSystemImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_FILEAPI_WEBFILESYSTEM_IMPL_H_
