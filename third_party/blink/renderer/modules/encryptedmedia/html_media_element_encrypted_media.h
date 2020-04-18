// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ENCRYPTEDMEDIA_HTML_MEDIA_ELEMENT_ENCRYPTED_MEDIA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ENCRYPTEDMEDIA_HTML_MEDIA_ELEMENT_ENCRYPTED_MEDIA_H_

#include "third_party/blink/public/platform/web_encrypted_media_types.h"
#include "third_party/blink/public/platform/web_media_player_encrypted_media_client.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class HTMLMediaElement;
class MediaKeys;
class ScriptPromise;
class ScriptState;
class WebContentDecryptionModule;

class MODULES_EXPORT HTMLMediaElementEncryptedMedia final
    : public GarbageCollectedFinalized<HTMLMediaElementEncryptedMedia>,
      public Supplement<HTMLMediaElement>,
      public WebMediaPlayerEncryptedMediaClient {
  USING_GARBAGE_COLLECTED_MIXIN(HTMLMediaElementEncryptedMedia);

 public:
  static const char kSupplementName[];

  static MediaKeys* mediaKeys(HTMLMediaElement&);
  static ScriptPromise setMediaKeys(ScriptState*,
                                    HTMLMediaElement&,
                                    MediaKeys*);
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(encrypted);
  DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(waitingforkey);

  // WebMediaPlayerEncryptedMediaClient methods
  void Encrypted(WebEncryptedMediaInitDataType,
                 const unsigned char* init_data,
                 unsigned init_data_length) final;
  void DidBlockPlaybackWaitingForKey() final;
  void DidResumePlaybackBlockedForKey() final;
  WebContentDecryptionModule* ContentDecryptionModule();

  static HTMLMediaElementEncryptedMedia& From(HTMLMediaElement&);

  ~HTMLMediaElementEncryptedMedia();

  void Trace(blink::Visitor*) override;

 private:
  friend class SetMediaKeysHandler;

  HTMLMediaElementEncryptedMedia(HTMLMediaElement&);

  // EventTarget
  bool SetAttributeEventListener(const AtomicString& event_type,
                                 EventListener*);
  EventListener* GetAttributeEventListener(const AtomicString& event_type);

  Member<HTMLMediaElement> media_element_;

  // Internal values specified by the EME spec:
  // http://w3c.github.io/encrypted-media/#idl-def-HTMLMediaElement
  // The following internal values are added to the HTMLMediaElement:
  // - waiting for key, which shall have a boolean value
  // - attaching media keys, which shall have a boolean value
  bool is_waiting_for_key_;
  bool is_attaching_media_keys_;

  Member<MediaKeys> media_keys_;
};

}  // namespace blink

#endif
