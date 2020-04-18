// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_AD_TRACKER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_AD_TRACKER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "v8/include/v8.h"

namespace blink {
class ExecutionContext;
class DocumentLoader;
class ResourceRequest;
class ResourceResponse;
struct FetchInitiatorInfo;

namespace probe {
class CallFunction;
class ExecuteScript;
}  // namespace probe

// Tracker for tagging resources as ads based on the call stack scripts.
// The tracker is maintained per local root.
class CORE_EXPORT AdTracker : public GarbageCollectedFinalized<AdTracker> {
 public:
  // Instrumenting methods.
  // Called when a script module or script gets executed from native code.
  void Will(const probe::ExecuteScript&);
  void Did(const probe::ExecuteScript&);

  // Called when a function gets called from native code.
  void Will(const probe::CallFunction&);
  void Did(const probe::CallFunction&);

  // Called when a resource request is about to be sent. This will do the
  // following:
  // - Mark a resource request as an ad if any executing scripts contain an ad.
  // - If the marked resource is a script, also save it to keep track of all
  // those script resources that have been identified as ads.
  void WillSendRequest(ExecutionContext*,
                       unsigned long identifier,
                       DocumentLoader*,
                       ResourceRequest&,
                       const ResourceResponse& redirect_response,
                       const FetchInitiatorInfo&,
                       Resource::Type);

  // Returns true if any script in the pseudo call stack have previously been
  // identified as an ad resource.
  bool IsAdScriptInStack(ExecutionContext*);

  virtual void Trace(blink::Visitor*);

  void Shutdown();
  explicit AdTracker(LocalFrame*);
  virtual ~AdTracker();

 protected:
  // Protected for testing.
  virtual String ScriptAtTopOfStack(ExecutionContext*);

 private:
  friend class FrameFetchContextSubresourceFilterTest;
  friend class AdTrackerTest;

  void WillExecuteScript(const String& script_name);
  void DidExecuteScript();
  void AppendToKnownAdScripts(const KURL&);

  Member<LocalFrame> local_root_;

  // Since the script URLs should be external strings in v8 (allocated in Blink)
  // getting it as String should end up with the same StringImpl. Thus storing a
  // vector of Strings here should not be expensive.
  struct ExecutingScript {
    String url;
    bool is_ad;
    ExecutingScript(String script_url, bool is_ad_script)
        : url(script_url), is_ad(is_ad_script){};
  };
  Vector<ExecutingScript> executing_scripts_;
  HashSet<String> known_ad_scripts_;

  DISALLOW_COPY_AND_ASSIGN(AdTracker);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_AD_TRACKER_H_
