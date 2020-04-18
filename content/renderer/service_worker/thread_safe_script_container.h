// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_
#define CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_

#include <map>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_installed_scripts_manager.h"

namespace content {

// ThreadSafeScriptContainer stores the scripts of a service worker for
// startup. This container is created for each service worker. The IO thread
// adds scripts to the container, and the worker thread takes the scripts.
//
// This class uses explicit synchronization because it needs to support
// synchronous importScripts() from the worker thread.
//
// This class is RefCounted because there is no ordering guarantee of lifetime
// of its owners, i.e. WebServiceWorkerInstalledScriptsManagerImpl and its
// Internal class. WebSWInstalledScriptsManagerImpl is destroyed earlier than
// Internal if the worker is terminated before all scripts are streamed, and
// Internal is destroyed earlier if all of scripts are received before finishing
// script evaluation.
class CONTENT_EXPORT ThreadSafeScriptContainer
    : public base::RefCountedThreadSafe<ThreadSafeScriptContainer> {
 public:
  using Data = blink::WebServiceWorkerInstalledScriptsManager::RawScriptData;

  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();
  ThreadSafeScriptContainer();

  enum class ScriptStatus {
    // The script data has been received.
    kReceived,
    // The script data has been received but it has already been taken.
    kTaken,
    // Receiving the script has failed.
    kFailed,
    // The script data has not been received yet.
    kPending
  };

  // Called on the IO thread.
  void AddOnIOThread(const GURL& url, std::unique_ptr<Data> data);

  // Called on the worker thread.
  ScriptStatus GetStatusOnWorkerThread(const GURL& url);

  // Removes the script. After calling this, ScriptStatus for the
  // script will be kPending.
  // Called on the worker thread.
  void ResetOnWorkerThread(const GURL& url);

  // Waits until the script is added. The thread is blocked until the script is
  // available or receiving the script fails. Returns false if an error happens
  // and the waiting script won't be available forever.
  // Called on the worker thread.
  bool WaitOnWorkerThread(const GURL& url);

  // Called on the worker thread.
  std::unique_ptr<Data> TakeOnWorkerThread(const GURL& url);

  // Called if no more data will be added.
  // Called on the IO thread.
  void OnAllDataAddedOnIOThread();

 private:
  friend class base::RefCountedThreadSafe<ThreadSafeScriptContainer>;
  ~ThreadSafeScriptContainer();

  // |lock_| protects |waiting_cv_|, |script_data_|, |waiting_url_| and
  // |are_all_data_added_|.
  base::Lock lock_;
  // |waiting_cv_| is signaled when a script whose url matches to |waiting_url|
  // is added, or OnAllDataAdded is called. The worker thread waits on this, and
  // the IO thread signals it.
  base::ConditionVariable waiting_cv_;
  std::map<GURL, std::unique_ptr<Data>> script_data_;
  GURL waiting_url_;
  bool are_all_data_added_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_
