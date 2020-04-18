// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/remoteplayback/html_media_element_remote_playback.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/modules/remoteplayback/remote_playback.h"

namespace blink {

// static
bool HTMLMediaElementRemotePlayback::FastHasAttribute(
    const QualifiedName& name,
    const HTMLMediaElement& element) {
  DCHECK(name == HTMLNames::disableremoteplaybackAttr);
  return element.FastHasAttribute(name);
}

// static
void HTMLMediaElementRemotePlayback::SetBooleanAttribute(
    const QualifiedName& name,
    HTMLMediaElement& element,
    bool value) {
  DCHECK(name == HTMLNames::disableremoteplaybackAttr);
  element.SetBooleanAttribute(name, value);

  HTMLMediaElementRemotePlayback& self =
      HTMLMediaElementRemotePlayback::From(element);
  if (self.remote_ && value)
    self.remote_->RemotePlaybackDisabled();
}

// static
HTMLMediaElementRemotePlayback& HTMLMediaElementRemotePlayback::From(
    HTMLMediaElement& element) {
  HTMLMediaElementRemotePlayback* supplement =
      Supplement<HTMLMediaElement>::From<HTMLMediaElementRemotePlayback>(
          element);
  if (!supplement) {
    supplement = new HTMLMediaElementRemotePlayback();
    ProvideTo(element, supplement);
  }
  return *supplement;
}

// static
RemotePlayback* HTMLMediaElementRemotePlayback::remote(
    HTMLMediaElement& element) {
  HTMLMediaElementRemotePlayback& self =
      HTMLMediaElementRemotePlayback::From(element);
  Document& document = element.GetDocument();
  if (!document.GetFrame())
    return nullptr;

  if (!self.remote_)
    self.remote_ = RemotePlayback::Create(element);

  return self.remote_;
}

// static
const char HTMLMediaElementRemotePlayback::kSupplementName[] =
    "HTMLMediaElementRemotePlayback";

void HTMLMediaElementRemotePlayback::Trace(blink::Visitor* visitor) {
  visitor->Trace(remote_);
  Supplement<HTMLMediaElement>::Trace(visitor);
}

}  // namespace blink
