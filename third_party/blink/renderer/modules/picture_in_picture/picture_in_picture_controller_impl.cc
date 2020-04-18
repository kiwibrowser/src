// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/picture_in_picture/picture_in_picture_controller_impl.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/modules/picture_in_picture/picture_in_picture_window.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"

namespace blink {

PictureInPictureControllerImpl::~PictureInPictureControllerImpl() = default;

// static
PictureInPictureControllerImpl* PictureInPictureControllerImpl::Create(
    Document& document) {
  return new PictureInPictureControllerImpl(document);
}

// static
PictureInPictureControllerImpl& PictureInPictureControllerImpl::From(
    Document& document) {
  return static_cast<PictureInPictureControllerImpl&>(
      PictureInPictureController::From(document));
}

bool PictureInPictureControllerImpl::PictureInPictureEnabled() const {
  return IsDocumentAllowed() == Status::kEnabled;
}

PictureInPictureController::Status
PictureInPictureControllerImpl::IsDocumentAllowed() const {
  DCHECK(GetSupplementable());

  // If document has been detached from a frame, return kFrameDetached status.
  LocalFrame* frame = GetSupplementable()->GetFrame();
  if (!frame)
    return Status::kFrameDetached;

  // `GetPictureInPictureEnabled()` returns false when the embedder or the
  // system forbids the page from using Picture-in-Picture.
  DCHECK(GetSupplementable()->GetSettings());
  if (!GetSupplementable()->GetSettings()->GetPictureInPictureEnabled())
    return Status::kDisabledBySystem;

  // If document is not allowed to use the policy-controlled feature named
  // "picture-in-picture", return kDisabledByFeaturePolicy status.
  if (IsSupportedInFeaturePolicy(
          blink::mojom::FeaturePolicyFeature::kPictureInPicture) &&
      !frame->IsFeatureEnabled(
          blink::mojom::FeaturePolicyFeature::kPictureInPicture)) {
    return Status::kDisabledByFeaturePolicy;
  }

  return Status::kEnabled;
}

PictureInPictureController::Status
PictureInPictureControllerImpl::IsElementAllowed(
    const HTMLVideoElement& element) const {
  PictureInPictureController::Status status = IsDocumentAllowed();
  if (status != Status::kEnabled)
    return status;

  if (element.getReadyState() == HTMLMediaElement::kHaveNothing)
    return Status::kMetadataNotLoaded;

  if (!element.HasVideo())
    return Status::kVideoTrackNotAvailable;

  if (element.FastHasAttribute(HTMLNames::disablepictureinpictureAttr))
    return Status::kDisabledByAttribute;

  return Status::kEnabled;
}

void PictureInPictureControllerImpl::EnterPictureInPicture(
    HTMLVideoElement* element,
    ScriptPromiseResolver* resolver) {
  element->enterPictureInPicture(WTF::Bind(
      &PictureInPictureControllerImpl::OnEnteredPictureInPicture,
      WrapPersistent(this), WrapPersistent(element), WrapPersistent(resolver)));
}

void PictureInPictureControllerImpl::OnEnteredPictureInPicture(
    HTMLVideoElement* element,
    ScriptPromiseResolver* resolver,
    const WebSize& picture_in_picture_window_size) {
  if (IsElementAllowed(*element) == Status::kDisabledByAttribute) {
    if (resolver)
      resolver->Reject(DOMException::Create(kInvalidStateError, ""));
    // TODO(crbug.com/806249): Test that WMPI sends the message.
    element->exitPictureInPicture(base::DoNothing());
    return;
  }

  picture_in_picture_element_ = element;

  picture_in_picture_element_->OnEnteredPictureInPicture();

  picture_in_picture_element_->DispatchEvent(
      Event::CreateBubble(EventTypeNames::enterpictureinpicture));

  // Closes the current Picture-in-Picture window if any.
  if (picture_in_picture_window_)
    picture_in_picture_window_->OnClose();

  picture_in_picture_window_ = new PictureInPictureWindow(
      GetSupplementable(), picture_in_picture_window_size);

  element->GetWebMediaPlayer()->RegisterPictureInPictureWindowResizeCallback(
      WTF::BindRepeating(&PictureInPictureWindow::OnResize,
                         WrapPersistent(picture_in_picture_window_.Get())));

  if (resolver)
    resolver->Resolve(picture_in_picture_window_);
}

void PictureInPictureControllerImpl::ExitPictureInPicture(
    HTMLVideoElement* element,
    ScriptPromiseResolver* resolver) {
  element->exitPictureInPicture(
      WTF::Bind(&PictureInPictureControllerImpl::OnExitedPictureInPicture,
                WrapPersistent(this), WrapPersistent(resolver)));
}

void PictureInPictureControllerImpl::OnExitedPictureInPicture(
    ScriptPromiseResolver* resolver) {
  DCHECK(GetSupplementable());

  // Bail out if document is not active.
  if (!GetSupplementable()->IsActive())
    return;

  if (picture_in_picture_window_)
    picture_in_picture_window_->OnClose();

  if (picture_in_picture_element_) {
    HTMLVideoElement* element = picture_in_picture_element_;
    picture_in_picture_element_ = nullptr;

    element->OnExitedPictureInPicture();
    element->DispatchEvent(
        Event::CreateBubble(EventTypeNames::leavepictureinpicture));
  }

  if (resolver)
    resolver->Resolve();
}

Element* PictureInPictureControllerImpl::PictureInPictureElement(
    TreeScope& scope) const {
  if (!picture_in_picture_element_)
    return nullptr;

  return scope.AdjustedElement(*picture_in_picture_element_);
}

bool PictureInPictureControllerImpl::IsPictureInPictureElement(
    const Element* element) const {
  if (!element)
    return false;
  return element == picture_in_picture_element_;
}

void PictureInPictureControllerImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(picture_in_picture_element_);
  visitor->Trace(picture_in_picture_window_);
  Supplement<Document>::Trace(visitor);
}

PictureInPictureControllerImpl::PictureInPictureControllerImpl(
    Document& document)
    : PictureInPictureController(document) {}

}  // namespace blink
