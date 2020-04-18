/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/fileapi/file.h"

#include <memory>
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/fileapi/file_property_bag.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/file_metadata.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

static String GetContentTypeFromFileName(const String& name,
                                         File::ContentTypeLookupPolicy policy) {
  String type;
  int index = name.ReverseFind('.');
  if (index != -1) {
    if (policy == File::kWellKnownContentTypes) {
      type = MIMETypeRegistry::GetWellKnownMIMETypeForExtension(
          name.Substring(index + 1));
    } else {
      DCHECK_EQ(policy, File::kAllContentTypes);
      type =
          MIMETypeRegistry::GetMIMETypeForExtension(name.Substring(index + 1));
    }
  }
  return type;
}

static std::unique_ptr<BlobData> CreateBlobDataForFileWithType(
    const String& path,
    const String& content_type) {
  std::unique_ptr<BlobData> blob_data =
      BlobData::CreateForFileWithUnknownSize(path);
  blob_data->SetContentType(content_type);
  return blob_data;
}

static std::unique_ptr<BlobData> CreateBlobDataForFile(
    const String& path,
    File::ContentTypeLookupPolicy policy) {
  return CreateBlobDataForFileWithType(
      path, GetContentTypeFromFileName(path, policy));
}

static std::unique_ptr<BlobData> CreateBlobDataForFileWithName(
    const String& path,
    const String& file_system_name,
    File::ContentTypeLookupPolicy policy) {
  return CreateBlobDataForFileWithType(
      path, GetContentTypeFromFileName(file_system_name, policy));
}

static std::unique_ptr<BlobData> CreateBlobDataForFileWithMetadata(
    const String& file_system_name,
    const FileMetadata& metadata) {
  std::unique_ptr<BlobData> blob_data;
  if (metadata.length == BlobData::kToEndOfFile) {
    blob_data = BlobData::CreateForFileWithUnknownSize(
        metadata.platform_path, metadata.modification_time / kMsPerSecond);
  } else {
    blob_data = BlobData::Create();
    blob_data->AppendFile(metadata.platform_path, 0, metadata.length,
                          metadata.modification_time / kMsPerSecond);
  }
  blob_data->SetContentType(GetContentTypeFromFileName(
      file_system_name, File::kWellKnownContentTypes));
  return blob_data;
}

static std::unique_ptr<BlobData> CreateBlobDataForFileSystemURL(
    const KURL& file_system_url,
    const FileMetadata& metadata) {
  std::unique_ptr<BlobData> blob_data;
  if (metadata.length == BlobData::kToEndOfFile) {
    blob_data = BlobData::CreateForFileSystemURLWithUnknownSize(
        file_system_url, metadata.modification_time / kMsPerSecond);
  } else {
    blob_data = BlobData::Create();
    blob_data->AppendFileSystemURL(file_system_url, 0, metadata.length,
                                   metadata.modification_time / kMsPerSecond);
  }
  blob_data->SetContentType(GetContentTypeFromFileName(
      file_system_url.GetPath(), File::kWellKnownContentTypes));
  return blob_data;
}

// static
File* File::Create(
    ExecutionContext* context,
    const HeapVector<ArrayBufferOrArrayBufferViewOrBlobOrUSVString>& file_bits,
    const String& file_name,
    const FilePropertyBag& options,
    ExceptionState& exception_state) {
  DCHECK(options.hasType());

  double last_modified;
  if (options.hasLastModified())
    last_modified = static_cast<double>(options.lastModified());
  else
    last_modified = CurrentTimeMS();
  DCHECK(options.hasEndings());
  bool normalize_line_endings_to_native = options.endings() == "native";
  if (normalize_line_endings_to_native)
    UseCounter::Count(context, WebFeature::kFileAPINativeLineEndings);

  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->SetContentType(NormalizeType(options.type()));
  PopulateBlobData(blob_data.get(), file_bits,
                   normalize_line_endings_to_native);

  long long file_size = blob_data->length();
  return File::Create(file_name, last_modified,
                      BlobDataHandle::Create(std::move(blob_data), file_size));
}

