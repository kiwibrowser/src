/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "third_party/blink/renderer/modules/filesystem/dom_window_file_system.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/modules/filesystem/dom_file_system.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_callbacks.h"
#include "third_party/blink/renderer/modules/filesystem/local_file_system.h"
#include "third_party/blink/renderer/platform/file_system_type.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

void DOMWindowFileSystem::webkitRequestFileSystem(
    LocalDOMWindow& window,
    int type,
    long long size,
    V8FileSystemCallback* success_callback,
    V8ErrorCallback* error_callback) {
  if (!window.IsCurrentlyDisplayedInFrame())
    return;

  Document* document = window.document();
  if (!document)
    return;

  if (SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
          document->GetSecurityOrigin()->Protocol()))
    UseCounter::Count(document, WebFeature::kRequestFileSystemNonWebbyOrigin);

  if (!document->GetSecurityOrigin()->CanAccessFileSystem()) {
    DOMFileSystem::ReportError(document,
                               ScriptErrorCallback::Wrap(error_callback),
                               FileError::kSecurityErr);
    return;
  } else if (document->GetSecurityOrigin()->IsLocal()) {
    UseCounter::Count(document, WebFeature::kFileAccessedFileSystem);
  }

  FileSystemType file_system_type = static_cast<FileSystemType>(type);
  if (!DOMFileSystemBase::IsValidType(file_system_type)) {
    DOMFileSystem::ReportError(document,
                               ScriptErrorCallback::Wrap(error_callback),
                               FileError::kInvalidModificationErr);
    return;
  }

  LocalFileSystem::From(*document)->RequestFileSystem(
      document, file_system_type, size,
      FileSystemCallbacks::Create(
          FileSystemCallbacks::OnDidOpenFileSystemV8Impl::Create(
              success_callback),
          ScriptErrorCallback::Wrap(error_callback), document,
          file_system_type));
}

void DOMWindowFileSystem::webkitResolveLocalFileSystemURL(
    LocalDOMWindow& window,
    const String& url,
    V8EntryCallback* success_callback,
    V8ErrorCallback* error_callback) {
  if (!window.IsCurrentlyDisplayedInFrame())
    return;

  Document* document = window.document();
  if (!document)
    return;

  const SecurityOrigin* security_origin = document->GetSecurityOrigin();
  KURL completed_url = document->CompleteURL(url);
  if (!security_origin->CanAccessFileSystem() ||
      !security_origin->CanRequest(completed_url)) {
    DOMFileSystem::ReportError(document,
                               ScriptErrorCallback::Wrap(error_callback),
                               FileError::kSecurityErr);
    return;
  } else if (document->GetSecurityOrigin()->IsLocal()) {
    UseCounter::Count(document, WebFeature::kFileAccessedFileSystem);
  }

  if (!completed_url.IsValid()) {
    DOMFileSystem::ReportError(document,
                               ScriptErrorCallback::Wrap(error_callback),
                               FileError::kEncodingErr);
    return;
  }

  LocalFileSystem::From(*document)->ResolveURL(
      document, completed_url,
      ResolveURICallbacks::Create(
          ResolveURICallbacks::OnDidGetEntryV8Impl::Create(success_callback),
          ScriptErrorCallback::Wrap(error_callback), document));
}

static_assert(
    static_cast<int>(DOMWindowFileSystem::kTemporary) ==
        static_cast<int>(kFileSystemTypeTemporary),
    "DOMWindowFileSystem::kTemporary should match FileSystemTypeTemporary");
static_assert(
    static_cast<int>(DOMWindowFileSystem::kPersistent) ==
        static_cast<int>(kFileSystemTypePersistent),
    "DOMWindowFileSystem::kPersistent should match FileSystemTypePersistent");

}  // namespace blink
