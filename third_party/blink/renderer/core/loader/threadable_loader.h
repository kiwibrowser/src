/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_THREADABLE_LOADER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_THREADABLE_LOADER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ResourceRequest;
class ExecutionContext;
class ThreadableLoaderClient;

struct ThreadableLoaderOptions {
  DISALLOW_NEW();
  ThreadableLoaderOptions() : timeout_milliseconds(0) {}

  // When adding members, CrossThreadThreadableLoaderOptionsData should
  // be updated.

  unsigned long timeout_milliseconds;
};

// Encode AtomicString as String to cross threads.
struct CrossThreadThreadableLoaderOptionsData {
  STACK_ALLOCATED();
  explicit CrossThreadThreadableLoaderOptionsData(
      const ThreadableLoaderOptions& options)
      : timeout_milliseconds(options.timeout_milliseconds) {}

  operator ThreadableLoaderOptions() const {
    ThreadableLoaderOptions options;
    options.timeout_milliseconds = timeout_milliseconds;
    return options;
  }

  unsigned long timeout_milliseconds;
};

template <>
struct CrossThreadCopier<ThreadableLoaderOptions> {
  typedef CrossThreadThreadableLoaderOptionsData Type;
  static Type Copy(const ThreadableLoaderOptions& options) {
    return CrossThreadThreadableLoaderOptionsData(options);
  }
};

// Useful for doing loader operations from any thread (not threadsafe, just able
// to run on threads other than the main thread).
//
// Arguments common to both loadResourceSynchronously() and create():
//
// - ThreadableLoaderOptions argument configures this ThreadableLoader's
//   behavior.
//
// - ResourceLoaderOptions argument will be passed to the FetchParameters
//   that this ThreadableLoader creates. It can be altered e.g. when
//   redirect happens.
class CORE_EXPORT ThreadableLoader
    : public GarbageCollectedFinalized<ThreadableLoader> {
 public:
  static void LoadResourceSynchronously(ExecutionContext&,
                                        const ResourceRequest&,
                                        ThreadableLoaderClient&,
                                        const ThreadableLoaderOptions&,
                                        const ResourceLoaderOptions&);

  // This method never returns nullptr.
  //
  // This method must always be followed by start() call.
  // ThreadableLoaderClient methods are never called before start() call.
  //
  // The async loading feature is separated into the create() method and
  // and the start() method in order to:
  // - reduce work done in a constructor
  // - not to ask the users to handle failures in the constructor and other
  //   async failures separately
  //
  // Loading completes when one of the following methods are called:
  // - didFinishLoading()
  // - didFail()
  // - didFailAccessControlCheck()
  // - didFailRedirectCheck()
  // After any of these methods is called, the loader won't call any of the
  // ThreadableLoaderClient methods.
  //
  // A user must guarantee that the loading completes before the attached
  // client gets invalid. Also, a user must guarantee that the loading
  // completes before the ThreadableLoader is destructed.
  //
  // When ThreadableLoader::cancel() is called,
  // ThreadableLoaderClient::didFail() is called with a ResourceError
  // with isCancellation() returning true, if any of didFinishLoading()
  // or didFail.*() methods have not been called yet. (didFail() may be
  // called with a ResourceError with isCancellation() returning true
  // also for cancellation happened inside the loader.)
  //
  // ThreadableLoaderClient methods may call cancel().
  static ThreadableLoader* Create(ExecutionContext&,
                                  ThreadableLoaderClient*,
                                  const ThreadableLoaderOptions&,
                                  const ResourceLoaderOptions&);

  // The methods on the ThreadableLoaderClient passed on create() call
  // may be called synchronous to start() call.
  virtual void Start(const ResourceRequest&) = 0;

  // A ThreadableLoader may have a timeout specified. It is possible, in some
  // cases, for the timeout to be overridden after the request is sent (for
  // example, XMLHttpRequests may override their timeout setting after sending).
  //
  // Set a new timeout relative to the time the request started, in
  // milliseconds.
  virtual void OverrideTimeout(unsigned long timeout_milliseconds) = 0;

  // Cancel the request.
  virtual void Cancel() = 0;

  // Detach the loader from the request. This function is for "keepalive"
  // requests. No notification will be sent to the client, but the request
  // will be processed.
  virtual void Detach() = 0;

  virtual ~ThreadableLoader() = default;

  virtual void Trace(blink::Visitor* visitor) {}

 protected:
  ThreadableLoader() = default;

  DISALLOW_COPY_AND_ASSIGN(ThreadableLoader);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_THREADABLE_LOADER_H_
