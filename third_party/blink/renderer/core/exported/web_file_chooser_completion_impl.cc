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

#include "third_party/blink/renderer/core/exported/web_file_chooser_completion_impl.h"

#include "third_party/blink/renderer/platform/file_metadata.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"

namespace blink {

WebFileChooserCompletionImpl::WebFileChooserCompletionImpl(
    scoped_refptr<FileChooser> chooser)
    : file_chooser_(std::move(chooser)) {}

WebFileChooserCompletionImpl::~WebFileChooserCompletionImpl() = default;

void WebFileChooserCompletionImpl::DidChooseFile(
    const WebVector<WebString>& file_names) {
  Vector<FileChooserFileInfo> file_info;
  for (size_t i = 0; i < file_names.size(); ++i)
    file_info.push_back(FileChooserFileInfo(file_names[i]));
  file_chooser_->ChooseFiles(file_info);
  // This object is no longer needed.
  delete this;
}

void WebFileChooserCompletionImpl::DidChooseFile(
    const WebVector<SelectedFileInfo>& files) {
  Vector<FileChooserFileInfo> file_info;
  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].file_system_url.IsEmpty()) {
      file_info.push_back(
          FileChooserFileInfo(files[i].path, files[i].display_name));
    } else {
      FileMetadata metadata;
      metadata.modification_time = files[i].modification_time * kMsPerSecond;
      metadata.length = files[i].length;
      metadata.type = files[i].is_directory ? FileMetadata::kTypeDirectory
                                            : FileMetadata::kTypeFile;
      file_info.push_back(
          FileChooserFileInfo(files[i].file_system_url, metadata));
    }
  }
  file_chooser_->ChooseFiles(file_info);
  // This object is no longer needed.
  delete this;
}

}  // namespace blink