File* File::CreateWithRelativePath(const String& path,
                                   const String& relative_path) {
  File* file = new File(path, File::kAllContentTypes, File::kIsUserVisible);
  file->relative_path_ = relative_path;
  return file;
}

File::File(const String& path,
           ContentTypeLookupPolicy policy,
           UserVisibility user_visibility)
    : Blob(BlobDataHandle::Create(CreateBlobDataForFile(path, policy), -1)),
      has_backing_file_(true),
      user_visibility_(user_visibility),
      path_(path),
      name_(FilePathToWebString(WebStringToFilePath(path).BaseName())),
      snapshot_size_(-1),
      snapshot_modification_time_ms_(InvalidFileTime()) {}

File::File(const String& path,
           const String& name,
           ContentTypeLookupPolicy policy,
           UserVisibility user_visibility)
    : Blob(BlobDataHandle::Create(
          CreateBlobDataForFileWithName(path, name, policy),
          -1)),
      has_backing_file_(true),
      user_visibility_(user_visibility),
      path_(path),
      name_(name),
      snapshot_size_(-1),
      snapshot_modification_time_ms_(InvalidFileTime()) {}

File::File(const String& path,
           const String& name,
           const String& relative_path,
           UserVisibility user_visibility,
           bool has_snapshot_data,
           uint64_t size,
           double last_modified,
           scoped_refptr<BlobDataHandle> blob_data_handle)
    : Blob(std::move(blob_data_handle)),
      has_backing_file_(!path.IsEmpty() || !relative_path.IsEmpty()),
      user_visibility_(user_visibility),
      path_(path),
      name_(name),
      snapshot_size_(has_snapshot_data ? static_cast<long long>(size) : -1),
      snapshot_modification_time_ms_(has_snapshot_data ? last_modified
                                                       : InvalidFileTime()),
      relative_path_(relative_path) {}

File::File(const String& name,
           double modification_time_ms,
           scoped_refptr<BlobDataHandle> blob_data_handle)
    : Blob(std::move(blob_data_handle)),
      has_backing_file_(false),
      user_visibility_(File::kIsNotUserVisible),
      name_(name),
      snapshot_size_(Blob::size()),
      snapshot_modification_time_ms_(modification_time_ms) {}

File::File(const String& name,
           const FileMetadata& metadata,
           UserVisibility user_visibility)
    : Blob(BlobDataHandle::Create(
          CreateBlobDataForFileWithMetadata(name, metadata),
          metadata.length)),
      has_backing_file_(true),
      user_visibility_(user_visibility),
      path_(metadata.platform_path),
      name_(name),
      snapshot_size_(metadata.length),
      snapshot_modification_time_ms_(metadata.modification_time) {}

File::File(const KURL& file_system_url,
           const FileMetadata& metadata,
           UserVisibility user_visibility)
    : Blob(BlobDataHandle::Create(
          CreateBlobDataForFileSystemURL(file_system_url, metadata),
          metadata.length)),
      has_backing_file_(false),
      user_visibility_(user_visibility),
      name_(DecodeURLEscapeSequences(file_system_url.LastPathComponent())),
      file_system_url_(file_system_url),
      snapshot_size_(metadata.length),
      snapshot_modification_time_ms_(metadata.modification_time) {}

File::File(const File& other)
    : Blob(other.GetBlobDataHandle()),
      has_backing_file_(other.has_backing_file_),
      user_visibility_(other.user_visibility_),
      path_(other.path_),
      name_(other.name_),
      file_system_url_(other.file_system_url_),
      snapshot_size_(other.snapshot_size_),
      snapshot_modification_time_ms_(other.snapshot_modification_time_ms_),
      relative_path_(other.relative_path_) {}

File* File::Clone(const String& name) const {
  File* file = new File(*this);
  if (!name.IsNull())
    file->name_ = name;
  return file;
}

