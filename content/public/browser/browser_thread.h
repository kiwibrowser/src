// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_H_
#define CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"

namespace content {

class BrowserThreadDelegate;
class BrowserThreadImpl;

// Use DCHECK_CURRENTLY_ON(BrowserThread::ID) to assert that a function can only
// be called on the named BrowserThread.
#define DCHECK_CURRENTLY_ON(thread_identifier)                      \
  (DCHECK(::content::BrowserThread::CurrentlyOn(thread_identifier)) \
   << ::content::BrowserThread::GetDCheckCurrentlyOnErrorMessage(   \
          thread_identifier))

///////////////////////////////////////////////////////////////////////////////
// BrowserThread
//
// Utility functions for threads that are known by a browser-wide
// name.  For example, there is one IO thread for the entire browser
// process, and various pieces of code find it useful to retrieve a
// pointer to the IO thread's message loop.
//
// Invoke a task by thread ID:
//
//   BrowserThread::PostTask(BrowserThread::IO, FROM_HERE, task);
//
// The return value is false if the task couldn't be posted because the target
// thread doesn't exist.  If this could lead to data loss, you need to check the
// result and restructure the code to ensure it doesn't occur.
//
// This class automatically handles the lifetime of different threads.
// It's always safe to call PostTask on any thread.  If it's not yet created,
// the task is deleted.  There are no race conditions.  If the thread that the
// task is posted to is guaranteed to outlive the current thread, then no locks
// are used.  You should never need to cache pointers to MessageLoops, since
// they're not thread safe.
class CONTENT_EXPORT BrowserThread {
 public:
  // An enumeration of the well-known threads.
  // NOTE: threads must be listed in the order of their life-time, with each
  // thread outliving every other thread below it.
  enum ID {
    // The main thread in the browser.
    UI,

    // This is the thread that processes non-blocking IO, i.e. IPC and network.
    // Blocking I/O should happen in TaskScheduler.
    IO,

    // NOTE: do not add new threads here. Instead you should just use
    // base::Create*TaskRunnerWithTraits.

    // This identifier does not represent a thread.  Instead it counts the
    // number of well-known threads.  Insert new well-known threads before this
    // identifier.
    ID_COUNT
  };

  // These are the same methods in message_loop.h, but are guaranteed to either
  // get posted to the MessageLoop if it's still alive, or be deleted otherwise.
  // They return true iff the thread existed and the task was posted.  Note that
  // even if the task is posted, there's no guarantee that it will run, since
  // the target thread may already have a Quit message in its queue.
  static bool PostTask(ID identifier,
                       const base::Location& from_here,
                       base::OnceClosure task);
  static bool PostDelayedTask(ID identifier,
                              const base::Location& from_here,
                              base::OnceClosure task,
                              base::TimeDelta delay);
  static bool PostNonNestableTask(ID identifier,
                                  const base::Location& from_here,
                                  base::OnceClosure task);
  static bool PostNonNestableDelayedTask(ID identifier,
                                         const base::Location& from_here,
                                         base::OnceClosure task,
                                         base::TimeDelta delay);

  static bool PostTaskAndReply(ID identifier,
                               const base::Location& from_here,
                               base::OnceClosure task,
                               base::OnceClosure reply);

  template <typename ReturnType, typename ReplyArgType>
  static bool PostTaskAndReplyWithResult(
      ID identifier,
      const base::Location& from_here,
      base::OnceCallback<ReturnType()> task,
      base::OnceCallback<void(ReplyArgType)> reply) {
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        GetTaskRunnerForThread(identifier);
    return base::PostTaskAndReplyWithResult(task_runner.get(), from_here,
                                            std::move(task), std::move(reply));
  }

  // Callback version of PostTaskAndReplyWithResult above.
  // Though RepeatingCallback is convertible to OnceCallback, we need this since
  // we cannot use template deduction and object conversion at once on the
  // overload resolution.
  // TODO(crbug.com/714018): Update all callers of the Callback version to use
  // OnceCallback.
  template <typename ReturnType, typename ReplyArgType>
  static bool PostTaskAndReplyWithResult(
      ID identifier,
      const base::Location& from_here,
      base::Callback<ReturnType()> task,
      base::Callback<void(ReplyArgType)> reply) {
    return PostTaskAndReplyWithResult(
        identifier, from_here,
        base::OnceCallback<ReturnType()>(std::move(task)),
        base::OnceCallback<void(ReplyArgType)>(std::move(reply)));
  }

  template <class T>
  static bool DeleteSoon(ID identifier,
                         const base::Location& from_here,
                         const T* object) {
    return GetTaskRunnerForThread(identifier)->DeleteSoon(from_here, object);
  }

