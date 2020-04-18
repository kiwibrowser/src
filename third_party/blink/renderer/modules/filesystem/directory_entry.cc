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

#include "third_party/blink/renderer/modules/filesystem/directory_entry.h"

#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/modules/filesystem/directory_reader.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_callbacks.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_flags.h"

namespace blink {

DirectoryEntry::DirectoryEntry(DOMFileSystemBase* file_system,
                               const String& full_path)
    : Entry(file_system, full_path) {}

DirectoryReader* DirectoryEntry::createReader() {
  return DirectoryReader::Create(file_system_, full_path_);
}

void DirectoryEntry::getFile(const String& path,
                             const FileSystemFlags& options,
                             V8EntryCallback* success_callback,
                             V8ErrorCallback* error_callback) {
  file_system_->GetFile(
      this, path, options,
      EntryCallbacks::OnDidGetEntryV8Impl::Create(success_callback),
      ScriptErrorCallback::Wrap(error_callback));
}

void DirectoryEntry::getDirectory(const String& path,
                                  const FileSystemFlags& options,
                                  V8EntryCallback* success_callback,
                                  V8ErrorCallback* error_callback) {
  file_system_->GetDirectory(
      this, path, options,
      EntryCallbacks::OnDidGetEntryV8Impl::Create(success_callback),
      ScriptErrorCallback::Wrap(error_callback));
}

void DirectoryEntry::removeRecursively(V8VoidCallback* success_callback,
                                       V8ErrorCallback* error_callback) const {
  file_system_->RemoveRecursively(
      this, VoidCallbacks::OnDidSucceedV8Impl::Create(success_callback),
      ScriptErrorCallback::Wrap(error_callback));
}

void DirectoryEntry::Trace(blink::Visitor* visitor) {
  Entry::Trace(visitor);
}

}  // namespace blink
