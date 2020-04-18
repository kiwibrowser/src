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

#include "third_party/blink/renderer/modules/filesystem/directory_entry_sync.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/modules/filesystem/directory_reader_sync.h"
#include "third_party/blink/renderer/modules/filesystem/entry.h"
#include "third_party/blink/renderer/modules/filesystem/file_entry_sync.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_flags.h"
#include "third_party/blink/renderer/modules/filesystem/sync_callback_helper.h"

namespace blink {

DirectoryEntrySync::DirectoryEntrySync(DOMFileSystemBase* file_system,
                                       const String& full_path)
    : EntrySync(file_system, full_path) {}

DirectoryReaderSync* DirectoryEntrySync::createReader() {
  return DirectoryReaderSync::Create(file_system_, full_path_);
}

FileEntrySync* DirectoryEntrySync::getFile(const String& path,
                                           const FileSystemFlags& options,
                                           ExceptionState& exception_state) {
  EntryCallbacksSyncHelper* sync_helper = EntryCallbacksSyncHelper::Create();
  file_system_->GetFile(this, path, options, sync_helper->GetSuccessCallback(),
                        sync_helper->GetErrorCallback(),
                        DOMFileSystemBase::kSynchronous);
  Entry* entry = sync_helper->GetResultOrThrow(exception_state);
  return entry ? ToFileEntrySync(EntrySync::Create(entry)) : nullptr;
}

DirectoryEntrySync* DirectoryEntrySync::getDirectory(
    const String& path,
    const FileSystemFlags& options,
    ExceptionState& exception_state) {
  EntryCallbacksSyncHelper* sync_helper = EntryCallbacksSyncHelper::Create();
  file_system_->GetDirectory(
      this, path, options, sync_helper->GetSuccessCallback(),
      sync_helper->GetErrorCallback(), DOMFileSystemBase::kSynchronous);
  Entry* entry = sync_helper->GetResultOrThrow(exception_state);
  return entry ? ToDirectoryEntrySync(EntrySync::Create(entry)) : nullptr;
}

void DirectoryEntrySync::removeRecursively(ExceptionState& exception_state) {
  VoidCallbacksSyncHelper* sync_helper = VoidCallbacksSyncHelper::Create();
  file_system_->RemoveRecursively(this, nullptr,
                                  sync_helper->GetErrorCallback(),
                                  DOMFileSystemBase::kSynchronous);
  sync_helper->GetResultOrThrow(exception_state);
}

void DirectoryEntrySync::Trace(blink::Visitor* visitor) {
  EntrySync::Trace(visitor);
}

}  // namespace blink
