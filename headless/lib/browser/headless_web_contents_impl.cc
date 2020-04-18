// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_web_contents_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_termination_info.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/renderer_preferences.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/browser/headless_browser_main_parts.h"
#include "headless/lib/browser/protocol/headless_handler.h"
#include "headless/public/internal/headless_devtools_client_impl.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/switches.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "headless/lib/browser/headless_print_manager.h"
#endif

namespace headless {

// static
HeadlessWebContentsImpl* HeadlessWebContentsImpl::From(
    HeadlessWebContents* web_contents) {
  // This downcast is safe because there is only one implementation of
  // HeadlessWebContents.
  return static_cast<HeadlessWebContentsImpl*>(web_contents);
}

// static
HeadlessWebContentsImpl* HeadlessWebContentsImpl::From(
    HeadlessBrowser* browser,
    content::WebContents* contents) {
  return HeadlessWebContentsImpl::From(
      browser->GetWebContentsForDevToolsAgentHostId(
          content::DevToolsAgentHost::GetOrCreateFor(contents)->GetId()));
}

class HeadlessWebContentsImpl::Delegate : public content::WebContentsDelegate {
 public:
  explicit Delegate(HeadlessWebContentsImpl* headless_web_contents)
      : headless_web_contents_(headless_web_contents) {}

#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  // Return the security style of the given |web_contents|, populating
  // |security_style_explanations| to explain why the SecurityStyle was chosen.
  blink::WebSecurityStyle GetSecurityStyle(
      content::WebContents* web_contents,
      content::SecurityStyleExplanations* security_style_explanations)
      override {
    security_state::SecurityInfo security_info;
    security_state::GetSecurityInfo(
        security_state::GetVisibleSecurityState(web_contents),
        false /* used_policy_installed_certificate */,
        base::Bind(&content::IsOriginSecure), &security_info);
    return security_state::GetSecurityStyle(security_info,
                                            security_style_explanations);
  }
#endif  // !defined(CHROME_MULTIPLE_DLL_CHILD)

  void ActivateContents(content::WebContents* contents) override {
    contents->GetRenderViewHost()->GetWidget()->Focus();
  }

  void CloseContents(content::WebContents* source) override {
    auto* const headless_contents =
        HeadlessWebContentsImpl::From(browser(), source);
    DCHECK(headless_contents);
    headless_contents->DelegateRequestsClose();
  }

  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override {
    DCHECK(new_contents->GetBrowserContext() ==
           headless_web_contents_->browser_context());

    std::unique_ptr<HeadlessWebContentsImpl> child_contents =
        HeadlessWebContentsImpl::CreateForChildContents(
            headless_web_contents_, std::move(new_contents));
    HeadlessWebContentsImpl* raw_child_contents = child_contents.get();
    headless_web_contents_->browser_context()->RegisterWebContents(
        std::move(child_contents));
    headless_web_contents_->browser_context()->NotifyChildContentsCreated(
        headless_web_contents_, raw_child_contents);

    const gfx::Rect default_rect(
        headless_web_contents_->browser()->options()->window_size);
    const gfx::Rect rect = initial_rect.IsEmpty() ? default_rect : initial_rect;
    raw_child_contents->SetBounds(rect);
  }

  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override {
    DCHECK_EQ(source, headless_web_contents_->web_contents());
    content::WebContents* target = nullptr;
    switch (params.disposition) {
      case WindowOpenDisposition::CURRENT_TAB:
        target = source;
        break;

      case WindowOpenDisposition::NEW_POPUP:
      case WindowOpenDisposition::NEW_WINDOW:
      case WindowOpenDisposition::NEW_BACKGROUND_TAB:
      case WindowOpenDisposition::NEW_FOREGROUND_TAB: {
        HeadlessWebContentsImpl* child_contents = HeadlessWebContentsImpl::From(
            headless_web_contents_->browser_context()
                ->CreateWebContentsBuilder()
                .SetWindowSize(source->GetContainerBounds().size())
                .Build());
        headless_web_contents_->browser_context()->NotifyChildContentsCreated(
            headless_web_contents_, child_contents);
        target = child_contents->web_contents();
        break;
      }

      // TODO(veluca): add support for other disposition types.
      case WindowOpenDisposition::SINGLETON_TAB:
      case WindowOpenDisposition::OFF_THE_RECORD:
      case WindowOpenDisposition::SAVE_TO_DISK:
      case WindowOpenDisposition::IGNORE_ACTION:
      default:
        return nullptr;
    }

    content::NavigationController::LoadURLParams load_url_params(params.url);
    load_url_params.source_site_instance = params.source_site_instance;
    load_url_params.transition_type = params.transition;
    load_url_params.frame_tree_node_id = params.frame_tree_node_id;
    load_url_params.referrer = params.referrer;
    load_url_params.redirect_chain = params.redirect_chain;
    load_url_params.extra_headers = params.extra_headers;
    load_url_params.is_renderer_initiated = params.is_renderer_initiated;
    load_url_params.should_replace_current_entry =
        params.should_replace_current_entry;

    if (params.uses_post) {
      load_url_params.load_type =
          content::NavigationController::LOAD_TYPE_HTTP_POST;
      load_url_params.post_data = params.post_data;
    }

    target->GetController().LoadURLWithParams(load_url_params);
    return target;
  }

  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override {
    return !headless_web_contents_->browser_context()
                ->options()
                ->block_new_web_contents();
  }

