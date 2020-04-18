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

#include "third_party/blink/renderer/core/html/media/media_document.h"

#include "base/macros.h"
#include "third_party/blink/renderer/bindings/core/v8/add_event_listener_options_or_boolean.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/dom/raw_data_document_parser.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/events/keyboard_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"
#include "third_party/blink/renderer/core/html/html_meta_element.h"
#include "third_party/blink/renderer/core/html/html_source_element.h"
#include "third_party/blink/renderer/core/html/media/autoplay_policy.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/platform/keyboard_codes.h"

namespace blink {

using namespace HTMLNames;

// FIXME: Share more code with PluginDocumentParser.
class MediaDocumentParser : public RawDataDocumentParser {
 public:
  static MediaDocumentParser* Create(MediaDocument* document) {
    return new MediaDocumentParser(document);
  }

 private:
  explicit MediaDocumentParser(Document* document)
      : RawDataDocumentParser(document), did_build_document_structure_(false) {}

  void AppendBytes(const char*, size_t) override;

  void CreateDocumentStructure();

  bool did_build_document_structure_;
};

void MediaDocumentParser::CreateDocumentStructure() {
  DCHECK(GetDocument());
  HTMLHtmlElement* root_element = HTMLHtmlElement::Create(*GetDocument());
  GetDocument()->AppendChild(root_element);
  root_element->InsertedByParser();

  if (IsDetached())
    return;  // runScriptsAtDocumentElementAvailable can detach the frame.

  HTMLHeadElement* head = HTMLHeadElement::Create(*GetDocument());
  HTMLMetaElement* meta = HTMLMetaElement::Create(*GetDocument());
  meta->setAttribute(nameAttr, "viewport");
  meta->setAttribute(contentAttr, "width=device-width");
  head->AppendChild(meta);

  HTMLVideoElement* media = HTMLVideoElement::Create(*GetDocument());
  media->setAttribute(controlsAttr, "");
  media->setAttribute(autoplayAttr, "");
  media->setAttribute(nameAttr, "media");

  HTMLSourceElement* source = HTMLSourceElement::Create(*GetDocument());
  source->SetSrc(GetDocument()->Url());

  if (DocumentLoader* loader = GetDocument()->Loader())
    source->setType(loader->MimeType());

  media->AppendChild(source);

  HTMLBodyElement* body = HTMLBodyElement::Create(*GetDocument());

  GetDocument()->WillInsertBody();

  body->AppendChild(media);
  root_element->AppendChild(head);
  if (IsDetached())
    return;  // DOM insertion events can detach the frame.
  root_element->AppendChild(body);

  did_build_document_structure_ = true;
}

void MediaDocumentParser::AppendBytes(const char*, size_t) {
  if (did_build_document_structure_)
    return;

  CreateDocumentStructure();
  Finish();
}

MediaDocument::MediaDocument(const DocumentInit& initializer)
    : HTMLDocument(initializer, kMediaDocumentClass) {
  SetCompatibilityMode(kQuirksMode);
  LockCompatibilityMode();

  // Set the autoplay policy to kNoUserGestureRequired.
  if (GetSettings()) {
    GetSettings()->SetAutoplayPolicy(
        AutoplayPolicy::Type::kNoUserGestureRequired);
  }
}

DocumentParser* MediaDocument::CreateParser() {
  return MediaDocumentParser::Create(this);
}

void MediaDocument::DefaultEventHandler(Event* event) {
  Node* target_node = event->target()->ToNode();
  if (!target_node)
    return;

  if (event->type() == EventTypeNames::keydown && event->IsKeyboardEvent()) {
    HTMLVideoElement* video =
        Traversal<HTMLVideoElement>::FirstWithin(*target_node);
    if (!video)
      return;

    KeyboardEvent* keyboard_event = ToKeyboardEvent(event);
    if (keyboard_event->key() == " " ||
        keyboard_event->keyCode() == VKEY_MEDIA_PLAY_PAUSE) {
      // space or media key (play/pause)
      video->TogglePlayState();
      event->SetDefaultHandled();
    }
  }
}

}  // namespace blink
