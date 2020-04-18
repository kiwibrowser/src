/*
 * Copyright (C) 2012 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USE_COUNTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USE_COUNTER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_mode.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/wtf/bit_vector.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class CSSStyleSheet;
class Document;
class EnumerationHistogram;
class ExecutionContext;
class LocalFrame;
class StyleSheetContents;
// Definition for UseCounter features can be found in:
// third_party/blink/public/platform/web_feature.mojom

// UseCounter is used for counting the number of times features of
// Blink are used on real web pages and help us know commonly
// features are used and thus when it's safe to remove or change them.
//
// The Chromium Content layer controls what is done with this data.
//
// For instance, in Google Chrome, these counts are submitted anonymously
// through the UMA histogram recording system in Chrome for users who have the
// "Automatically send usage statistics and crash reports to Google" setting
// enabled:
// http://www.google.com/chrome/intl/en/privacy.html
//
// Changes on UseCounter are observable by UseCounter::Observer.
class CORE_EXPORT UseCounter {
  DISALLOW_NEW();

 public:
  // The context determines whether a feature is reported to UMA histograms. For
  // example, when the context is set to kDisabledContext, no features will be
  // reported to UMA, but features may still be marked as seen to avoid multiple
  // console warnings for deprecation.
  enum Context {
    kDefaultContext,
    // Counters for SVGImages (lifetime independent from other pages).
    kSVGImageContext,
    // Counters for extensions.
    kExtensionContext,
    // Context when counters should be disabled (eg, internal pages such as
    // about, chrome-devtools, etc).
    kDisabledContext
  };

  UseCounter(Context = kDefaultContext);

  // An interface to observe UseCounter changes. Note that this is never
  // notified when the counter is disabled by |m_muteCount| or when |m_context|
  // is kDisabledContext.
  class Observer : public GarbageCollected<Observer> {
   public:
    // Notified when a feature is counted for the first time. This should return
    // true if it no longer needs to observe changes so that the counter can
    // remove a reference to the observer and stop notifications.
    virtual bool OnCountFeature(WebFeature) = 0;

    virtual void Trace(blink::Visitor* visitor) {}
  };

  // "count" sets the bit for this feature to 1. Repeated calls are ignored.
  static void Count(const LocalFrame*, WebFeature);
  static void Count(const Document&, WebFeature);
  static void Count(ExecutionContext*, WebFeature);

  void Count(CSSParserMode, CSSPropertyID, const LocalFrame*);
  void Count(WebFeature, const LocalFrame*);

  static void CountAnimatedCSS(const Document&, CSSPropertyID);
  void CountAnimatedCSS(CSSPropertyID, const LocalFrame*);

  // Count only features if they're being used in an iframe which does not
  // have script access into the top level document.
  static void CountCrossOriginIframe(const Document&, WebFeature);

  // Return whether the Feature was previously counted for this document.
  // NOTE: only for use in testing.
  static bool IsCounted(Document&, WebFeature);
  // Return whether the CSSPropertyID was previously counted for this document.
  // NOTE: only for use in testing.
  static bool IsCounted(Document&, const String&);
  bool IsCounted(CSSPropertyID unresolved_property);

  // Return whether the CSSPropertyID was previously counted for this document.
  // NOTE: only for use in testing.
  static bool IsCountedAnimatedCSS(Document&, const String&);
  bool IsCountedAnimatedCSS(CSSPropertyID unresolved_property);

  // Retains a reference to the observer to notify of UseCounter changes.
  void AddObserver(Observer*);

  // Invoked when a new document is loaded into the main frame of the page.
  void DidCommitLoad(const LocalFrame*);

  static int MapCSSPropertyIdToCSSSampleIdForHistogram(CSSPropertyID);

  // When muted, all calls to "count" functions are ignoed.  May be nested.
  void MuteForInspector();
  void UnmuteForInspector();

  void RecordMeasurement(WebFeature, const LocalFrame&);

  // Return whether the feature has been seen since the last page load
  // (except when muted).  Does include features seen in documents which have
  // reporting disabled.
  bool HasRecordedMeasurement(WebFeature) const;

  // Triggers a use counter if a feature, which is currently available in all
  // frames, would be blocked by the introduction of feature policy. This takes
  // two counters (which may be the same). It triggers |blockedCrossOrigin| if
  // the frame is cross-origin relative to the top-level document, and triggers
  // |blockedSameOrigin| if it is same-origin with the top level, but is
  // embedded in any way through a cross-origin frame. (A->B->A embedding)
  static void CountIfFeatureWouldBeBlockedByFeaturePolicy(
      const LocalFrame&,
      WebFeature blockedCrossOrigin,
      WebFeature blockedSameOrigin);

  void Trace(blink::Visitor*);

 private:
  // Notifies that a feature is newly counted to |m_observers|. This shouldn't
  // be called when the counter is disabled by |m_muteCount| or when |m_context|
  // if kDisabledContext.
  void NotifyFeatureCounted(WebFeature);

  EnumerationHistogram& FeaturesHistogram() const;
  EnumerationHistogram& CssHistogram() const;
  EnumerationHistogram& AnimatedCSSHistogram() const;

  // If non-zero, ignore all 'count' calls completely.
  unsigned mute_count_;

  // The scope represented by this UseCounter instance, which must be fixed for
  // the duration of a page but can change when a new page is loaded.
  Context context_;

  // Track what features/properties have been reported to the histograms.
  BitVector features_recorded_;
  BitVector css_recorded_;
  BitVector animated_css_recorded_;

  HeapHashSet<Member<Observer>> observers_;

  DISALLOW_COPY_AND_ASSIGN(UseCounter);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USE_COUNTER_H_
