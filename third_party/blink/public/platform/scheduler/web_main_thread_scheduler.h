// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_MAIN_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_MAIN_THREAD_SCHEDULER_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "third_party/blink/public/platform/scheduler/single_thread_idle_task_runner.h"
#include "third_party/blink/public/platform/scheduler/web_render_widget_scheduling_state.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_input_event_result.h"
#include "third_party/blink/public/platform/web_scoped_virtual_time_pauser.h"
#include "v8/include/v8.h"

namespace base {
namespace trace_event {
class BlameContext;
}
}  // namespace base

namespace blink {
class WebThread;
class WebInputEvent;
}  // namespace blink

namespace viz {
struct BeginFrameArgs;
}

namespace blink {
namespace scheduler {

enum class RendererProcessType;
class WebRenderWidgetSchedulingState;

class BLINK_PLATFORM_EXPORT WebMainThreadScheduler
    : virtual public WebThreadScheduler {
 public:
  class BLINK_PLATFORM_EXPORT RAILModeObserver {
   public:
    virtual ~RAILModeObserver();
    virtual void OnRAILModeChanged(v8::RAILMode rail_mode) = 0;
  };

  ~WebMainThreadScheduler() override;

  // If |initial_virtual_time| is specified then the scheduler will be created
  // with virtual time enabled and paused, and base::Time will be overridden to
  // start at |initial_virtual_time|.
  static std::unique_ptr<WebMainThreadScheduler> Create(
      base::Optional<base::Time> initial_virtual_time = base::nullopt);

  // Returns the compositor task runner.
  virtual scoped_refptr<base::SingleThreadTaskRunner>
  CompositorTaskRunner() = 0;

  // Returns the input task runner.
  virtual scoped_refptr<base::SingleThreadTaskRunner> InputTaskRunner() = 0;

  // Creates a WebThread implementation for the renderer main thread.
  virtual std::unique_ptr<WebThread> CreateMainThread() = 0;

  // Returns a new WebRenderWidgetSchedulingState.  The signals from this will
  // be used to make scheduling decisions.
  virtual std::unique_ptr<WebRenderWidgetSchedulingState>
  NewRenderWidgetSchedulingState() = 0;

  // Called to notify about the start of an extended period where no frames
  // need to be drawn. Must be called from the main thread.
  virtual void BeginFrameNotExpectedSoon() = 0;

  // Called to notify about the start of a period where main frames are not
  // scheduled and so short idle work can be scheduled. This will precede
  // BeginFrameNotExpectedSoon and is also called when the compositor may be
  // busy but the main thread is not.
  virtual void BeginMainFrameNotExpectedUntil(base::TimeTicks time) = 0;

  // Called to notify about the start of a new frame.  Must be called from the
  // main thread.
  virtual void WillBeginFrame(const viz::BeginFrameArgs& args) = 0;

  // Called to notify that a previously begun frame was committed. Must be
  // called from the main thread.
  virtual void DidCommitFrameToCompositor() = 0;

  // Keep InputEventStateToString() in sync with this enum.
  enum class InputEventState {
    EVENT_CONSUMED_BY_COMPOSITOR,
    EVENT_FORWARDED_TO_MAIN_THREAD,
  };
  static const char* InputEventStateToString(InputEventState input_event_state);

  // Tells the scheduler that the system processed an input event. Called by the
  // compositor (impl) thread.  Note it's expected that every call to
  // DidHandleInputEventOnCompositorThread where |event_state| is
  // EVENT_FORWARDED_TO_MAIN_THREAD will be followed by a corresponding call
  // to DidHandleInputEventOnMainThread.
  virtual void DidHandleInputEventOnCompositorThread(
      const WebInputEvent& web_input_event,
      InputEventState event_state) = 0;

  // Tells the scheduler that the system processed an input event. Must be
  // called from the main thread.
  virtual void DidHandleInputEventOnMainThread(
      const WebInputEvent& web_input_event,
      WebInputEventResult result) = 0;

  // Tells the scheduler that the system is displaying an input animation (e.g.
  // a fling). Called by the compositor (impl) thread.
  virtual void DidAnimateForInputOnCompositorThread() = 0;

  // Tells the scheduler about the change of renderer visibility status (e.g.
  // "all widgets are hidden" condition). Used mostly for metric purposes.
  // Must be called on the main thread.
  virtual void SetRendererHidden(bool hidden) = 0;

  // Tells the scheduler about the change of renderer background status, i.e.,
  // there are no critical, user facing activities (visual, audio, etc...)
  // driven by this process. A stricter condition than |SetRendererHidden()|,
  // the process is assumed to be foregrounded when the scheduler is
  // constructed. Must be called on the main thread.
  virtual void SetRendererBackgrounded(bool backgrounded) = 0;

  // Tells the scheduler about "keep-alive" state which can be due to:
  // service workers, shared workers, or fetch keep-alive.
  // If set to true, then the scheduler should not freeze the renderer.
  virtual void SetSchedulerKeepActive(bool keep_active) = 0;

#if defined(OS_ANDROID)
  // Android WebView has very strange WebView.pauseTimers/resumeTimers API.
  // It's very old and very inconsistent. The API promises that this
  // "pauses all layout, parsing, and JavaScript timers for all WebViews".
  // Also CTS tests expect that loading tasks continue to run.
  // We should change it to something consistent (e.g. stop all javascript)
  // but changing WebView and CTS is a slow and painful process, so for
  // the time being we're doing our best.
  // DO NOT USE FOR ANYTHING EXCEPT ANDROID WEBVIEW API IMPLEMENTATION.
  virtual void PauseTimersForAndroidWebView() = 0;
  virtual void ResumeTimersForAndroidWebView() = 0;
#endif  // defined(OS_ANDROID)

  // RAII handle for pausing the renderer. Renderer is paused while
  // at least one pause handle exists.
  class BLINK_PLATFORM_EXPORT RendererPauseHandle {
   public:
    RendererPauseHandle() = default;
    virtual ~RendererPauseHandle() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(RendererPauseHandle);
  };

  // Tells the scheduler that the renderer process should be paused.
  // Pausing means that all javascript callbacks should not fire.
  // https://html.spec.whatwg.org/#pause
  //
  // Renderer will be resumed when the handle is destroyed.
  // Handle should be destroyed before the renderer.
  virtual std::unique_ptr<RendererPauseHandle> PauseRenderer()
      WARN_UNUSED_RESULT = 0;

  // Returns true if the scheduler has reason to believe that high priority work
  // may soon arrive on the main thread, e.g., if gesture events were observed
  // recently.
  // Must be called from the main thread.
  virtual bool IsHighPriorityWorkAnticipated() = 0;

  // Sets whether to allow suspension of tasks after the backgrounded signal is
  // received via SetRendererBackgrounded(true). Defaults to disabled.
  virtual void SetFreezingWhenBackgroundedEnabled(bool enabled) = 0;

  // Sets the default blame context to which top level work should be
  // attributed in this renderer. |blame_context| must outlive this scheduler.
  virtual void SetTopLevelBlameContext(
      base::trace_event::BlameContext* blame_context) = 0;

  // The renderer scheduler maintains an estimated RAIL mode[1]. This observer
  // can be used to get notified when the mode changes. The observer will be
  // called on the main thread and must outlive this class.
  // [1]
  // https://developers.google.com/web/tools/chrome-devtools/profile/evaluate-performance/rail
  virtual void SetRAILModeObserver(RAILModeObserver* observer) = 0;

  // Sets the kind of renderer process. Should be called on the main thread
  // once.
  virtual void SetRendererProcessType(RendererProcessType type) = 0;

  // Returns a WebScopedVirtualTimePauser which can be used to vote for pausing
  // virtual time. Virtual time will be paused if any WebScopedVirtualTimePauser
  // votes to pause it, and only unpaused only if all
  // WebScopedVirtualTimePausers are either destroyed or vote to unpause.  Note
  // the WebScopedVirtualTimePauser returned by this method is initially
  // unpaused.
  virtual WebScopedVirtualTimePauser CreateWebScopedVirtualTimePauser(
      const char* name,
      WebScopedVirtualTimePauser::VirtualTaskDuration duration =
          WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant) = 0;

 protected:
  WebMainThreadScheduler();
  DISALLOW_COPY_AND_ASSIGN(WebMainThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_MAIN_THREAD_SCHEDULER_H_