  template <class T>
  static bool DeleteSoon(ID identifier,
                         const base::Location& from_here,
                         std::unique_ptr<T> object) {
    return DeleteSoon(identifier, from_here, object.release());
  }

  template <class T>
  static bool ReleaseSoon(ID identifier,
                          const base::Location& from_here,
                          const T* object) {
    return GetTaskRunnerForThread(identifier)->ReleaseSoon(from_here, object);
  }

  // For use with scheduling non-critical tasks for execution after startup.
  // The order or execution of tasks posted here is unspecified even when
  // posting to a SequencedTaskRunner and tasks are not guaranteed to be run
  // prior to browser shutdown.
  // When called after the browser startup is complete, will post |task|
  // to |task_runner| immediately.
  // Note: see related ContentBrowserClient::PostAfterStartupTask.
  static void PostAfterStartupTask(
      const base::Location& from_here,
      const scoped_refptr<base::TaskRunner>& task_runner,
      base::OnceClosure task);

  // Callable on any thread.  Returns whether the given well-known thread is
  // initialized.
  static bool IsThreadInitialized(ID identifier) WARN_UNUSED_RESULT;

  // Callable on any thread.  Returns whether you're currently on a particular
  // thread.  To DCHECK this, use the DCHECK_CURRENTLY_ON() macro above.
  static bool CurrentlyOn(ID identifier) WARN_UNUSED_RESULT;

  // If the current message loop is one of the known threads, returns true and
  // sets identifier to its ID.  Otherwise returns false.
  static bool GetCurrentThreadIdentifier(ID* identifier) WARN_UNUSED_RESULT;

  // Callers can hold on to a refcounted task runner beyond the lifetime
  // of the thread.
  static scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunnerForThread(
      ID identifier);

  // Sets the delegate for BrowserThread::IO.
  //
  // Only one delegate may be registered at a time. The delegate may be
  // unregistered by providing a nullptr pointer.
  //
  // The delegate can only be registered through this call before
  // BrowserThreadImpl(BrowserThread::IO) is created and unregistered after
  // it was destroyed and its underlying thread shutdown.
  static void SetIOThreadDelegate(BrowserThreadDelegate* delegate);

  // Use these templates in conjunction with RefCountedThreadSafe or scoped_ptr
  // when you want to ensure that an object is deleted on a specific thread.
  // This is needed when an object can hop between threads (i.e. UI -> IO ->
  // UI), and thread switching delays can mean that the final UI tasks executes
  // before the IO task's stack unwinds. This would lead to the object
  // destructing on the IO thread, which often is not what you want (i.e. to
  // unregister from NotificationService, to notify other objects on the
  // creating thread etc). Note: see base::OnTaskRunnerDeleter and
  // base::RefCountedDeleteOnSequence to bind to SequencedTaskRunner instead of
  // specific BrowserThreads.
  template<ID thread>
  struct DeleteOnThread {
    template<typename T>
    static void Destruct(const T* x) {
      if (CurrentlyOn(thread)) {
        delete x;
      } else {
        if (!DeleteSoon(thread, FROM_HERE, x)) {
#if defined(UNIT_TEST)
          // Only logged under unit testing because leaks at shutdown
          // are acceptable under normal circumstances.
          LOG(ERROR) << "DeleteSoon failed on thread " << thread;
#endif  // UNIT_TEST
        }
      }
    }
    template <typename T>
    inline void operator()(T* ptr) const {
      enum { type_must_be_complete = sizeof(T) };
      Destruct(ptr);
    }
  };

  // Sample usage with RefCountedThreadSafe:
  // class Foo
  //     : public base::RefCountedThreadSafe<
  //           Foo, BrowserThread::DeleteOnIOThread> {
  //
  // ...
  //  private:
  //   friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  //   friend class base::DeleteHelper<Foo>;
  //
  //   ~Foo();
  //
  // Sample usage with scoped_ptr:
  // std::unique_ptr<Foo, BrowserThread::DeleteOnIOThread> ptr;
  //
  // Note: see base::OnTaskRunnerDeleter and base::RefCountedDeleteOnSequence to
  // bind to SequencedTaskRunner instead of specific BrowserThreads.
  struct DeleteOnUIThread : public DeleteOnThread<UI> { };
  struct DeleteOnIOThread : public DeleteOnThread<IO> { };

  // Returns an appropriate error message for when DCHECK_CURRENTLY_ON() fails.
  static std::string GetDCheckCurrentlyOnErrorMessage(ID expected);

 private:
  friend class BrowserThreadImpl;

  BrowserThread() {}
  DISALLOW_COPY_AND_ASSIGN(BrowserThread);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_H_
