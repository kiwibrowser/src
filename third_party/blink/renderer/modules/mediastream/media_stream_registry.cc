/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/mediastream/media_stream_registry.h"

#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

MediaStreamRegistry& MediaStreamRegistry::Registry() {
  // Since WebWorkers cannot obtain MediaStream objects, we should be on the
  // main thread.
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(MediaStreamRegistry, instance, ());
  return instance;
}

void MediaStreamRegistry::RegisterURL(SecurityOrigin*,
                                      const KURL& url,
                                      URLRegistrable* stream) {
  DCHECK(&stream->Registry() == this);
  DCHECK(IsMainThread());
  stream_descriptors_.Set(url.GetString(),
                          static_cast<MediaStream*>(stream)->Descriptor());
}

void MediaStreamRegistry::UnregisterURL(const KURL& url) {
  DCHECK(IsMainThread());
  stream_descriptors_.erase(url.GetString());
}

bool MediaStreamRegistry::Contains(const String& url) {
  DCHECK(IsMainThread());
  return stream_descriptors_.Contains(url);
}

MediaStreamDescriptor* MediaStreamRegistry::LookupMediaStreamDescriptor(
    const String& url) {
  DCHECK(IsMainThread());
  return stream_descriptors_.at(url);
}

MediaStreamRegistry::MediaStreamRegistry() {
  HTMLMediaElement::SetMediaStreamRegistry(this);
}

}  // namespace blink
