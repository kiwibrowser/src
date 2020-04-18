/*
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/storage/storage_namespace.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_storage_area.h"
#include "third_party/blink/public/platform/web_storage_namespace.h"
#include "third_party/blink/renderer/modules/storage/storage_area.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

StorageNamespace::StorageNamespace(
    std::unique_ptr<WebStorageNamespace> web_storage_namespace)
    : web_storage_namespace_(std::move(web_storage_namespace)) {}

StorageNamespace::~StorageNamespace() = default;

StorageArea* StorageNamespace::LocalStorageArea(const SecurityOrigin* origin) {
  DCHECK(IsMainThread());
  static std::unique_ptr<WebStorageNamespace> local_storage_namespace = nullptr;
  if (!local_storage_namespace)
    local_storage_namespace =
        Platform::Current()->CreateLocalStorageNamespace();
  return StorageArea::Create(
      base::WrapUnique(local_storage_namespace->CreateStorageArea(
          WebSecurityOrigin(origin))),
      StorageArea::kLocalStorage);
}

StorageArea* StorageNamespace::GetStorageArea(const SecurityOrigin* origin) {
  return StorageArea::Create(
      base::WrapUnique(
          web_storage_namespace_->CreateStorageArea(WebSecurityOrigin(origin))),
      StorageArea::kSessionStorage);
}

bool StorageNamespace::IsSameNamespace(
    const WebStorageNamespace& session_namespace) const {
  return web_storage_namespace_ &&
         web_storage_namespace_->IsSameNamespace(session_namespace);
}

}  // namespace blink
