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

#include "third_party/blink/renderer/modules/filesystem/dom_file_system_base.h"

#include <memory>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_file_system.h"
#include "third_party/blink/public/platform/web_file_system_callbacks.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/modules/filesystem/directory_entry.h"
#include "third_party/blink/renderer/modules/filesystem/directory_reader_base.h"
#include "third_party/blink/renderer/modules/filesystem/dom_file_path.h"
#include "third_party/blink/renderer/modules/filesystem/entry.h"
#include "third_party/blink/renderer/modules/filesystem/entry_base.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_callbacks.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"

namespace blink {

const char DOMFileSystemBase::kPersistentPathPrefix[] = "persistent";
const char DOMFileSystemBase::kTemporaryPathPrefix[] = "temporary";
const char DOMFileSystemBase::kIsolatedPathPrefix[] = "isolated";
const char DOMFileSystemBase::kExternalPathPrefix[] = "external";

DOMFileSystemBase::DOMFileSystemBase(ExecutionContext* context,
                                     const String& name,
                                     FileSystemType type,
                                     const KURL& root_url)
    : context_(context),
      name_(name),
      type_(type),
      filesystem_root_url_(root_url),
      clonable_(false) {}

DOMFileSystemBase::~DOMFileSystemBase() = default;

void DOMFileSystemBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(context_);
  ScriptWrappable::Trace(visitor);
}

WebFileSystem* DOMFileSystemBase::FileSystem() const {
  Platform* platform = Platform::Current();
  if (!platform)
    return nullptr;
  return platform->FileSystem();
}

const SecurityOrigin* DOMFileSystemBase::GetSecurityOrigin() const {
  return context_->GetSecurityOrigin();
}

bool DOMFileSystemBase::IsValidType(FileSystemType type) {
  return type == kFileSystemTypeTemporary ||
         type == kFileSystemTypePersistent || type == kFileSystemTypeIsolated ||
         type == kFileSystemTypeExternal;
}

KURL DOMFileSystemBase::CreateFileSystemRootURL(const String& origin,
                                                FileSystemType type) {
  String type_string;
  if (type == kFileSystemTypeTemporary)
    type_string = kTemporaryPathPrefix;
  else if (type == kFileSystemTypePersistent)
    type_string = kPersistentPathPrefix;
  else if (type == kFileSystemTypeExternal)
    type_string = kExternalPathPrefix;
  else
    return KURL();

  String result = "filesystem:" + origin + "/" + type_string + "/";
  return KURL(result);
}

bool DOMFileSystemBase::SupportsToURL() const {
  DCHECK(IsValidType(type_));
  return type_ != kFileSystemTypeIsolated;
}

KURL DOMFileSystemBase::CreateFileSystemURL(const EntryBase* entry) const {
  return CreateFileSystemURL(entry->fullPath());
}

KURL DOMFileSystemBase::CreateFileSystemURL(const String& full_path) const {
  DCHECK(DOMFilePath::IsAbsolute(full_path));

  if (GetType() == kFileSystemTypeExternal) {
    // For external filesystem originString could be different from what we have
    // in m_filesystemRootURL.
    StringBuilder result;
    result.Append("filesystem:");
    result.Append(GetSecurityOrigin()->ToString());
    result.Append('/');
    result.Append(kExternalPathPrefix);
    result.Append(filesystem_root_url_.GetPath());
    // Remove the extra leading slash.
    result.Append(EncodeWithURLEscapeSequences(full_path.Substring(1)));
    return KURL(result.ToString());
  }

  // For regular types we can just append the entry's fullPath to the
  // m_filesystemRootURL that should look like
  // 'filesystem:<origin>/<typePrefix>'.
  DCHECK(!filesystem_root_url_.IsEmpty());
  KURL url = filesystem_root_url_;
  // Remove the extra leading slash.
  url.SetPath(url.GetPath() +
              EncodeWithURLEscapeSequences(full_path.Substring(1)));
  return url;
}

bool DOMFileSystemBase::PathToAbsolutePath(FileSystemType type,
                                           const EntryBase* base,
                                           String path,
                                           String& absolute_path) {
  DCHECK(base);

  if (!DOMFilePath::IsAbsolute(path))
    path = DOMFilePath::Append(base->fullPath(), path);
  absolute_path = DOMFilePath::RemoveExtraParentReferences(path);

  return (type != kFileSystemTypeTemporary &&
          type != kFileSystemTypePersistent) ||
         DOMFilePath::IsValidPath(absolute_path);
}

