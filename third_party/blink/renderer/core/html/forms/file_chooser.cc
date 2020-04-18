/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/file_chooser.h"

namespace blink {

FileChooserClient::~FileChooserClient() = default;

FileChooser* FileChooserClient::NewFileChooser(
    const WebFileChooserParams& params) {
  if (chooser_)
    chooser_->DisconnectClient();

  chooser_ = FileChooser::Create(this, params);
  return chooser_.get();
}

inline FileChooser::FileChooser(FileChooserClient* client,
                                const WebFileChooserParams& params)
    : client_(client), params_(params) {}

scoped_refptr<FileChooser> FileChooser::Create(
    FileChooserClient* client,
    const WebFileChooserParams& params) {
  return base::AdoptRef(new FileChooser(client, params));
}

FileChooser::~FileChooser() = default;

void FileChooser::ChooseFiles(const Vector<FileChooserFileInfo>& files) {
  // FIXME: This is inelegant. We should not be looking at params_ here.
  if (params_.selected_files.size() == files.size()) {
    bool was_changed = false;
    for (unsigned i = 0; i < files.size(); ++i) {
      if (String(params_.selected_files[i]) != files[i].path) {
        was_changed = true;
        break;
      }
    }
    if (!was_changed)
      return;
  }

  if (client_)
    client_->FilesChosen(files);
}

}  // namespace blink
