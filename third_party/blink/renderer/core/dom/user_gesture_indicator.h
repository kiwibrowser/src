// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_USER_GESTURE_INDICATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_USER_GESTURE_INDICATOR_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

// A UserGestureToken represents the current state of a user gesture. It can be
// retrieved from a UserGestureIndicator to save for later (see, e.g., DOMTimer,
// which propagates user gestures to the timer fire in certain situations).
// Passing it to a UserGestureIndicator later on will cause it to be considered
// as currently being processed.
class CORE_EXPORT UserGestureToken : public RefCounted<UserGestureToken> {
  friend class UserGestureIndicator;

 public:
  enum Status { kNewGesture, kPossiblyExistingGesture };
  enum TimeoutPolicy { kDefault, kOutOfProcess, kHasPaused };

  ~UserGestureToken() = default;

  // TODO(mustaq): The only user of this method is PepperPluginInstanceImpl.  We
  // need to investigate the usecase closely.
  bool HasGestures() const;

 private:
  UserGestureToken(Status);

  void TransferGestureTo(UserGestureToken*);
  bool ConsumeGesture();
  void SetTimeoutPolicy(TimeoutPolicy);
  void ResetTimestamp();
  bool HasTimedOut() const;
  bool WasForwardedCrossProcess() const;
  void SetWasForwardedCrossProcess();

  size_t consumable_gestures_;
  double timestamp_;
  TimeoutPolicy timeout_policy_;
  bool was_forwarded_cross_process_;
  DISALLOW_COPY_AND_ASSIGN(UserGestureToken);
};

class CORE_EXPORT UserGestureIndicator final {
  USING_FAST_MALLOC(UserGestureIndicator);

 public:
  // Note: All *ThreadSafe methods are safe to call from any thread. Their
  // non-suffixed counterparts *must* be called on the main thread. Consider
  // always using the non-suffixed one unless the code really
  // needs to be thread-safe

  // Returns whether a user gesture is currently in progress.
  static bool ProcessingUserGesture();
  static bool ProcessingUserGestureThreadSafe();

  // Mark the current user gesture (if any) as having been used, such that
  // it cannot be used again.  This is done only for very security-sensitive
  // operations like creating a new process.
  static bool ConsumeUserGesture();
  static bool ConsumeUserGestureThreadSafe();

  static UserGestureToken* CurrentToken();
  static UserGestureToken* CurrentTokenThreadSafe();

  static void SetTimeoutPolicy(UserGestureToken::TimeoutPolicy);

  // Temporarily track whether a given user gesture has been forwarded to a
  // cross-process subframe (e.g., via postMessage).  This prevents forwarding
  // an unbounded number of gestures using OOPIFs.
  //
  // TODO(alexmos, mustaq): Remove this once either (1) browser process tracks
  // and coordinates user gestures (see http://crbug.com/161068), or (2)
  // UserActivation v2 ships and supports OOPIFs (see https://crbug.com/696617
  // and https://crbug.com/780556).
  static bool WasForwardedCrossProcess();
  static void SetWasForwardedCrossProcess();

  explicit UserGestureIndicator(scoped_refptr<UserGestureToken>);

  // Constructs a UserGestureIndicator with a new UserGestureToken of the given
  // status.
  explicit UserGestureIndicator(
      UserGestureToken::Status = UserGestureToken::kPossiblyExistingGesture);
  ~UserGestureIndicator();

 private:
  void UpdateRootToken();

  static UserGestureToken* root_token_;

  scoped_refptr<UserGestureToken> token_;
  DISALLOW_COPY_AND_ASSIGN(UserGestureIndicator);
};

}  // namespace blink

#endif