bool DOMFileSystemBase::PathPrefixToFileSystemType(const String& path_prefix,
                                                   FileSystemType& type) {
  if (path_prefix == kTemporaryPathPrefix) {
    type = kFileSystemTypeTemporary;
    return true;
  }

  if (path_prefix == kPersistentPathPrefix) {
    type = kFileSystemTypePersistent;
    return true;
  }

  if (path_prefix == kExternalPathPrefix) {
    type = kFileSystemTypeExternal;
    return true;
  }

  return false;
}

File* DOMFileSystemBase::CreateFile(const FileMetadata& metadata,
                                    const KURL& file_system_url,
                                    FileSystemType type,
                                    const String name) {
  // For regular filesystem types (temporary or persistent), we should not cache
  // file metadata as it could change File semantics.  For other filesystem
  // types (which could be platform-specific ones), there's a chance that the
  // files are on remote filesystem.  If the port has returned metadata just
  // pass it to File constructor (so we may cache the metadata).
  // FIXME: We should use the snapshot metadata for all files.
  // https://www.w3.org/Bugs/Public/show_bug.cgi?id=17746
  if (type == kFileSystemTypeTemporary || type == kFileSystemTypePersistent)
    return File::CreateForFileSystemFile(metadata.platform_path, name);

  const File::UserVisibility user_visibility = (type == kFileSystemTypeExternal)
                                                   ? File::kIsUserVisible
                                                   : File::kIsNotUserVisible;

  if (!metadata.platform_path.IsEmpty()) {
    // If the platformPath in the returned metadata is given, we create a File
    // object for the snapshot path.
    return File::CreateForFileSystemFile(name, metadata, user_visibility);
  } else {
    // Otherwise we create a File object for the fileSystemURL.
    return File::CreateForFileSystemFile(file_system_url, metadata,
                                         user_visibility);
  }
}

void DOMFileSystemBase::GetMetadata(
    const EntryBase* entry,
    MetadataCallbacks::OnDidReadMetadataCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(MetadataCallbacks::Create(
      success_callback, error_callback, context_, this));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);
  FileSystem()->ReadMetadata(CreateFileSystemURL(entry), std::move(callbacks));
}

static bool VerifyAndGetDestinationPathForCopyOrMove(const EntryBase* source,
                                                     EntryBase* parent,
                                                     const String& new_name,
                                                     String& destination_path) {
  DCHECK(source);

  if (!parent || !parent->isDirectory())
    return false;

  if (!new_name.IsEmpty() && !DOMFilePath::IsValidName(new_name))
    return false;

  const bool is_same_file_system =
      (*source->filesystem() == *parent->filesystem());

  // It is an error to try to copy or move an entry inside itself at any depth
  // if it is a directory.
  if (source->isDirectory() && is_same_file_system &&
      DOMFilePath::IsParentOf(source->fullPath(), parent->fullPath()))
    return false;

  // It is an error to copy or move an entry into its parent if a name different
  // from its current one isn't provided.
  if (is_same_file_system &&
      (new_name.IsEmpty() || source->name() == new_name) &&
      DOMFilePath::GetDirectory(source->fullPath()) == parent->fullPath())
    return false;

  destination_path = parent->fullPath();
  if (!new_name.IsEmpty())
    destination_path = DOMFilePath::Append(destination_path, new_name);
  else
    destination_path = DOMFilePath::Append(destination_path, source->name());

  return true;
}