 private:
  HeadlessBrowserImpl* browser() { return headless_web_contents_->browser(); }

  HeadlessWebContentsImpl* headless_web_contents_;  // Not owned.
  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

struct HeadlessWebContentsImpl::PendingFrame {
 public:
  PendingFrame() = default;
  ~PendingFrame() = default;

  bool MaybeRunCallback() {
    if (wait_for_copy_result || !display_did_finish_frame)
      return false;
    std::move(callback).Run(has_damage, std::move(bitmap));
    return true;
  }

  uint64_t sequence_number = 0;
  bool wait_for_copy_result = false;
  bool display_did_finish_frame = false;
  bool has_damage = false;
  std::unique_ptr<SkBitmap> bitmap;
  FrameFinishedCallback callback;

 private:
  DISALLOW_COPY_AND_ASSIGN(PendingFrame);
};

// static
std::unique_ptr<HeadlessWebContentsImpl> HeadlessWebContentsImpl::Create(
    HeadlessWebContents::Builder* builder) {
  content::WebContents::CreateParams create_params(builder->browser_context_,
                                                   nullptr);
  create_params.initial_size = builder->window_size_;

  std::unique_ptr<HeadlessWebContentsImpl> headless_web_contents =
      base::WrapUnique(new HeadlessWebContentsImpl(
          content::WebContents::Create(create_params),
          builder->browser_context_));

  headless_web_contents->begin_frame_control_enabled_ =
      builder->enable_begin_frame_control_ ||
      headless_web_contents->browser()->options()->enable_begin_frame_control;
  headless_web_contents->InitializeWindow(gfx::Rect(builder->window_size_));
  if (!headless_web_contents->OpenURL(builder->initial_url_))
    return nullptr;
  return headless_web_contents;
}

// static
std::unique_ptr<HeadlessWebContentsImpl>
HeadlessWebContentsImpl::CreateForChildContents(
    HeadlessWebContentsImpl* parent,
    std::unique_ptr<content::WebContents> child_contents) {
  auto child = base::WrapUnique(new HeadlessWebContentsImpl(
      std::move(child_contents), parent->browser_context()));

  // Child contents have their own root window and inherit the BeginFrameControl
  // setting.
  child->begin_frame_control_enabled_ = parent->begin_frame_control_enabled_;
  child->InitializeWindow(child->web_contents_->GetContainerBounds());

  // There may already be frames, so make sure they also have our services.
  for (content::RenderFrameHost* frame_host :
       child->web_contents_->GetAllFrames())
    child->RenderFrameCreated(frame_host);

  return child;
}

void HeadlessWebContentsImpl::InitializeWindow(
    const gfx::Rect& initial_bounds) {
  static int window_id = 1;
  window_id_ = window_id++;
  window_state_ = "normal";

  browser()->PlatformInitializeWebContents(this);
  SetBounds(initial_bounds);

  if (begin_frame_control_enabled_) {
    ui::Compositor* compositor = browser()->PlatformGetCompositor(this);
    DCHECK(compositor);
    compositor->SetExternalBeginFrameClient(this);
  }
}

void HeadlessWebContentsImpl::SetBounds(const gfx::Rect& bounds) {
  browser()->PlatformSetWebContentsBounds(this, bounds);
}

HeadlessWebContentsImpl::HeadlessWebContentsImpl(
    std::unique_ptr<content::WebContents> web_contents,
    HeadlessBrowserContextImpl* browser_context)
    : content::WebContentsObserver(web_contents.get()),
      web_contents_delegate_(new HeadlessWebContentsImpl::Delegate(this)),
      web_contents_(std::move(web_contents)),
      agent_host_(
          content::DevToolsAgentHost::GetOrCreateFor(web_contents_.get())),
      browser_context_(browser_context),
      render_process_host_(web_contents_->GetMainFrame()->GetProcess()),
      weak_ptr_factory_(this) {
#if BUILDFLAG(ENABLE_PRINTING) && !defined(CHROME_MULTIPLE_DLL_CHILD)
  HeadlessPrintManager::CreateForWebContents(web_contents_.get());
// TODO(weili): Add support for printing OOPIFs.
#endif
  web_contents_->GetMutableRendererPrefs()->accept_languages =
      browser_context->options()->accept_language();
  web_contents_->GetMutableRendererPrefs()->hinting =
      browser_context->options()->font_render_hinting();
  web_contents_->SetDelegate(web_contents_delegate_.get());
  render_process_host_->AddObserver(this);
  agent_host_->AddObserver(this);
}

HeadlessWebContentsImpl::~HeadlessWebContentsImpl() {
  agent_host_->RemoveObserver(this);
  if (render_process_host_)
    render_process_host_->RemoveObserver(this);
  if (begin_frame_control_enabled_) {
    ui::Compositor* compositor = browser()->PlatformGetCompositor(this);
    DCHECK(compositor);
    compositor->SetExternalBeginFrameClient(nullptr);
  }
}

void HeadlessWebContentsImpl::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  browser_context_->SetDevToolsFrameToken(
      render_frame_host->GetProcess()->GetID(),
      render_frame_host->GetRoutingID(),
      render_frame_host->GetDevToolsFrameToken(),
      render_frame_host->GetFrameTreeNodeId());
}

void HeadlessWebContentsImpl::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  browser_context_->RemoveDevToolsFrameToken(
      render_frame_host->GetProcess()->GetID(),
      render_frame_host->GetRoutingID(),
      render_frame_host->GetFrameTreeNodeId());
}

