/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_FILE_SYSTEM_CALLBACKS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_FILE_SYSTEM_CALLBACKS_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_file_error.h"
#include "third_party/blink/public/platform/web_file_system_entry.h"
#include "third_party/blink/public/platform/web_file_system_type.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_vector.h"

#if INSIDE_BLINK
#include <memory>
#endif

namespace blink {

class AsyncFileSystemCallbacks;
class WebFileWriter;
class WebString;
class WebURL;
class WebFileSystemCallbacksPrivate;
struct WebFileInfo;

class WebFileSystemCallbacks {
 public:
  ~WebFileSystemCallbacks() { Reset(); }
  WebFileSystemCallbacks() = default;
  WebFileSystemCallbacks(const WebFileSystemCallbacks& c) { Assign(c); }
  WebFileSystemCallbacks& operator=(const WebFileSystemCallbacks& c) {
    Assign(c);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Reset();
  BLINK_PLATFORM_EXPORT void Assign(const WebFileSystemCallbacks&);

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebFileSystemCallbacks(
      std::unique_ptr<AsyncFileSystemCallbacks>&&);
#endif

  // Callback for WebFileSystem's various operations that don't require
  // return values.
  BLINK_PLATFORM_EXPORT void DidSucceed();

  // Callback for WebFileSystem::ReadMetadata. Called with the file metadata
  // for the requested path.
  BLINK_PLATFORM_EXPORT void DidReadMetadata(const WebFileInfo&);

  // Callback for WebFileSystem::CreateSnapshot. The metadata also includes the
  // platform file path.
  BLINK_PLATFORM_EXPORT void DidCreateSnapshotFile(const WebFileInfo&);

  // Callback for WebFileSystem::ReadDirectory. Called with a vector of
  // file entries in the requested directory. This callback might be called
  // multiple times if the directory has many entries. |has_more| must be
  // true when there are more entries.
  BLINK_PLATFORM_EXPORT void DidReadDirectory(
      const WebVector<WebFileSystemEntry>&,
      bool has_more);

  // Callback for WebFileSystem::OpenFileSystem. Called with a name and
  // root URL for the FileSystem when the request is accepted.
  BLINK_PLATFORM_EXPORT void DidOpenFileSystem(const WebString& name,
                                               const WebURL& root_url);

  // Callback for WebFileSystem::ResolveURL. Called with a name, root URL and
  // file path for the FileSystem when the request is accepted. |is_directory|
  // must be true when an entry to be resolved is a directory.
  BLINK_PLATFORM_EXPORT void DidResolveURL(const WebString& name,
                                           const WebURL& root_url,
                                           WebFileSystemType,
                                           const WebString& file_path,
                                           bool is_directory);

  // Callback for WebFileSystem::CreateFileWriter. Called with an instance
  // of WebFileWriter and the target file length. The writer's ownership
  // is transferred to the callback.
  BLINK_PLATFORM_EXPORT void DidCreateFileWriter(WebFileWriter*,
                                                 long long length);

  // Called with an error code when a requested operation hasn't been
  // completed.
  BLINK_PLATFORM_EXPORT void DidFail(WebFileError);

  // Returns true if the caller expects to be blocked until the request
  // is fullfilled.
  BLINK_PLATFORM_EXPORT bool ShouldBlockUntilCompletion() const;

 private:
  WebPrivatePtr<WebFileSystemCallbacksPrivate> private_;
};

}  // namespace blink

#endif