void DOMFileSystemBase::Move(
    const EntryBase* source,
    EntryBase* parent,
    const String& new_name,
    EntryCallbacks::OnDidGetEntryCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  String destination_path;
  if (!VerifyAndGetDestinationPathForCopyOrMove(source, parent, new_name,
                                                destination_path)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(EntryCallbacks::Create(
      success_callback, error_callback, context_, parent->filesystem(),
      destination_path, source->isDirectory()));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  FileSystem()->Move(
      CreateFileSystemURL(source),
      parent->filesystem()->CreateFileSystemURL(destination_path),
      std::move(callbacks));
}

void DOMFileSystemBase::Copy(
    const EntryBase* source,
    EntryBase* parent,
    const String& new_name,
    EntryCallbacks::OnDidGetEntryCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  String destination_path;
  if (!VerifyAndGetDestinationPathForCopyOrMove(source, parent, new_name,
                                                destination_path)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(EntryCallbacks::Create(
      success_callback, error_callback, context_, parent->filesystem(),
      destination_path, source->isDirectory()));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  FileSystem()->Copy(
      CreateFileSystemURL(source),
      parent->filesystem()->CreateFileSystemURL(destination_path),
      std::move(callbacks));
}

void DOMFileSystemBase::Remove(
    const EntryBase* entry,
    VoidCallbacks::OnDidSucceedCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  DCHECK(entry);
  // We don't allow calling remove() on the root directory.
  if (entry->fullPath() == String(DOMFilePath::kRoot)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(
      VoidCallbacks::Create(success_callback, error_callback, context_, this));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  FileSystem()->Remove(CreateFileSystemURL(entry), std::move(callbacks));
}

void DOMFileSystemBase::RemoveRecursively(
    const EntryBase* entry,
    VoidCallbacks::OnDidSucceedCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  DCHECK(entry);
  DCHECK(entry->isDirectory());
  // We don't allow calling remove() on the root directory.
  if (entry->fullPath() == String(DOMFilePath::kRoot)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(
      VoidCallbacks::Create(success_callback, error_callback, context_, this));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  FileSystem()->RemoveRecursively(CreateFileSystemURL(entry),
                                  std::move(callbacks));
}

void DOMFileSystemBase::GetParent(
    const EntryBase* entry,
    EntryCallbacks::OnDidGetEntryCallback* success_callback,
    ErrorCallbackBase* error_callback) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  DCHECK(entry);
  String path = DOMFilePath::GetDirectory(entry->fullPath());

  FileSystem()->DirectoryExists(
      CreateFileSystemURL(path),
      EntryCallbacks::Create(success_callback, error_callback, context_, this,
                             path, true));
}

void DOMFileSystemBase::GetFile(
    const EntryBase* entry,
    const String& path,
    const FileSystemFlags& flags,
    EntryCallbacks::OnDidGetEntryCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  String absolute_path;
  if (!PathToAbsolutePath(type_, entry, path, absolute_path)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(EntryCallbacks::Create(
      success_callback, error_callback, context_, this, absolute_path, false));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  if (flags.createFlag())
    FileSystem()->CreateFile(CreateFileSystemURL(absolute_path),
                             flags.exclusive(), std::move(callbacks));
  else
    FileSystem()->FileExists(CreateFileSystemURL(absolute_path),
                             std::move(callbacks));
}

void DOMFileSystemBase::GetDirectory(
    const EntryBase* entry,
    const String& path,
    const FileSystemFlags& flags,
    EntryCallbacks::OnDidGetEntryCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return;
  }

  String absolute_path;
  if (!PathToAbsolutePath(type_, entry, path, absolute_path)) {
    ReportError(error_callback, FileError::kInvalidModificationErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(EntryCallbacks::Create(
      success_callback, error_callback, context_, this, absolute_path, true));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  if (flags.createFlag())
    FileSystem()->CreateDirectory(CreateFileSystemURL(absolute_path),
                                  flags.exclusive(), std::move(callbacks));
  else
    FileSystem()->DirectoryExists(CreateFileSystemURL(absolute_path),
                                  std::move(callbacks));
}

int DOMFileSystemBase::ReadDirectory(
    DirectoryReaderBase* reader,
    const String& path,
    EntriesCallbacks::OnDidGetEntriesCallback* success_callback,
    ErrorCallbackBase* error_callback,
    SynchronousType synchronous_type) {
  if (!FileSystem()) {
    ReportError(error_callback, FileError::kAbortErr);
    return 0;
  }

  DCHECK(DOMFilePath::IsAbsolute(path));

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks(EntriesCallbacks::Create(
      success_callback, error_callback, context_, reader, path));
  callbacks->SetShouldBlockUntilCompletion(synchronous_type == kSynchronous);

  return FileSystem()->ReadDirectory(CreateFileSystemURL(path),
                                     std::move(callbacks));
}

bool DOMFileSystemBase::WaitForAdditionalResult(int callbacks_id) {
  if (!FileSystem())
    return false;
  return FileSystem()->WaitForAdditionalResult(callbacks_id);
}

STATIC_ASSERT_ENUM(WebFileSystem::kTypeTemporary, kFileSystemTypeTemporary);
STATIC_ASSERT_ENUM(WebFileSystem::kTypePersistent, kFileSystemTypePersistent);
STATIC_ASSERT_ENUM(WebFileSystem::kTypeExternal, kFileSystemTypeExternal);
STATIC_ASSERT_ENUM(WebFileSystem::kTypeIsolated, kFileSystemTypeIsolated);

}  // namespace blink