void HeadlessWebContentsImpl::RenderViewReady() {
  DCHECK(web_contents()->GetMainFrame()->IsRenderFrameLive());

  if (devtools_target_ready_notification_sent_)
    return;

  for (auto& observer : observers_)
    observer.DevToolsTargetReady();

  devtools_target_ready_notification_sent_ = true;
}

int HeadlessWebContentsImpl::GetMainFrameRenderProcessId() const {
  if (!web_contents() || !web_contents()->GetMainFrame())
    return -1;
  return web_contents()->GetMainFrame()->GetProcess()->GetID();
}

int HeadlessWebContentsImpl::GetMainFrameTreeNodeId() const {
  if (!web_contents() || !web_contents()->GetMainFrame())
    return -1;
  return web_contents()->GetMainFrame()->GetFrameTreeNodeId();
}

std::string HeadlessWebContentsImpl::GetMainFrameDevToolsId() const {
  if (!web_contents() || !web_contents()->GetMainFrame())
    return "";
  return web_contents()->GetMainFrame()->GetDevToolsFrameToken().ToString();
}

bool HeadlessWebContentsImpl::OpenURL(const GURL& url) {
  if (!url.is_valid())
    return false;
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);
  web_contents_->GetController().LoadURLWithParams(params);
  web_contents_delegate_->ActivateContents(web_contents_.get());
  web_contents_->Focus();
  return true;
}

