/*
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.h"

#include <v8-inspector.h>
#include <memory>
#include <utility>

#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_float_rect.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/CoreProbeSink.h"
#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/core/events/web_input_event_conversion.h"
#include "third_party/blink/renderer/core/exported/web_settings_impl.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/inspector/dev_tools_emulator.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/inspector/inspector_animation_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_application_cache_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_audits_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_css_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_debugger_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_snapshot_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_emulation_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_io_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_layer_tree_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_log_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_memory_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_network_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_overlay_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_page_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_performance_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_resource_container.h"
#include "third_party/blink/renderer/core/inspector/inspector_resource_content_loader.h"
#include "third_party/blink/renderer/core/inspector/inspector_session.h"
#include "third_party/blink/renderer/core/inspector/inspector_task_runner.h"
#include "third_party/blink/renderer/core/inspector/inspector_tracing_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_worker_agent.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

bool IsMainFrame(WebLocalFrameImpl* frame) {
  // TODO(dgozman): sometimes view->mainFrameImpl() does return null, even
  // though |frame| is meant to be main frame.  See http://crbug.com/526162.
  return frame->ViewImpl() && !frame->Parent();
}

bool ShouldInterruptForMethod(const String& method) {
  // Keep in sync with DevToolsSession::ShouldSendOnIO.
  // TODO(dgozman): find a way to share this.
  return method == "Debugger.pause" || method == "Debugger.setBreakpoint" ||
         method == "Debugger.setBreakpointByUrl" ||
         method == "Debugger.removeBreakpoint" ||
         method == "Debugger.setBreakpointsActive" ||
         method == "Performance.getMetrics" || method == "Page.crash" ||
         method == "Runtime.terminateExecution" ||
         method == "Emulation.setScriptExecutionDisabled";
}

}  // namespace

class ClientMessageLoopAdapter : public MainThreadDebugger::ClientMessageLoop {
 public:
  ~ClientMessageLoopAdapter() override { instance_ = nullptr; }

  static void EnsureMainThreadDebuggerCreated() {
    if (instance_)
      return;
    std::unique_ptr<ClientMessageLoopAdapter> instance(
        new ClientMessageLoopAdapter(
            Platform::Current()->CreateNestedMessageLoopRunner()));
    instance_ = instance.get();
    MainThreadDebugger::Instance()->SetClientMessageLoop(std::move(instance));
  }

  static void ContinueProgram() {
    // Release render thread if necessary.
    if (instance_)
      instance_->QuitNow();
  }

 private:
  ClientMessageLoopAdapter(
      std::unique_ptr<Platform::NestedMessageLoopRunner> message_loop)
      : running_for_debug_break_(false),
        message_loop_(std::move(message_loop)) {
    DCHECK(message_loop_.get());
  }

  void Run(LocalFrame* frame) override {
    if (running_for_debug_break_)
      return;

    running_for_debug_break_ = true;
    RunLoop(WebLocalFrameImpl::FromFrame(frame));
  }

  void RunLoop(WebLocalFrameImpl* frame) {
    // 0. Flush pending frontend messages.
    WebDevToolsAgentImpl* agent = frame->DevToolsAgentImpl();
    agent->FlushProtocolNotifications();

    // 1. Disable input events.
    WebFrameWidgetBase::SetIgnoreInputEvents(true);
    for (auto* const view : WebViewImpl::AllInstances())
      view->GetChromeClient().NotifyPopupOpeningObservers();

    // 2. Disable active objects
    WebView::WillEnterModalLoop();

    // 3. Process messages until quitNow is called.
    message_loop_->Run();

    // 4. Resume active objects
    WebView::DidExitModalLoop();

    // 5. Enable input events.
    WebFrameWidgetBase::SetIgnoreInputEvents(false);
  }

  void QuitNow() override {
    if (running_for_debug_break_) {
      running_for_debug_break_ = false;
      message_loop_->QuitNow();
    }
  }

  void RunIfWaitingForDebugger(LocalFrame* frame) override {
    WebDevToolsAgentImpl* agent =
        WebLocalFrameImpl::FromFrame(frame)->DevToolsAgentImpl();
    if (agent && agent->worker_client_)
      agent->worker_client_->ResumeStartup();
  }

  bool running_for_debug_break_;
  std::unique_ptr<Platform::NestedMessageLoopRunner> message_loop_;

  static ClientMessageLoopAdapter* instance_;
};

ClientMessageLoopAdapter* ClientMessageLoopAdapter::instance_ = nullptr;

// --------- WebDevToolsAgentImpl::Session -------------

class WebDevToolsAgentImpl::Session : public GarbageCollectedFinalized<Session>,
                                      public mojom::blink::DevToolsSession,
                                      public InspectorSession::Client {
 public:
  Session(WebDevToolsAgentImpl*,
          mojom::blink::DevToolsSessionHostAssociatedPtrInfo host_ptr_info,
          mojom::blink::DevToolsSessionAssociatedRequest main_request,
          mojom::blink::DevToolsSessionRequest io_request,
          const String& reattach_state);
  ~Session() override;

  virtual void Trace(blink::Visitor*);
  void Detach();

  InspectorSession* inspector_session() { return inspector_session_.Get(); }
  InspectorNetworkAgent* network_agent() { return network_agent_.Get(); }
  InspectorPageAgent* page_agent() { return page_agent_.Get(); }
  InspectorTracingAgent* tracing_agent() { return tracing_agent_.Get(); }
  InspectorOverlayAgent* overlay_agent() { return overlay_agent_.Get(); }

 private:
  class IOSession;

  // mojom::blink::DevToolsSession implementation.
  void DispatchProtocolCommand(int call_id,
                               const String& method,
                               const String& message) override;

  // InspectorSession::Client implementation.
  void SendProtocolResponse(int session_id,
                            int call_id,
                            const String& response,
                            const String& state) override;
  void SendProtocolNotification(int session_id,
                                const String& message,
                                const String& state) override;

  void DispatchProtocolCommandInternal(int call_id,
                                       const String& method,
                                       const String& message);
  void InitializeInspectorSession(const String& reattach_state);

  Member<WebDevToolsAgentImpl> agent_;
  Member<WebLocalFrameImpl> frame_;
  mojo::AssociatedBinding<mojom::blink::DevToolsSession> binding_;
  mojom::blink::DevToolsSessionHostAssociatedPtr host_ptr_;
  IOSession* io_session_;
  Member<InspectorSession> inspector_session_;
  Member<InspectorNetworkAgent> network_agent_;
  Member<InspectorPageAgent> page_agent_;
  Member<InspectorTracingAgent> tracing_agent_;
  Member<InspectorOverlayAgent> overlay_agent_;
  bool detached_ = false;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

// Created and stored in unique_ptr on UI.
// Binds request, receives messages and destroys on IO.
class WebDevToolsAgentImpl::Session::IOSession
    : public mojom::blink::DevToolsSession {
 public:
  IOSession(scoped_refptr<base::SingleThreadTaskRunner> session_task_runner,
            scoped_refptr<InspectorTaskRunner> inspector_task_runner,
            CrossThreadWeakPersistent<WebDevToolsAgentImpl::Session> session,
            mojom::blink::DevToolsSessionRequest request)
      : session_task_runner_(session_task_runner),
        inspector_task_runner_(inspector_task_runner),
        session_(std::move(session)),
        binding_(this) {
    session_task_runner->PostTask(
        FROM_HERE, ConvertToBaseCallback(CrossThreadBind(
                       &IOSession::BindInterface, CrossThreadUnretained(this),
                       WTF::Passed(std::move(request)))));
  }

  ~IOSession() override {}

  void BindInterface(mojom::blink::DevToolsSessionRequest request) {
    binding_.Bind(std::move(request));
  }

  void DeleteSoon() { session_task_runner_->DeleteSoon(FROM_HERE, this); }

  // mojom::blink::DevToolsSession implementation.
  void DispatchProtocolCommand(int call_id,
                               const String& method,
                               const String& message) override {
    DCHECK(ShouldInterruptForMethod(method));
    // Crash renderer.
    if (method == "Page.crash")
      CHECK(false);
    inspector_task_runner_->AppendTask(
        CrossThreadBind(&WebDevToolsAgentImpl::Session::DispatchProtocolCommand,
                        session_, call_id, method, message));
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> session_task_runner_;
  scoped_refptr<InspectorTaskRunner> inspector_task_runner_;
  CrossThreadWeakPersistent<WebDevToolsAgentImpl::Session> session_;
  mojo::Binding<mojom::blink::DevToolsSession> binding_;

  DISALLOW_COPY_AND_ASSIGN(IOSession);
};

WebDevToolsAgentImpl::Session::Session(
    WebDevToolsAgentImpl* agent,
    mojom::blink::DevToolsSessionHostAssociatedPtrInfo host_ptr_info,
    mojom::blink::DevToolsSessionAssociatedRequest request,
    mojom::blink::DevToolsSessionRequest io_request,
    const String& reattach_state)
    : agent_(agent),
      frame_(agent->web_local_frame_impl_),
      binding_(this, std::move(request)) {
  io_session_ =
      new IOSession(Platform::Current()->GetIOTaskRunner(),
                    frame_->GetFrame()->GetInspectorTaskRunner(),
                    WrapCrossThreadWeakPersistent(this), std::move(io_request));

  host_ptr_.Bind(std::move(host_ptr_info));
  host_ptr_.set_connection_error_handler(WTF::Bind(
      &WebDevToolsAgentImpl::Session::Detach, WrapWeakPersistent(this)));

  InitializeInspectorSession(reattach_state);
}

WebDevToolsAgentImpl::Session::~Session() {
  DCHECK(detached_);
}

void WebDevToolsAgentImpl::Session::Trace(blink::Visitor* visitor) {
  visitor->Trace(agent_);
  visitor->Trace(frame_);
  visitor->Trace(inspector_session_);
  visitor->Trace(network_agent_);
  visitor->Trace(page_agent_);
  visitor->Trace(tracing_agent_);
  visitor->Trace(overlay_agent_);
}

void WebDevToolsAgentImpl::Session::Detach() {
  DCHECK(!detached_);
  detached_ = true;
  agent_->DetachSession(this);
  binding_.Close();
  host_ptr_.reset();
  io_session_->DeleteSoon();
  io_session_ = nullptr;
  inspector_session_->Dispose();
}

void WebDevToolsAgentImpl::Session::SendProtocolResponse(int session_id,
                                                         int call_id,
                                                         const String& response,
                                                         const String& state) {
  if (detached_)
    return;

  // Make tests more predictable by flushing all sessions before sending
  // protocol response in any of them.
  if (LayoutTestSupport::IsRunningLayoutTest())
    agent_->FlushProtocolNotifications();
  host_ptr_->DispatchProtocolResponse(response, call_id, state);
}

void WebDevToolsAgentImpl::Session::SendProtocolNotification(
    int session_id,
    const String& message,
    const String& state) {
  host_ptr_->DispatchProtocolNotification(message, state);
}

void WebDevToolsAgentImpl::Session::DispatchProtocolCommand(
    int call_id,
    const String& method,
    const String& message) {
  // IOSession does not provide ordering guarantees relative to
  // Session, so a command may come to IOSession after Session is detached,
  // and get posted to main thread to this method.
  //
  // At the same time, Session may not be garbage collected yet
  // (even though already detached), and CrossThreadWeakPersistent<Session>
  // will still be valid.
  //
  // Both these factors combined may lead to this method being called after
  // detach, so we have to check a flag here.
  if (detached_)
    return;
  inspector_session_->DispatchProtocolMessage(method, message);
}

void WebDevToolsAgentImpl::Session::InitializeInspectorSession(
    const String& reattach_state) {
  ClientMessageLoopAdapter::EnsureMainThreadDebuggerCreated();
  MainThreadDebugger* main_thread_debugger = MainThreadDebugger::Instance();
  v8::Isolate* isolate = V8PerIsolateData::MainThreadIsolate();
  InspectedFrames* inspected_frames = agent_->inspected_frames_.Get();

  inspector_session_ = new InspectorSession(
      this, agent_->probe_sink_.Get(), 0,
      main_thread_debugger->GetV8Inspector(),
      main_thread_debugger->ContextGroupId(inspected_frames->Root()),
      reattach_state);

  InspectorDOMAgent* dom_agent = new InspectorDOMAgent(
      isolate, inspected_frames, inspector_session_->V8Session());
  inspector_session_->Append(dom_agent);

  InspectorLayerTreeAgent* layer_tree_agent =
      InspectorLayerTreeAgent::Create(inspected_frames, agent_);
  inspector_session_->Append(layer_tree_agent);

  network_agent_ = new InspectorNetworkAgent(inspected_frames, nullptr,
                                             inspector_session_->V8Session());
  inspector_session_->Append(network_agent_);

  InspectorCSSAgent* css_agent =
      InspectorCSSAgent::Create(dom_agent, inspected_frames, network_agent_,
                                agent_->resource_content_loader_.Get(),
                                agent_->resource_container_.Get());
  inspector_session_->Append(css_agent);

  InspectorDOMDebuggerAgent* dom_debugger_agent = new InspectorDOMDebuggerAgent(
      isolate, dom_agent, inspector_session_->V8Session());
  inspector_session_->Append(dom_debugger_agent);

  inspector_session_->Append(
      InspectorDOMSnapshotAgent::Create(inspected_frames, dom_debugger_agent));

  inspector_session_->Append(new InspectorAnimationAgent(
      inspected_frames, css_agent, inspector_session_->V8Session()));

  inspector_session_->Append(InspectorMemoryAgent::Create(inspected_frames));

  inspector_session_->Append(
      InspectorPerformanceAgent::Create(inspected_frames));

  inspector_session_->Append(
      InspectorApplicationCacheAgent::Create(inspected_frames));

  inspector_session_->Append(
      new InspectorWorkerAgent(inspected_frames, nullptr));

  tracing_agent_ = new InspectorTracingAgent(inspected_frames);
  inspector_session_->Append(tracing_agent_);

  page_agent_ = InspectorPageAgent::Create(
      inspected_frames, agent_, agent_->resource_content_loader_.Get(),
      inspector_session_->V8Session());
  inspector_session_->Append(page_agent_);

  inspector_session_->Append(new InspectorLogAgent(
      &inspected_frames->Root()->GetPage()->GetConsoleMessageStorage(),
      inspected_frames->Root()->GetPerformanceMonitor(),
      inspector_session_->V8Session()));

  overlay_agent_ =
      new InspectorOverlayAgent(frame_.Get(), inspected_frames,
                                inspector_session_->V8Session(), dom_agent);
  inspector_session_->Append(overlay_agent_);

  inspector_session_->Append(
      new InspectorIOAgent(isolate, inspector_session_->V8Session()));

  inspector_session_->Append(new InspectorAuditsAgent(network_agent_));

  // TODO(dgozman): we should actually pass the view instead of frame, but
  // during remote->local transition we cannot access mainFrameImpl() yet, so
  // we have to store the frame which will become the main frame later.
  inspector_session_->Append(new InspectorEmulationAgent(frame_.Get()));

  // Call session init callbacks registered from higher layers
  CoreInitializer::GetInstance().InitInspectorAgentSession(
      inspector_session_, agent_->include_view_agents_, dom_agent,
      inspected_frames, frame_->ViewImpl()->GetPage());

  if (!reattach_state.IsNull()) {
    inspector_session_->Restore();
    if (agent_->worker_client_)
      agent_->worker_client_->ResumeStartup();
  }
}

// --------- WebDevToolsAgentImpl -------------

// static
WebDevToolsAgentImpl* WebDevToolsAgentImpl::CreateForFrame(
    WebLocalFrameImpl* frame) {
  return new WebDevToolsAgentImpl(frame, IsMainFrame(frame), nullptr);
}

// static
WebDevToolsAgentImpl* WebDevToolsAgentImpl::CreateForWorker(
    WebLocalFrameImpl* frame,
    WorkerClient* worker_client) {
  return new WebDevToolsAgentImpl(frame, true, worker_client);
}

WebDevToolsAgentImpl::WebDevToolsAgentImpl(
    WebLocalFrameImpl* web_local_frame_impl,
    bool include_view_agents,
    WorkerClient* worker_client)
    : binding_(this),
      worker_client_(worker_client),
      web_local_frame_impl_(web_local_frame_impl),
      probe_sink_(web_local_frame_impl_->GetFrame()->GetProbeSink()),
      resource_content_loader_(InspectorResourceContentLoader::Create(
          web_local_frame_impl_->GetFrame())),
      inspected_frames_(new InspectedFrames(web_local_frame_impl_->GetFrame())),
      resource_container_(new InspectorResourceContainer(inspected_frames_)),
      include_view_agents_(include_view_agents) {
  DCHECK(IsMainThread());
  DCHECK(web_local_frame_impl_->GetFrame());
}

WebDevToolsAgentImpl::~WebDevToolsAgentImpl() {
  DCHECK(!worker_client_);
}

void WebDevToolsAgentImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(sessions_);
  visitor->Trace(web_local_frame_impl_);
  visitor->Trace(probe_sink_);
  visitor->Trace(resource_content_loader_);
  visitor->Trace(inspected_frames_);
  visitor->Trace(resource_container_);
  visitor->Trace(node_to_inspect_);
}

void WebDevToolsAgentImpl::WillBeDestroyed() {
  DCHECK(web_local_frame_impl_->GetFrame());
  DCHECK(inspected_frames_->Root()->View());

  HeapHashSet<Member<Session>> copy(sessions_);
  for (auto& session : copy)
    session->Detach();

  resource_content_loader_->Dispose();
  worker_client_ = nullptr;
  binding_.Close();
}

void WebDevToolsAgentImpl::BindRequest(
    mojom::blink::DevToolsAgentAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

void WebDevToolsAgentImpl::AttachDevToolsSession(
    mojom::blink::DevToolsSessionHostAssociatedPtrInfo host,
    mojom::blink::DevToolsSessionAssociatedRequest session_request,
    mojom::blink::DevToolsSessionRequest io_session_request,
    const String& reattach_state) {
  if (!sessions_.size())
    Platform::Current()->CurrentThread()->AddTaskObserver(this);
  Session* session =
      new Session(this, std::move(host), std::move(session_request),
                  std::move(io_session_request), reattach_state);
  sessions_.insert(session);
  if (node_to_inspect_) {
    session->overlay_agent()->Inspect(node_to_inspect_);
    node_to_inspect_ = nullptr;
  }
}

void WebDevToolsAgentImpl::InspectElement(const WebPoint& point_in_local_root) {
  WebPoint point = point_in_local_root;
  // TODO(dgozman): the ViewImpl() check must not be necessary,
  // but it is required when attaching early to a provisional frame.
  // We should clean this up once provisional frames are gone.
  // See https://crbug.com/578349.
  if (web_local_frame_impl_->ViewImpl() &&
      web_local_frame_impl_->ViewImpl()->Client()) {
    WebFloatRect rect(point.x, point.y, 0, 0);
    web_local_frame_impl_->ViewImpl()->Client()->ConvertWindowToViewport(&rect);
    point = WebPoint(rect.x, rect.y);
  }

  HitTestRequest::HitTestRequestType hit_type =
      HitTestRequest::kMove | HitTestRequest::kReadOnly |
      HitTestRequest::kAllowChildFrameContent;
  HitTestRequest request(hit_type);
  WebMouseEvent dummy_event(WebInputEvent::kMouseDown,
                            WebInputEvent::kNoModifiers,
                            WTF::CurrentTimeTicks());
  dummy_event.SetPositionInWidget(point.x, point.y);
  IntPoint transformed_point = FlooredIntPoint(
      TransformWebMouseEvent(web_local_frame_impl_->GetFrameView(), dummy_event)
          .PositionInRootFrame());
  HitTestResult result(
      request, web_local_frame_impl_->GetFrameView()->RootFrameToContents(
                   transformed_point));
  web_local_frame_impl_->GetFrame()->ContentLayoutObject()->HitTest(result);
  Node* node = result.InnerNode();
  if (!node && web_local_frame_impl_->GetFrame()->GetDocument())
    node = web_local_frame_impl_->GetFrame()->GetDocument()->documentElement();

  if (!sessions_.IsEmpty()) {
    for (auto& session : sessions_)
      session->overlay_agent()->Inspect(node);
  } else {
    node_to_inspect_ = node;
  }
}

void WebDevToolsAgentImpl::DetachSession(Session* session) {
  sessions_.erase(session);
  if (!sessions_.size())
    Platform::Current()->CurrentThread()->RemoveTaskObserver(this);
}

void WebDevToolsAgentImpl::DidCommitLoadForLocalFrame(LocalFrame* frame) {
  resource_container_->DidCommitLoadForLocalFrame(frame);
  resource_content_loader_->DidCommitLoadForLocalFrame(frame);
  for (auto& session : sessions_)
    session->inspector_session()->DidCommitLoadForLocalFrame(frame);
}

void WebDevToolsAgentImpl::DidStartProvisionalLoad(LocalFrame* frame) {
  if (inspected_frames_->Root() == frame) {
    for (auto& session : sessions_)
      session->inspector_session()->V8Session()->resume();
  }
}

bool WebDevToolsAgentImpl::ScreencastEnabled() {
  for (auto& session : sessions_) {
    if (session->page_agent()->ScreencastEnabled())
      return true;
  }
  return false;
}

void WebDevToolsAgentImpl::PageLayoutInvalidated(bool resized) {
  for (auto& session : sessions_)
    session->overlay_agent()->PageLayoutInvalidated(resized);
}

bool WebDevToolsAgentImpl::IsInspectorLayer(GraphicsLayer* layer) {
  for (auto& session : sessions_) {
    if (session->overlay_agent()->IsInspectorLayer(layer))
      return true;
  }
  return false;
}

String WebDevToolsAgentImpl::EvaluateInOverlayForTesting(const String& script) {
  String result;
  for (auto& session : sessions_)
    result = session->overlay_agent()->EvaluateInOverlayForTest(script);
  return result;
}

void WebDevToolsAgentImpl::PaintOverlay() {
  for (auto& session : sessions_)
    session->overlay_agent()->PaintOverlay();
}

void WebDevToolsAgentImpl::LayoutOverlay() {
  for (auto& session : sessions_)
    session->overlay_agent()->LayoutOverlay();
}

void WebDevToolsAgentImpl::DispatchBufferedTouchEvents() {
  for (auto& session : sessions_)
    session->overlay_agent()->DispatchBufferedTouchEvents();
}

bool WebDevToolsAgentImpl::HandleInputEvent(const WebInputEvent& event) {
  for (auto& session : sessions_) {
    if (session->overlay_agent()->HandleInputEvent(event))
      return true;
  }
  return false;
}

String WebDevToolsAgentImpl::NavigationInitiatorInfo(LocalFrame* frame) {
  for (auto& session : sessions_) {
    String initiator = session->network_agent()->NavigationInitiatorInfo(frame);
    if (!initiator.IsNull())
      return initiator;
  }
  return String();
}

void WebDevToolsAgentImpl::FlushProtocolNotifications() {
  for (auto& session : sessions_)
    session->inspector_session()->flushProtocolNotifications();
}

void WebDevToolsAgentImpl::WillProcessTask() {
  if (sessions_.IsEmpty())
    return;
  ThreadDebugger::IdleFinished(V8PerIsolateData::MainThreadIsolate());
}

void WebDevToolsAgentImpl::DidProcessTask() {
  if (sessions_.IsEmpty())
    return;
  ThreadDebugger::IdleStarted(V8PerIsolateData::MainThreadIsolate());
  FlushProtocolNotifications();
}

}  // namespace blink
