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

#include "third_party/blink/public/platform/web_file_system_callbacks.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_file_info.h"
#include "third_party/blink/public/platform/web_file_system.h"
#include "third_party/blink/public/platform/web_file_system_entry.h"
#include "third_party/blink/public/platform/web_file_writer.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/platform/async_file_system_callbacks.h"
#include "third_party/blink/renderer/platform/file_metadata.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

class WebFileSystemCallbacksPrivate
    : public RefCounted<WebFileSystemCallbacksPrivate> {
 public:
  static scoped_refptr<WebFileSystemCallbacksPrivate> Create(
      std::unique_ptr<AsyncFileSystemCallbacks> callbacks) {
    return base::AdoptRef(
        new WebFileSystemCallbacksPrivate(std::move(callbacks)));
  }

  AsyncFileSystemCallbacks* Callbacks() { return callbacks_.get(); }

 private:
  WebFileSystemCallbacksPrivate(
      std::unique_ptr<AsyncFileSystemCallbacks> callbacks)
      : callbacks_(std::move(callbacks)) {}
  std::unique_ptr<AsyncFileSystemCallbacks> callbacks_;
};

WebFileSystemCallbacks::WebFileSystemCallbacks(
    std::unique_ptr<AsyncFileSystemCallbacks>&& callbacks) {
  private_ = WebFileSystemCallbacksPrivate::Create(std::move(callbacks));
}

void WebFileSystemCallbacks::Reset() {
  private_.Reset();
}

void WebFileSystemCallbacks::Assign(const WebFileSystemCallbacks& other) {
  private_ = other.private_;
}

void WebFileSystemCallbacks::DidSucceed() {
  DCHECK(!private_.IsNull());
  private_->Callbacks()->DidSucceed();
  private_.Reset();
}

void WebFileSystemCallbacks::DidReadMetadata(const WebFileInfo& web_file_info) {
  DCHECK(!private_.IsNull());
  FileMetadata file_metadata;
  file_metadata.modification_time = web_file_info.modification_time;
  file_metadata.length = web_file_info.length;
  file_metadata.type = static_cast<FileMetadata::Type>(web_file_info.type);
  file_metadata.platform_path = web_file_info.platform_path;
  private_->Callbacks()->DidReadMetadata(file_metadata);
  private_.Reset();
}

void WebFileSystemCallbacks::DidCreateSnapshotFile(
    const WebFileInfo& web_file_info) {
  DCHECK(!private_.IsNull());
  // It's important to create a BlobDataHandle that refers to the platform file
  // path prior to return from this method so the underlying file will not be
  // deleted.
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->AppendFile(web_file_info.platform_path, 0, web_file_info.length,
                        InvalidFileTime());
  scoped_refptr<BlobDataHandle> snapshot_blob =
      BlobDataHandle::Create(std::move(blob_data), web_file_info.length);

  FileMetadata file_metadata;
  file_metadata.modification_time = web_file_info.modification_time;
  file_metadata.length = web_file_info.length;
  file_metadata.type = static_cast<FileMetadata::Type>(web_file_info.type);
  file_metadata.platform_path = web_file_info.platform_path;
  private_->Callbacks()->DidCreateSnapshotFile(file_metadata, snapshot_blob);
  private_.Reset();
}

void WebFileSystemCallbacks::DidReadDirectory(
    const WebVector<WebFileSystemEntry>& entries,
    bool has_more) {
  DCHECK(!private_.IsNull());
  for (size_t i = 0; i < entries.size(); ++i)
    private_->Callbacks()->DidReadDirectoryEntry(entries[i].name,
                                                 entries[i].is_directory);
  private_->Callbacks()->DidReadDirectoryEntries(has_more);
  private_.Reset();
}

void WebFileSystemCallbacks::DidOpenFileSystem(const WebString& name,
                                               const WebURL& root_url) {
  DCHECK(!private_.IsNull());
  private_->Callbacks()->DidOpenFileSystem(name, root_url);
  private_.Reset();
}

void WebFileSystemCallbacks::DidResolveURL(const WebString& name,
                                           const WebURL& root_url,
                                           WebFileSystemType type,
                                           const WebString& file_path,
                                           bool is_directory) {
  DCHECK(!private_.IsNull());
  private_->Callbacks()->DidResolveURL(name, root_url,
                                       static_cast<FileSystemType>(type),
                                       file_path, is_directory);
  private_.Reset();
}

void WebFileSystemCallbacks::DidCreateFileWriter(WebFileWriter* web_file_writer,
                                                 long long length) {
  DCHECK(!private_.IsNull());
  private_->Callbacks()->DidCreateFileWriter(base::WrapUnique(web_file_writer),
                                             length);
  private_.Reset();
}

void WebFileSystemCallbacks::DidFail(WebFileError error) {
  DCHECK(!private_.IsNull());
  private_->Callbacks()->DidFail(error);
  private_.Reset();
}

bool WebFileSystemCallbacks::ShouldBlockUntilCompletion() const {
  DCHECK(!private_.IsNull());
  return private_->Callbacks()->ShouldBlockUntilCompletion();
}

}  // namespace blink