void HeadlessWebContentsImpl::Close() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (quit_closure_)
    return;

  if (!render_process_exited_) {
    web_contents_->ClosePage();
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  browser_context()->DestroyWebContents(this);
}

void HeadlessWebContentsImpl::DelegateRequestsClose() {
  if (quit_closure_) {
    quit_closure_.Run();
    quit_closure_ = base::Closure();
  } else {
    browser_context()->DestroyWebContents(this);
  }
}

std::string HeadlessWebContentsImpl::GetDevToolsAgentHostId() {
  return agent_host_->GetId();
}

void HeadlessWebContentsImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void HeadlessWebContentsImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void HeadlessWebContentsImpl::DevToolsAgentHostAttached(
    content::DevToolsAgentHost* agent_host) {
  for (auto& observer : observers_)
    observer.DevToolsClientAttached();
}

void HeadlessWebContentsImpl::DevToolsAgentHostDetached(
    content::DevToolsAgentHost* agent_host) {
  for (auto& observer : observers_)
    observer.DevToolsClientDetached();
}

void HeadlessWebContentsImpl::RenderProcessExited(
    content::RenderProcessHost* host,
    const content::ChildProcessTerminationInfo& info) {
  DCHECK_EQ(render_process_host_, host);
  render_process_exited_ = true;
  for (auto& observer : observers_)
    observer.RenderProcessExited(info.status, info.exit_code);
}

void HeadlessWebContentsImpl::RenderProcessHostDestroyed(
    content::RenderProcessHost* host) {
  DCHECK_EQ(render_process_host_, host);
  render_process_host_ = nullptr;
}

HeadlessDevToolsTarget* HeadlessWebContentsImpl::GetDevToolsTarget() {
  return web_contents()->GetMainFrame()->IsRenderFrameLive() ? this : nullptr;
}

void HeadlessWebContentsImpl::AttachClient(HeadlessDevToolsClient* client) {
  HeadlessDevToolsClientImpl::From(client)->AttachToHost(agent_host_.get());
}

void HeadlessWebContentsImpl::DetachClient(HeadlessDevToolsClient* client) {
  DCHECK(agent_host_);
  HeadlessDevToolsClientImpl::From(client)->DetachFromHost(agent_host_.get());
}

bool HeadlessWebContentsImpl::IsAttached() {
  DCHECK(agent_host_);
  return agent_host_->IsAttached();
}

content::WebContents* HeadlessWebContentsImpl::web_contents() const {
  return web_contents_.get();
}

HeadlessBrowserImpl* HeadlessWebContentsImpl::browser() const {
  return browser_context_->browser();
}

HeadlessBrowserContextImpl* HeadlessWebContentsImpl::browser_context() const {
  return browser_context_;
}

void HeadlessWebContentsImpl::OnDisplayDidFinishFrame(
    const viz::BeginFrameAck& ack) {
  TRACE_EVENT2("headless", "HeadlessWebContentsImpl::OnDisplayDidFinishFrame",
               "source_id", ack.source_id, "sequence_number",
               ack.sequence_number);

  auto it = pending_frames_.begin();
  while (it != pending_frames_.end()) {
    if (begin_frame_source_id_ == ack.source_id &&
        (*it)->sequence_number <= ack.sequence_number) {
      (*it)->has_damage = ack.has_damage;
      (*it)->display_did_finish_frame = true;
      if ((*it)->MaybeRunCallback()) {
        it = pending_frames_.erase(it);
      } else {
        ++it;
      }
    } else {
      ++it;
    }
  }
}

void HeadlessWebContentsImpl::OnNeedsExternalBeginFrames(
    bool needs_begin_frames) {
  protocol::HeadlessHandler::OnNeedsBeginFrames(this, needs_begin_frames);
  TRACE_EVENT1("headless",
               "HeadlessWebContentsImpl::OnNeedsExternalBeginFrames",
               "needs_begin_frames", needs_begin_frames);
  needs_external_begin_frames_ = needs_begin_frames;
}

