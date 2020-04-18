// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FRAME_REQUEST_CALLBACK_COLLECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FRAME_REQUEST_CALLBACK_COLLECTION_H_

#include "third_party/blink/renderer/bindings/core/v8/v8_frame_request_callback.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ExecutionContext;

class CORE_EXPORT FrameRequestCallbackCollection final
    : public TraceWrapperBase {
  DISALLOW_NEW();

 public:
  explicit FrameRequestCallbackCollection(ExecutionContext*);

  using CallbackId = int;

  // |FrameCallback| is an interface type which generalizes callbacks which are
  // invoked when a script-based animation needs to be resampled.
  class CORE_EXPORT FrameCallback
      : public GarbageCollectedFinalized<FrameCallback>,
        public TraceWrapperBase {
   public:
    virtual void Trace(blink::Visitor* visitor) {}
    void TraceWrappers(ScriptWrappableVisitor* visitor) const override {}
    const char* NameInHeapSnapshot() const override { return "FrameCallback"; }
    virtual ~FrameCallback() = default;
    virtual void Invoke(double) = 0;

    int Id() const { return id_; }
    bool IsCancelled() const { return is_cancelled_; }
    bool GetUseLegacyTimeBase() const { return use_legacy_time_base_; }
    void SetId(int id) { id_ = id; }
    void SetIsCancelled(bool is_cancelled) { is_cancelled_ = is_cancelled; }
    void SetUseLegacyTimeBase(bool use_legacy_time_base) {
      use_legacy_time_base_ = use_legacy_time_base;
    }

   protected:
    FrameCallback() = default;

   private:
    int id_ = 0;
    bool is_cancelled_ = false;
    bool use_legacy_time_base_ = false;
  };

  // |V8FrameCallback| is an adapter class for the conversion from
  // |V8FrameRequestCallback| to |Framecallback|.
  class CORE_EXPORT V8FrameCallback : public FrameCallback {
   public:
    static V8FrameCallback* Create(V8FrameRequestCallback* callback) {
      return new V8FrameCallback(callback);
    }
    void Trace(blink::Visitor*) override;
    void TraceWrappers(ScriptWrappableVisitor*) const override;
    const char* NameInHeapSnapshot() const override {
      return "V8FrameCallback";
    }
    ~V8FrameCallback() override = default;
    void Invoke(double) override;

   private:
    explicit V8FrameCallback(V8FrameRequestCallback*);
    TraceWrapperMember<V8FrameRequestCallback> callback_;
  };

  CallbackId RegisterCallback(FrameCallback*);
  void CancelCallback(CallbackId);
  void ExecuteCallbacks(double high_res_now_ms, double high_res_now_ms_legacy);

  bool IsEmpty() const { return !callbacks_.size(); }

  void Trace(blink::Visitor*);
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "FrameRequestCallbackCollection";
  }

 private:
  using CallbackList = HeapVector<TraceWrapperMember<FrameCallback>>;
  CallbackList callbacks_;
  CallbackList
      callbacks_to_invoke_;  // only non-empty while inside executeCallbacks

  CallbackId next_callback_id_ = 0;

  Member<ExecutionContext> context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FRAME_REQUEST_CALLBACK_COLLECTION_H_
