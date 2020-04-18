// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

// User gestures timeout in 1 second.
const double kUserGestureTimeout = 1.0;

// For out of process tokens we allow a 10 second delay.
const double kUserGestureOutOfProcessTimeout = 10.0;

UserGestureToken::UserGestureToken(Status status)
    : consumable_gestures_(0),
      timestamp_(WTF::CurrentTime()),
      timeout_policy_(kDefault),
      was_forwarded_cross_process_(false) {
  if (status == kNewGesture || !UserGestureIndicator::CurrentTokenThreadSafe())
    consumable_gestures_++;
}

bool UserGestureToken::HasGestures() const {
  return consumable_gestures_ && !HasTimedOut();
}

void UserGestureToken::TransferGestureTo(UserGestureToken* other) {
  if (!HasGestures())
    return;
  consumable_gestures_--;
  other->consumable_gestures_++;
}

bool UserGestureToken::ConsumeGesture() {
  if (!consumable_gestures_)
    return false;
  consumable_gestures_--;
  return true;
}

void UserGestureToken::SetTimeoutPolicy(TimeoutPolicy policy) {
  if (!HasTimedOut() && HasGestures() && policy > timeout_policy_)
    timeout_policy_ = policy;
}

void UserGestureToken::ResetTimestamp() {
  timestamp_ = WTF::CurrentTime();
}

bool UserGestureToken::HasTimedOut() const {
  if (timeout_policy_ == kHasPaused)
    return false;
  double timeout = timeout_policy_ == kOutOfProcess
                       ? kUserGestureOutOfProcessTimeout
                       : kUserGestureTimeout;
  return WTF::CurrentTime() - timestamp_ > timeout;
}

bool UserGestureToken::WasForwardedCrossProcess() const {
  return was_forwarded_cross_process_;
}

void UserGestureToken::SetWasForwardedCrossProcess() {
  was_forwarded_cross_process_ = true;
}

// This enum is used in a histogram, so its values shouldn't change.
enum GestureMergeState {
  kOldTokenHasGesture = 1 << 0,
  kNewTokenHasGesture = 1 << 1,
  kGestureMergeStateEnd = 1 << 2,
};

// Remove this when user gesture propagation is standardized. See
// https://crbug.com/404161.
static void RecordUserGestureMerge(const UserGestureToken& old_token,
                                   const UserGestureToken& new_token) {
  DEFINE_STATIC_LOCAL(EnumerationHistogram, gesture_merge_histogram,
                      ("Blink.Gesture.Merged", kGestureMergeStateEnd));
  int sample = 0;
  if (old_token.HasGestures())
    sample |= kOldTokenHasGesture;
  if (new_token.HasGestures())
    sample |= kNewTokenHasGesture;
  gesture_merge_histogram.Count(sample);
}

UserGestureToken* UserGestureIndicator::root_token_ = nullptr;

void UserGestureIndicator::UpdateRootToken() {
  if (!root_token_) {
    root_token_ = token_.get();
  } else {
    RecordUserGestureMerge(*root_token_, *token_);
    token_->TransferGestureTo(root_token_);
  }
}

UserGestureIndicator::UserGestureIndicator(
    scoped_refptr<UserGestureToken> token) {
  if (!IsMainThread() || !token || token == root_token_)
    return;
  token_ = std::move(token);
  token_->ResetTimestamp();
  UpdateRootToken();
}

UserGestureIndicator::UserGestureIndicator(UserGestureToken::Status status) {
  if (!IsMainThread())
    return;
  token_ = base::AdoptRef(new UserGestureToken(status));
  UpdateRootToken();
}

UserGestureIndicator::~UserGestureIndicator() {
  if (IsMainThread() && token_ && token_ == root_token_)
    root_token_ = nullptr;
}

// static
bool UserGestureIndicator::ProcessingUserGesture() {
  if (auto* token = CurrentToken())
    return token->HasGestures();
  return false;
}

// static
bool UserGestureIndicator::ProcessingUserGestureThreadSafe() {
  return IsMainThread() && ProcessingUserGesture();
}

// static
bool UserGestureIndicator::ConsumeUserGesture() {
  if (auto* token = CurrentToken()) {
    if (token->ConsumeGesture())
      return true;
  }
  return false;
}

// static
bool UserGestureIndicator::ConsumeUserGestureThreadSafe() {
  return IsMainThread() && ConsumeUserGesture();
}

// static
UserGestureToken* UserGestureIndicator::CurrentToken() {
  DCHECK(IsMainThread());
  return root_token_;
}

// static
UserGestureToken* UserGestureIndicator::CurrentTokenThreadSafe() {
  return IsMainThread() ? CurrentToken() : nullptr;
}

// static
void UserGestureIndicator::SetTimeoutPolicy(
    UserGestureToken::TimeoutPolicy policy) {
  if (auto* token = CurrentTokenThreadSafe())
    token->SetTimeoutPolicy(policy);
}

// static
bool UserGestureIndicator::WasForwardedCrossProcess() {
  if (auto* token = CurrentTokenThreadSafe())
    return token->WasForwardedCrossProcess();
  return false;
}

// static
void UserGestureIndicator::SetWasForwardedCrossProcess() {
  if (auto* token = CurrentTokenThreadSafe())
    token->SetWasForwardedCrossProcess();
}

}  // namespace blink
