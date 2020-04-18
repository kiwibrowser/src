/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/page/validation_message_client_impl.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/web/web_text_direction.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/validation_message_overlay_delegate.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/platform_chrome_client.h"

namespace blink {

ValidationMessageClientImpl::ValidationMessageClientImpl(WebViewImpl& web_view)
    : web_view_(web_view),
      current_anchor_(nullptr),
      finish_time_(0) {}

ValidationMessageClientImpl* ValidationMessageClientImpl::Create(
    WebViewImpl& web_view) {
  return new ValidationMessageClientImpl(web_view);
}

ValidationMessageClientImpl::~ValidationMessageClientImpl() = default;

LocalFrameView* ValidationMessageClientImpl::CurrentView() {
  return current_anchor_->GetDocument().View();
}

void ValidationMessageClientImpl::ShowValidationMessage(
    const Element& anchor,
    const String& message,
    TextDirection message_dir,
    const String& sub_message,
    TextDirection sub_message_dir) {
  if (message.IsEmpty()) {
    HideValidationMessage(anchor);
    return;
  }
  if (!anchor.GetLayoutBox())
    return;
  if (current_anchor_)
    HideValidationMessageImmediately(*current_anchor_);
  current_anchor_ = &anchor;
  message_ = message;
  web_view_.GetChromeClient().RegisterPopupOpeningObserver(this);
  const double kMinimumSecondToShowValidationMessage = 5.0;
  const double kSecondPerCharacter = 0.05;
  finish_time_ =
      CurrentTimeTicksInSeconds() +
      std::max(kMinimumSecondToShowValidationMessage,
               (message.length() + sub_message.length()) * kSecondPerCharacter);

  auto* target_frame =
      web_view_.MainFrameImpl()
          ? web_view_.MainFrameImpl()
          : WebLocalFrameImpl::FromFrame(anchor.GetDocument().GetFrame());
  auto delegate = ValidationMessageOverlayDelegate::Create(
      *web_view_.GetPage(), anchor, message_, message_dir, sub_message,
      sub_message_dir);
  overlay_delegate_ = delegate.get();
  overlay_ = PageOverlay::Create(target_frame, std::move(delegate));
  bool success = target_frame->GetFrameView()
                     ->UpdateLifecycleToCompositingCleanPlusScrolling();
  // The lifecycle update should always succeed, because this is not inside
  // of a throttling scope.
  DCHECK(success);
  LayoutOverlay();
}

void ValidationMessageClientImpl::HideValidationMessage(const Element& anchor) {
  if (LayoutTestSupport::IsRunningLayoutTest()) {
    HideValidationMessageImmediately(anchor);
    return;
  }
  if (!current_anchor_ || !IsValidationMessageVisible(anchor))
    return;
  DCHECK(overlay_);
  overlay_delegate_->StartToHide();
  timer_ = std::make_unique<TaskRunnerTimer<ValidationMessageClientImpl>>(
      anchor.GetDocument().GetTaskRunner(TaskType::kInternalDefault), this,
      &ValidationMessageClientImpl::Reset);
  // This should be equal to or larger than transition duration of
  // #container.hiding in validation_bubble.css.
  const double kHidingAnimationDuration = 0.13333;
  timer_->StartOneShot(kHidingAnimationDuration, FROM_HERE);
}

void ValidationMessageClientImpl::HideValidationMessageImmediately(
    const Element& anchor) {
  if (!current_anchor_ || !IsValidationMessageVisible(anchor))
    return;
  Reset(nullptr);
}

void ValidationMessageClientImpl::Reset(TimerBase*) {
  timer_ = nullptr;
  current_anchor_ = nullptr;
  message_ = String();
  finish_time_ = 0;
  overlay_ = nullptr;
  overlay_delegate_ = nullptr;
  web_view_.GetChromeClient().UnregisterPopupOpeningObserver(this);
}

bool ValidationMessageClientImpl::IsValidationMessageVisible(
    const Element& anchor) {
  return current_anchor_ == &anchor;
}

void ValidationMessageClientImpl::DocumentDetached(const Document& document) {
  if (current_anchor_ && current_anchor_->GetDocument() == document)
    HideValidationMessageImmediately(*current_anchor_);
}

void ValidationMessageClientImpl::CheckAnchorStatus(TimerBase*) {
  DCHECK(current_anchor_);
  if ((!LayoutTestSupport::IsRunningLayoutTest() &&
       CurrentTimeTicksInSeconds() >= finish_time_) ||
      !CurrentView()) {
    HideValidationMessage(*current_anchor_);
    return;
  }

  IntRect new_anchor_rect_in_viewport =
      current_anchor_->VisibleBoundsInVisualViewport();
  if (new_anchor_rect_in_viewport.IsEmpty()) {
    HideValidationMessage(*current_anchor_);
    return;
  }
}

void ValidationMessageClientImpl::WillBeDestroyed() {
  if (current_anchor_)
    HideValidationMessageImmediately(*current_anchor_);
}

void ValidationMessageClientImpl::WillOpenPopup() {
  if (current_anchor_)
    HideValidationMessage(*current_anchor_);
}

void ValidationMessageClientImpl::LayoutOverlay() {
  if (!overlay_)
    return;
  CheckAnchorStatus(nullptr);
  if (overlay_)
    overlay_->Update();
}

void ValidationMessageClientImpl::PaintOverlay() {
  if (overlay_)
    overlay_->GetGraphicsLayer()->Paint(nullptr);
}

void ValidationMessageClientImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(current_anchor_);
  ValidationMessageClient::Trace(visitor);
}

}  // namespace blink
