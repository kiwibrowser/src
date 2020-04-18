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

#include "third_party/blink/public/web/web_storage_event_dispatcher.h"

#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/modules/storage/storage_area.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

void WebStorageEventDispatcher::DispatchLocalStorageEvent(
    const WebString& key,
    const WebString& old_value,
    const WebString& new_value,
    const WebURL& origin,
    const WebURL& page_url,
    WebStorageArea* source_area_instance) {
  scoped_refptr<const SecurityOrigin> security_origin =
      SecurityOrigin::Create(origin);
  StorageArea::DispatchLocalStorageEvent(key, old_value, new_value,
                                         security_origin.get(), page_url,
                                         source_area_instance);
}

void WebStorageEventDispatcher::DispatchSessionStorageEvent(
    const WebString& key,
    const WebString& old_value,
    const WebString& new_value,
    const WebURL& origin,
    const WebURL& page_url,
    const WebStorageNamespace& session_namespace,
    WebStorageArea* source_area_instance) {
  scoped_refptr<const SecurityOrigin> security_origin =
      SecurityOrigin::Create(origin);
  StorageArea::DispatchSessionStorageEvent(
      key, old_value, new_value, security_origin.get(), page_url,
      session_namespace, source_area_instance);
}

}  // namespace blink
