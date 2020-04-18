// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/media/picture_in_picture_interstitial.h"

#include "cc/layers/layer.h"
#include "third_party/blink/public/platform/web_localized_string.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"

namespace {

constexpr double kPictureInPictureStyleChangeTransSeconds = 0.2;
constexpr double kPictureInPictureHiddenAnimationSeconds = 0.3;

}  // namespace

namespace blink {

PictureInPictureInterstitial::PictureInPictureInterstitial(
    HTMLVideoElement& videoElement)
    : HTMLDivElement(videoElement.GetDocument()),
      interstitial_timer_(
          videoElement.GetDocument().GetTaskRunner(TaskType::kInternalMedia),
          this,
          &PictureInPictureInterstitial::ToggleInterstitialTimerFired),
      video_element_(&videoElement) {
  SetShadowPseudoId(AtomicString("-internal-media-interstitial"));
  background_image_ = HTMLImageElement::Create(GetDocument());
  background_image_->SetShadowPseudoId(
      AtomicString("-internal-media-interstitial-background-image"));
  background_image_->SetSrc(videoElement.getAttribute(HTMLNames::posterAttr));
  AppendChild(background_image_);

  pip_icon_ = HTMLDivElement::Create(GetDocument());
  pip_icon_->SetShadowPseudoId(
      AtomicString("-internal-picture-in-picture-icon"));
  AppendChild(pip_icon_);

  HTMLDivElement* message_element_ = HTMLDivElement::Create(GetDocument());
  message_element_->SetShadowPseudoId(
      AtomicString("-internal-media-interstitial-message"));
  message_element_->setInnerText(
      GetVideoElement().GetLocale().QueryString(
          WebLocalizedString::kPictureInPictureInterstitialText),
      ASSERT_NO_EXCEPTION);
  AppendChild(message_element_);
}

void PictureInPictureInterstitial::Show() {
  if (should_be_visible_)
    return;

  if (interstitial_timer_.IsActive())
    interstitial_timer_.Stop();
  should_be_visible_ = true;
  RemoveInlineStyleProperty(CSSPropertyDisplay);
  interstitial_timer_.StartOneShot(kPictureInPictureStyleChangeTransSeconds,
                                   FROM_HERE);

  DCHECK(GetVideoElement().CcLayer());
  GetVideoElement().CcLayer()->SetIsDrawable(false);
}

void PictureInPictureInterstitial::Hide() {
  if (!should_be_visible_)
    return;

  if (interstitial_timer_.IsActive())
    interstitial_timer_.Stop();
  should_be_visible_ = false;
  SetInlineStyleProperty(CSSPropertyOpacity, 0,
                         CSSPrimitiveValue::UnitType::kNumber);
  interstitial_timer_.StartOneShot(kPictureInPictureHiddenAnimationSeconds,
                                   FROM_HERE);

  if (GetVideoElement().CcLayer())
    GetVideoElement().CcLayer()->SetIsDrawable(true);
}

void PictureInPictureInterstitial::ToggleInterstitialTimerFired(TimerBase*) {
  interstitial_timer_.Stop();
  if (should_be_visible_) {
    SetInlineStyleProperty(CSSPropertyBackgroundColor, CSSValueBlack);
    SetInlineStyleProperty(CSSPropertyOpacity, 1,
                           CSSPrimitiveValue::UnitType::kNumber);
  } else {
    SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
  }
}

void PictureInPictureInterstitial::OnPosterImageChanged() {
  background_image_->SetSrc(
      GetVideoElement().getAttribute(HTMLNames::posterAttr));
}

void PictureInPictureInterstitial::Trace(blink::Visitor* visitor) {
  visitor->Trace(video_element_);
  visitor->Trace(background_image_);
  visitor->Trace(pip_icon_);
  visitor->Trace(message_element_);
  HTMLDivElement::Trace(visitor);
}

}  // namespace blink