double File::LastModifiedMS() const {
  if (HasValidSnapshotMetadata() &&
      IsValidFileTime(snapshot_modification_time_ms_))
    return snapshot_modification_time_ms_;

  double modification_time_ms;
  if (HasBackingFile() &&
      GetFileModificationTime(path_, modification_time_ms) &&
      IsValidFileTime(modification_time_ms))
    return modification_time_ms;

  return CurrentTimeMS();
}

long long File::lastModified() const {
  double modified_date = LastModifiedMS();

  // The getter should return the current time when the last modification time
  // isn't known.
  if (!IsValidFileTime(modified_date))
    modified_date = CurrentTimeMS();

  // lastModified returns a number, not a Date instance,
  // http://dev.w3.org/2006/webapi/FileAPI/#file-attrs
  return floor(modified_date);
}

double File::lastModifiedDate() const {
  double modified_date = LastModifiedMS();

  // The getter should return the current time when the last modification time
  // isn't known.
  if (!IsValidFileTime(modified_date))
    modified_date = CurrentTimeMS();

  // lastModifiedDate returns a Date instance,
  // http://www.w3.org/TR/FileAPI/#dfn-lastModifiedDate
  return modified_date;
}

unsigned long long File::size() const {
  if (HasValidSnapshotMetadata())
    return snapshot_size_;

  // FIXME: JavaScript cannot represent sizes as large as unsigned long long, we
  // need to come up with an exception to throw if file size is not
  // representable.
  long long size;
  if (!HasBackingFile() || !GetFileSize(path_, size))
    return 0;
  return static_cast<unsigned long long>(size);
}

Blob* File::slice(long long start,
                  long long end,
                  const String& content_type,
                  ExceptionState& exception_state) const {
  if (!has_backing_file_)
    return Blob::slice(start, end, content_type, exception_state);

  // FIXME: This involves synchronous file operation. We need to figure out how
  // to make it asynchronous.
  long long size;
  double modification_time_ms;
  CaptureSnapshot(size, modification_time_ms);
  ClampSliceOffsets(size, start, end);

  long long length = end - start;
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->SetContentType(NormalizeType(content_type));
  DCHECK(!path_.IsEmpty());
  blob_data->AppendFile(path_, start, length,
                        modification_time_ms / kMsPerSecond);
  return Blob::Create(BlobDataHandle::Create(std::move(blob_data), length));
}

void File::CaptureSnapshot(long long& snapshot_size,
                           double& snapshot_modification_time_ms) const {
  if (HasValidSnapshotMetadata()) {
    snapshot_size = snapshot_size_;
    snapshot_modification_time_ms = snapshot_modification_time_ms_;
    return;
  }

  // Obtains a snapshot of the file by capturing its current size and
  // modification time. This is used when we slice a file for the first time.
  // If we fail to retrieve the size or modification time, probably due to that
  // the file has been deleted, 0 size is returned.
  FileMetadata metadata;
  if (!HasBackingFile() || !GetFileMetadata(path_, metadata)) {
    snapshot_size = 0;
    snapshot_modification_time_ms = InvalidFileTime();
    return;
  }

  snapshot_size = metadata.length;
  snapshot_modification_time_ms = metadata.modification_time;
}

void File::AppendTo(BlobData& blob_data) const {
  if (!has_backing_file_) {
    Blob::AppendTo(blob_data);
    return;
  }

  // FIXME: This involves synchronous file operation. We need to figure out how
  // to make it asynchronous.
  long long size;
  double modification_time_ms;
  CaptureSnapshot(size, modification_time_ms);
  DCHECK(!path_.IsEmpty());
  blob_data.AppendFile(path_, 0, size, modification_time_ms / kMsPerSecond);
}

bool File::HasSameSource(const File& other) const {
  if (has_backing_file_ != other.has_backing_file_)
    return false;

  if (has_backing_file_)
    return path_ == other.path_;

  if (file_system_url_.IsEmpty() != other.file_system_url_.IsEmpty())
    return false;

  if (!file_system_url_.IsEmpty())
    return file_system_url_ == other.file_system_url_;

  return Uuid() == other.Uuid();
}

}  // namespace blink
