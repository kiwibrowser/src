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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_

#include <memory>
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExceptionState;
class KURL;
class SecurityOrigin;
class Storage;
class WebStorageArea;
class WebStorageNamespace;

class MODULES_EXPORT StorageArea final
    : public GarbageCollectedFinalized<StorageArea> {
 public:
  enum StorageType { kLocalStorage, kSessionStorage };

  static StorageArea* Create(std::unique_ptr<WebStorageArea>, StorageType);

  virtual ~StorageArea();

  // The HTML5 DOM Storage API
  unsigned length(ExceptionState&, LocalFrame* source_frame);
  String Key(unsigned index, ExceptionState&, LocalFrame* source_frame);
  String GetItem(const String& key, ExceptionState&, LocalFrame* source_frame);
  void SetItem(const String& key,
               const String& value,
               ExceptionState&,
               LocalFrame* source_frame);
  void RemoveItem(const String& key, ExceptionState&, LocalFrame* source_frame);
  void Clear(ExceptionState&, LocalFrame* source_frame);
  bool Contains(const String& key, ExceptionState&, LocalFrame* source_frame);

  bool CanAccessStorage(LocalFrame*);

  static void DispatchLocalStorageEvent(const String& key,
                                        const String& old_value,
                                        const String& new_value,
                                        const SecurityOrigin*,
                                        const KURL& page_url,
                                        WebStorageArea* source_area_instance);
  static void DispatchSessionStorageEvent(const String& key,
                                          const String& old_value,
                                          const String& new_value,
                                          const SecurityOrigin*,
                                          const KURL& page_url,
                                          const WebStorageNamespace&,
                                          WebStorageArea* source_area_instance);

  void Trace(blink::Visitor*);

 private:
  StorageArea(std::unique_ptr<WebStorageArea>, StorageType);

  static bool IsEventSource(Storage*, WebStorageArea* source_area_instance);

  std::unique_ptr<WebStorageArea> storage_area_;
  StorageType storage_type_;
  WeakMember<LocalFrame> frame_used_for_can_access_storage_;
  bool can_access_storage_cached_result_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_AREA_H_
