// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/cache_storage/cache_storage_error.h"

#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/cache_storage/cache.h"

namespace blink {

DOMException* CacheStorageError::CreateException(
    mojom::CacheStorageError web_error) {
  switch (web_error) {
    case mojom::CacheStorageError::kErrorNotImplemented:
      return DOMException::Create(kNotSupportedError,
                                  "Method is not implemented.");
    case mojom::CacheStorageError::kErrorNotFound:
      return DOMException::Create(kNotFoundError, "Entry was not found.");
    case mojom::CacheStorageError::kErrorExists:
      return DOMException::Create(kInvalidAccessError, "Entry already exists.");
    case mojom::CacheStorageError::kErrorQuotaExceeded:
      return DOMException::Create(kQuotaExceededError, "Quota exceeded.");
    case mojom::CacheStorageError::kErrorCacheNameNotFound:
      return DOMException::Create(kNotFoundError, "Cache was not found.");
    case mojom::CacheStorageError::kErrorQueryTooLarge:
      return DOMException::Create(kAbortError, "Operation too large.");
    case mojom::CacheStorageError::kErrorStorage:
      return DOMException::Create(kUnknownError, "Unexpected internal error.");
    case mojom::CacheStorageError::kSuccess:
      // This function should only be called with an error.
      break;
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace blink