void HeadlessWebContentsImpl::PendingFrameReadbackComplete(
    HeadlessWebContentsImpl::PendingFrame* pending_frame,
    const SkBitmap& bitmap) {
  TRACE_EVENT2("headless",
               "HeadlessWebContentsImpl::PendingFrameReadbackComplete",
               "sequence_number", pending_frame->sequence_number, "success",
               !bitmap.drawsNothing());
  if (bitmap.drawsNothing()) {
    LOG(WARNING) << "Readback from surface failed.";
  } else {
    pending_frame->bitmap = std::make_unique<SkBitmap>(bitmap);
  }

  pending_frame->wait_for_copy_result = false;

  // Run callback if the frame was already finished by the display.
  if (pending_frame->MaybeRunCallback()) {
    base::EraseIf(pending_frames_,
                  [pending_frame](const std::unique_ptr<PendingFrame>& frame) {
                    return frame.get() == pending_frame;
                  });
  }
}

void HeadlessWebContentsImpl::BeginFrame(
    const base::TimeTicks& frame_timeticks,
    const base::TimeTicks& deadline,
    const base::TimeDelta& interval,
    bool animate_only,
    bool capture_screenshot,
    FrameFinishedCallback frame_finished_callback) {
  DCHECK(begin_frame_control_enabled_);
  TRACE_EVENT2("headless", "HeadlessWebContentsImpl::BeginFrame", "frame_time",
               frame_timeticks, "capture_screenshot", capture_screenshot);

  uint64_t sequence_number = begin_frame_sequence_number_++;

  auto pending_frame = std::make_unique<PendingFrame>();
  pending_frame->sequence_number = sequence_number;
  pending_frame->callback = std::move(frame_finished_callback);
  // Note: It's important to move |pending_frame| into |pending_frames_| now
  // since the CopyFromSurface() call below can run its result callback
  // synchronously on certain platforms/environments.
  auto* const pending_frame_raw_ptr = pending_frame.get();
  pending_frames_.emplace_back(std::move(pending_frame));

  if (capture_screenshot) {
    content::RenderWidgetHostView* view =
        web_contents()->GetRenderWidgetHostView();
    if (view && view->IsSurfaceAvailableForCopy()) {
      pending_frame_raw_ptr->wait_for_copy_result = true;
      view->CopyFromSurface(
          gfx::Rect(), gfx::Size(),
          base::BindOnce(&HeadlessWebContentsImpl::PendingFrameReadbackComplete,
                         weak_ptr_factory_.GetWeakPtr(),
                         pending_frame_raw_ptr));
    } else {
      LOG(WARNING) << "Surface not ready for screenshot.";
    }
  }

  ui::Compositor* compositor = browser()->PlatformGetCompositor(this);
  DCHECK(compositor);

  auto args = viz::BeginFrameArgs::Create(
      BEGINFRAME_FROM_HERE, begin_frame_source_id_, sequence_number,
      frame_timeticks, deadline, interval, viz::BeginFrameArgs::NORMAL);
  args.animate_only = animate_only;

  compositor->IssueExternalBeginFrame(args);
}

HeadlessWebContents::Builder::Builder(
    HeadlessBrowserContextImpl* browser_context)
    : browser_context_(browser_context),
      window_size_(browser_context->options()->window_size()) {}

HeadlessWebContents::Builder::~Builder() = default;

HeadlessWebContents::Builder::Builder(Builder&&) = default;

HeadlessWebContents::Builder& HeadlessWebContents::Builder::SetInitialURL(
    const GURL& initial_url) {
  initial_url_ = initial_url;
  return *this;
}

HeadlessWebContents::Builder& HeadlessWebContents::Builder::SetWindowSize(
    const gfx::Size& size) {
  window_size_ = size;
  return *this;
}

HeadlessWebContents::Builder&
HeadlessWebContents::Builder::SetEnableBeginFrameControl(
    bool enable_begin_frame_control) {
  enable_begin_frame_control_ = enable_begin_frame_control;
  return *this;
}

HeadlessWebContents* HeadlessWebContents::Builder::Build() {
  return browser_context_->CreateWebContents(this);
}

}  // namespace headless
