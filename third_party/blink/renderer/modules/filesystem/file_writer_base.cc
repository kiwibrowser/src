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

#include "third_party/blink/renderer/modules/filesystem/file_writer_base.h"

#include <memory>
#include "third_party/blink/public/platform/web_file_writer.h"
#include "third_party/blink/renderer/core/events/progress_event.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"

namespace blink {

FileWriterBase::~FileWriterBase() = default;

void FileWriterBase::Initialize(std::unique_ptr<WebFileWriter> writer,
                                long long length) {
  DCHECK(!writer_);
  DCHECK_GE(length, 0);
  writer_ = std::move(writer);
  length_ = length;
}

FileWriterBase::FileWriterBase() : position_(0) {}

void FileWriterBase::SeekInternal(long long position) {
  if (position > length_)
    position = length_;
  else if (position < 0)
    position = length_ + position;
  if (position < 0)
    position = 0;
  position_ = position;
}

void FileWriterBase::ResetWriter() {
  writer_ = nullptr;
}

void FileWriterBase::Dispose() {
  // Need to explicitly destroy m_writer in pre-finalizer, because otherwise it
  // may attempt to call methods on the FileWriter before we are finalized.
  ResetWriter();
}

}  // namespace blink
