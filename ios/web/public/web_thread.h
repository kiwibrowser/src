// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_THREAD_H_
#define IOS_WEB_PUBLIC_WEB_THREAD_H_

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"

namespace base {
class MessageLoop;
}

namespace base {
class Location;
}

namespace web {

class WebThreadDelegate;

// Use DCHECK_CURRENTLY_ON(WebThread::ID) to assert that a function can only be
// called on the named WebThread.
#define DCHECK_CURRENTLY_ON(thread_identifier)              \
  (DCHECK(::web::WebThread::CurrentlyOn(thread_identifier)) \
   << ::web::WebThread::GetDCheckCurrentlyOnErrorMessage(thread_identifier))

///////////////////////////////////////////////////////////////////////////////
// WebThread
//
// Utility functions for threads that are known by name. For example, there is
// one IO thread for the entire process, and various pieces of code find it
// useful to retrieve a pointer to the IO thread's message loop.
//
// Invoke a task by thread ID:
//
//   WebThread::PostTask(WebThread::IO, FROM_HERE, task);
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
class WebThread {
 public:
  // An enumeration of the well-known threads.
  // NOTE: threads must be listed in the order of their life-time, with each
  // thread outliving every other thread below it.
  enum ID {
    // The main thread in the browser.
    UI,

    // This is the thread that processes non-blocking IO, i.e. IPC and network.
    // Blocking IO should happen in TaskScheduler.
    IO,

    // NOTE: do not add new threads here that are only used by a small number of
    // files. Instead you should just use a Thread class and pass its
    // SingleThreadTaskRunner around. Named threads there are only for threads
    // that are used in many places.

    // This identifier does not represent a thread.  Instead it counts the
    // number of well-known threads.  Insert new well-known threads before this
    // identifier.
    ID_COUNT
  };

  // These are the same methods as in message_loop.h, but are guaranteed to
  // either get posted to the MessageLoop if it's still alive, or be deleted
  // otherwise.
  // They return true iff the thread existed and the task was posted.
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

  template <class T>
  static bool DeleteSoon(ID identifier,
                         const base::Location& from_here,
                         const T* object) {
    return GetTaskRunnerForThread(identifier)->DeleteSoon(from_here, object);
  }

  // Callable on any thread.  Returns whether the given well-known thread is
  // initialized.
  static bool IsThreadInitialized(ID identifier) WARN_UNUSED_RESULT;

  // Callable on any thread.  Returns whether execution is currently on the
  // given thread.  To DCHECK this, use the DCHECK_CURRENTLY_ON() macro above.
  static bool CurrentlyOn(ID identifier) WARN_UNUSED_RESULT;

  // If the current message loop is one of the known threads, returns true and
  // sets identifier to its ID.
  static bool GetCurrentThreadIdentifier(ID* identifier) WARN_UNUSED_RESULT;

  // Callers can hold on to a refcounted SingleThreadTaskRunner beyond the
  // lifetime of the thread.
  static scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunnerForThread(
      ID identifier);

  // Sets the delegate for the specified WebThread.
  //
  // Only one delegate may be registered at a time. Delegates may be
  // unregistered by providing a nullptr pointer.
  //
  // If the caller unregisters a delegate before CleanUp has been
  // called, it must perform its own locking to ensure the delegate is
  // not deleted while unregistering.
  static void SetDelegate(ID identifier, WebThreadDelegate* delegate);

  // Returns an appropriate error message for when DCHECK_CURRENTLY_ON() fails.
  static std::string GetDCheckCurrentlyOnErrorMessage(ID expected);

  // Use these templates in conjunction with RefCountedThreadSafe or
  // std::unique_ptr when you want to ensure that an object is deleted on a
  // specific thread. This is needed when an object can hop between threads
  // (i.e. IO -> UI -> IO), and thread switching delays can mean that the final
  // IO tasks executes before the UI task's stack unwinds. This would lead to
  // the object destructing on the UI thread, which often is not what you want
  // (i.e. to unregister from NotificationService, to notify other objects on
  // the creating thread etc).
  template <ID thread>
  struct DeleteOnThread {
    template <typename T>
    static void Destruct(const T* x) {
      if (CurrentlyOn(thread)) {
        delete x;
      } else {
        if (!DeleteSoon(thread, FROM_HERE, x)) {
          // Leaks at shutdown are acceptable under normal circumstances,
          // do not report.
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
  //           Foo, web::WebThread::DeleteOnIOThread> {
  //
  // ...
  //  private:
  //   friend struct web::WebThread::DeleteOnThread<web::WebThread::IO>;
  //   friend class base::DeleteHelper<Foo>;
  //
  //   ~Foo();
  //
  // Sample usage with std::unique_ptr:
  // std::unique_ptr<Foo, web::WebThread::DeleteOnIOThread> ptr;
  struct DeleteOnUIThread : public DeleteOnThread<UI> {};
  struct DeleteOnIOThread : public DeleteOnThread<IO> {};

 private:
  friend class WebThreadImpl;

  WebThread() {}
  DISALLOW_COPY_AND_ASSIGN(WebThread);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_THREAD_H_
