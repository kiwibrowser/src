/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#include "third_party/blink/renderer/modules/filesystem/file_writer_sync.h"

#include "third_party/blink/public/platform/web_file_writer.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"

namespace blink {

void FileWriterSync::write(Blob* data, ExceptionState& exception_state) {
  DCHECK(data);
  DCHECK(Writer());
  DCHECK(complete_);

  PrepareForWrite();
  Writer()->Write(position(), data->Uuid());
  DCHECK(complete_);
  if (error_) {
    FileError::ThrowDOMException(exception_state, error_);
    return;
  }
  SetPosition(position() + data->size());
  if (position() > length())
    SetLength(position());
}

void FileWriterSync::seek(long long position, ExceptionState& exception_state) {
  DCHECK(Writer());
  DCHECK(complete_);
  SeekInternal(position);
}

void FileWriterSync::truncate(long long offset,
                              ExceptionState& exception_state) {
  DCHECK(Writer());
  DCHECK(complete_);
  if (offset < 0) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      FileError::kInvalidStateErrorMessage);
    return;
  }
  PrepareForWrite();
  Writer()->Truncate(offset);
  DCHECK(complete_);
  if (error_) {
    FileError::ThrowDOMException(exception_state, error_);
    return;
  }
  if (offset < position())
    SetPosition(offset);
  SetLength(offset);
}

void FileWriterSync::DidWrite(long long bytes, bool complete) {
  DCHECK_EQ(FileError::kOK, error_);
  DCHECK(!complete_);
  complete_ = complete;
}

void FileWriterSync::DidTruncate() {
  DCHECK_EQ(FileError::kOK, error_);
  DCHECK(!complete_);
  complete_ = true;
}

void FileWriterSync::DidFail(WebFileError error) {
  DCHECK_EQ(FileError::kOK, error_);
  error_ = static_cast<FileError::ErrorCode>(error);
  DCHECK(!complete_);
  complete_ = true;
}

FileWriterSync::FileWriterSync() : error_(FileError::kOK), complete_(true) {}

void FileWriterSync::PrepareForWrite() {
  DCHECK(complete_);
  error_ = FileError::kOK;
  complete_ = false;
}

FileWriterSync::~FileWriterSync() = default;

void FileWriterSync::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  FileWriterBase::Trace(visitor);
}

}  // namespace blink
