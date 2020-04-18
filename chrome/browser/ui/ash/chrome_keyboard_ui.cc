// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_keyboard_ui.h"

#include <set>
#include <string>
#include <utility>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/bindings_policy.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_private_api.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/api/virtual_keyboard_private.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_messages.h"
#include "ipc/ipc_message_macros.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/compositor_extra/shadow.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/keyboard/keyboard_resource_util.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/wm/core/shadow_types.h"

namespace virtual_keyboard_private = extensions::api::virtual_keyboard_private;

namespace {

GURL& GetOverrideVirtualKeyboardUrl() {
  static base::NoDestructor<GURL> url;
  return *url;
}

class WindowBoundsChangeObserver : public aura::WindowObserver {
 public:
  explicit WindowBoundsChangeObserver(ChromeKeyboardUI* ui) : ui_(ui) {}
  ~WindowBoundsChangeObserver() override {}

  void AddObservedWindow(aura::Window* window) {
    if (!window->HasObserver(this)) {
      window->AddObserver(this);
      observed_windows_.insert(window);
    }
  }
  void RemoveAllObservedWindows() {
    for (aura::Window* window : observed_windows_)
      window->RemoveObserver(this);
    observed_windows_.clear();
  }

 private:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    ui_->UpdateInsetsForWindow(window);
  }
  void OnWindowDestroyed(aura::Window* window) override {
    if (window->HasObserver(this))
      window->RemoveObserver(this);
    observed_windows_.erase(window);
  }

  ChromeKeyboardUI* const ui_;
  std::set<aura::Window*> observed_windows_;

  DISALLOW_COPY_AND_ASSIGN(WindowBoundsChangeObserver);
};

// The WebContentsDelegate for the chrome keyboard.
// The delegate deletes itself when the keyboard is destroyed.
class ChromeKeyboardContentsDelegate : public content::WebContentsDelegate,
                                       public content::WebContentsObserver {
 public:
  explicit ChromeKeyboardContentsDelegate(ChromeKeyboardUI* ui) : ui_(ui) {}
  ~ChromeKeyboardContentsDelegate() override {}

 private:
  // content::WebContentsDelegate:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override {
    source->GetController().LoadURL(params.url, params.referrer,
                                    params.transition, params.extra_headers);
    Observe(source);
    return source;
  }

  bool CanDragEnter(content::WebContents* source,
                    const content::DropData& data,
                    blink::WebDragOperationsMask operations_allowed) override {
    return false;
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
    return false;
  }

  bool IsPopupOrPanel(const content::WebContents* source) const override {
    return true;
  }

  void MoveContents(content::WebContents* source,
                    const gfx::Rect& pos) override {
    aura::Window* keyboard = ui_->GetContentsWindow();
    // keyboard window must have been added to keyboard container window at this
    // point. Otherwise, wrong keyboard bounds is used and may cause problem as
    // described in crbug.com/367788.
    DCHECK(keyboard->parent());
    // keyboard window bounds may not set to |pos| after this call. If keyboard
    // is in FULL_WIDTH mode, only the height of keyboard window will be
    // changed.
    keyboard->SetBounds(pos);
  }

  // content::WebContentsDelegate:
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override {
    ui_->RequestAudioInput(web_contents, request, callback);
  }

  // content::WebContentsDelegate:
  bool PreHandleGestureEvent(content::WebContents* source,
                             const blink::WebGestureEvent& event) override {
    switch (event.GetType()) {
      // Scroll events are not suppressed because the menu to select IME should
      // be scrollable.
      case blink::WebInputEvent::kGestureScrollBegin:
      case blink::WebInputEvent::kGestureScrollEnd:
      case blink::WebInputEvent::kGestureScrollUpdate:
      case blink::WebInputEvent::kGestureFlingStart:
      case blink::WebInputEvent::kGestureFlingCancel:
        return false;
      default:
        // Stop gesture events from being passed to renderer to suppress the
        // context menu. crbug.com/685140
        return true;
    }
  }

  // content::WebContentsObserver:
  void WebContentsDestroyed() override { delete this; }

  ChromeKeyboardUI* const ui_;

  DISALLOW_COPY_AND_ASSIGN(ChromeKeyboardContentsDelegate);
};

class AshKeyboardControllerObserver
    : public keyboard::KeyboardControllerObserver {
 public:
  explicit AshKeyboardControllerObserver(content::BrowserContext* context)
      : context_(context) {}
  ~AshKeyboardControllerObserver() override {}

  // KeyboardControllerObserver:
  void OnKeyboardVisibleBoundsChanged(const gfx::Rect& bounds) override {
    extensions::EventRouter* router = extensions::EventRouter::Get(context_);

    if (!router->HasEventListener(
            virtual_keyboard_private::OnBoundsChanged::kEventName)) {
      return;
    }

    auto event_args = std::make_unique<base::ListValue>();
    auto new_bounds = std::make_unique<base::DictionaryValue>();
    new_bounds->SetInteger("left", bounds.x());
    new_bounds->SetInteger("top", bounds.y());
    new_bounds->SetInteger("width", bounds.width());
    new_bounds->SetInteger("height", bounds.height());
    event_args->Append(std::move(new_bounds));

    auto event = std::make_unique<extensions::Event>(
        extensions::events::VIRTUAL_KEYBOARD_PRIVATE_ON_BOUNDS_CHANGED,
        virtual_keyboard_private::OnBoundsChanged::kEventName,
        std::move(event_args), context_);
    router->BroadcastEvent(std::move(event));
  }

  void OnKeyboardClosed() override {
    extensions::EventRouter* router = extensions::EventRouter::Get(context_);

    if (!router->HasEventListener(
            virtual_keyboard_private::OnKeyboardClosed::kEventName)) {
      return;
    }

    auto event = std::make_unique<extensions::Event>(
        extensions::events::VIRTUAL_KEYBOARD_PRIVATE_ON_KEYBOARD_CLOSED,
        virtual_keyboard_private::OnKeyboardClosed::kEventName,
        std::make_unique<base::ListValue>(), context_);
    router->BroadcastEvent(std::move(event));
  }

  void OnKeyboardConfigChanged() override {
    extensions::VirtualKeyboardAPI* api =
        extensions::BrowserContextKeyedAPIFactory<
            extensions::VirtualKeyboardAPI>::Get(context_);
    api->delegate()->OnKeyboardConfigChanged();
  }

 private:
  content::BrowserContext* const context_;

  DISALLOW_COPY_AND_ASSIGN(AshKeyboardControllerObserver);
};

}  // namespace

void ChromeKeyboardUI::TestApi::SetOverrideVirtualKeyboardUrl(const GURL& url) {
  GURL& override_url = GetOverrideVirtualKeyboardUrl();
  override_url = url;
}

ChromeKeyboardUI::ChromeKeyboardUI(content::BrowserContext* context)
    : browser_context_(context),
      default_url_(keyboard::kKeyboardURL),
      window_bounds_observer_(
          std::make_unique<WindowBoundsChangeObserver>(this)) {}

ChromeKeyboardUI::~ChromeKeyboardUI() {
  ResetInsets();
  DCHECK(!keyboard_controller());
}

void ChromeKeyboardUI::RequestAudioInput(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  const extensions::Extension* extension = nullptr;
  GURL origin(request.security_origin);
  if (origin.SchemeIs(extensions::kExtensionScheme)) {
    const extensions::ExtensionRegistry* registry =
        extensions::ExtensionRegistry::Get(browser_context());
    extension = registry->enabled_extensions().GetByID(origin.host());
    DCHECK(extension);
  }

  MediaCaptureDevicesDispatcher::GetInstance()->ProcessMediaAccessRequest(
      web_contents, request, callback, extension);
}

void ChromeKeyboardUI::UpdateInsetsForWindow(aura::Window* window) {
  aura::Window* keyboard_container =
      keyboard_controller()->GetContainerWindow();
  if (!ShouldWindowOverscroll(window))
    return;

  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderWidgetHostView* view = widget->GetView();
    if (view && window->Contains(view->GetNativeView())) {
      gfx::Rect window_bounds = view->GetNativeView()->GetBoundsInScreen();
      gfx::Rect intersect =
          gfx::IntersectRects(window_bounds, keyboard_container->bounds());
      int overlap = ShouldEnableInsets(window) ? intersect.height() : 0;
      if (overlap > 0 && overlap < window_bounds.height())
        view->SetInsets(gfx::Insets(0, 0, overlap, 0));
      else
        view->SetInsets(gfx::Insets());
      return;
    }
  }
}

aura::Window* ChromeKeyboardUI::GetContentsWindow() {
  if (!keyboard_contents_) {
    keyboard_contents_ = CreateWebContents();
    keyboard_contents_->SetDelegate(new ChromeKeyboardContentsDelegate(this));
    SetupWebContents(keyboard_contents_.get());
    LoadContents(GetVirtualKeyboardUrl());
    keyboard_contents_->GetNativeView()->AddObserver(this);
    content::RenderWidgetHostView* view =
        keyboard_contents_->GetMainFrame()->GetView();
    view->SetBackgroundColor(SK_ColorTRANSPARENT);
    view->GetNativeView()->SetTransparent(true);
  }

  return keyboard_contents_->GetNativeView();
}

bool ChromeKeyboardUI::HasContentsWindow() const {
  return !!keyboard_contents_;
}

bool ChromeKeyboardUI::ShouldWindowOverscroll(aura::Window* window) const {
  aura::Window* root_window = window->GetRootWindow();
  if (!root_window)
    return true;

  if (root_window != GetKeyboardRootWindow())
    return false;

  ash::RootWindowController* root_window_controller =
      ash::RootWindowController::ForWindow(root_window);
  // Shell ime window container contains virtual keyboard windows and IME
  // windows(IME windows are created by chrome.app.window.create api). They
  // should never be overscrolled.
  return !root_window_controller
              ->GetContainer(ash::kShellWindowId_ImeWindowParentContainer)
              ->Contains(window);
}

void ChromeKeyboardUI::ReloadKeyboardIfNeeded() {
  DCHECK(keyboard_contents_);
  if (keyboard_contents_->GetURL() != GetVirtualKeyboardUrl()) {
    if (keyboard_contents_->GetURL().GetOrigin() !=
        GetVirtualKeyboardUrl().GetOrigin()) {
      // Sets keyboard window rectangle to 0 and close current page before
      // navigate to a keyboard in a different extension. This keeps the UX the
      // same as Android. Note we need to explicitly close current page as it
      // might try to resize keyboard window in javascript on a resize event.
      TRACE_EVENT0("vk", "ReloadKeyboardIfNeeded");
      GetContentsWindow()->SetBounds(gfx::Rect());
      keyboard_contents_->ClosePage();
    }
    LoadContents(GetVirtualKeyboardUrl());
  }
}

void ChromeKeyboardUI::InitInsets(const gfx::Rect& new_bounds) {
  // Adjust the height of the viewport for visible windows on the primary
  // display.
  // TODO(kevers): Add EnvObserver to properly initialize insets if a
  // window is created while the keyboard is visible.
  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderWidgetHostView* view = widget->GetView();
    // Can be NULL, e.g. if the RenderWidget is being destroyed or
    // the render process crashed.
    if (view) {
      aura::Window* window = view->GetNativeView();
      // Added while we determine if RenderWidgetHostViewChildFrame can be
      // changed to always return a non-null value: https://crbug.com/644726 .
      // If we cannot guarantee a non-null value, then this may need to stay.
      if (!window)
        continue;

      if (ShouldWindowOverscroll(window)) {
        gfx::Rect window_bounds = window->GetBoundsInScreen();
        gfx::Rect intersect = gfx::IntersectRects(window_bounds, new_bounds);
        int overlap = intersect.height();

        // TODO(crbug.com/826617): get the actual obscured height from IME side.
        if (keyboard::IsFullscreenHandwritingVirtualKeyboardEnabled())
          overlap = 0;

        if (overlap > 0 && overlap < window_bounds.height())
          view->SetInsets(gfx::Insets(0, 0, overlap, 0));
        else
          view->SetInsets(gfx::Insets());
        AddBoundsChangedObserver(window);
      }
    }
  }
}

void ChromeKeyboardUI::ResetInsets() {
  const gfx::Insets insets;
  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderWidgetHostView* view = widget->GetView();
    if (view)
      view->SetInsets(insets);
  }
  window_bounds_observer_->RemoveAllObservedWindows();
}

void ChromeKeyboardUI::OnWindowBoundsChanged(aura::Window* window,
                                             const gfx::Rect& old_bounds,
                                             const gfx::Rect& new_bounds,
                                             ui::PropertyChangeReason reason) {
  SetShadowAroundKeyboard();
}

void ChromeKeyboardUI::OnWindowDestroyed(aura::Window* window) {
  window->RemoveObserver(this);
}

void ChromeKeyboardUI::OnWindowParentChanged(aura::Window* window,
                                             aura::Window* parent) {
  SetShadowAroundKeyboard();
}

const aura::Window* ChromeKeyboardUI::GetKeyboardRootWindow() const {
  return keyboard_contents_
             ? keyboard_contents_->GetNativeView()->GetRootWindow()
             : nullptr;
}

std::unique_ptr<content::WebContents> ChromeKeyboardUI::CreateWebContents() {
  content::BrowserContext* context = browser_context();
  return content::WebContents::Create(content::WebContents::CreateParams(
      context,
      content::SiteInstance::CreateForURL(context, GetVirtualKeyboardUrl())));
}

void ChromeKeyboardUI::LoadContents(const GURL& url) {
  if (keyboard_contents_) {
    TRACE_EVENT0("vk", "LoadContents");
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::SINGLETON_TAB,
                                  ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
    keyboard_contents_->OpenURL(params);
  }
}

const GURL& ChromeKeyboardUI::GetVirtualKeyboardUrl() {
  const GURL& override_url = GetOverrideVirtualKeyboardUrl();
  if (!override_url.is_empty())
    return override_url;

  chromeos::input_method::InputMethodManager* ime_manager =
      chromeos::input_method::InputMethodManager::Get();
  if (!keyboard::IsInputViewEnabled() || !ime_manager ||
      !ime_manager->GetActiveIMEState())
    return default_url_;

  const GURL& input_view_url =
      ime_manager->GetActiveIMEState()->GetInputViewUrl();
  return input_view_url.is_valid() ? input_view_url : default_url_;
}

bool ChromeKeyboardUI::ShouldEnableInsets(aura::Window* window) {
  aura::Window* contents_window = GetContentsWindow();
  return (contents_window->GetRootWindow() == window->GetRootWindow() &&
          keyboard::IsKeyboardOverscrollEnabled() &&
          contents_window->IsVisible() &&
          keyboard_controller()->keyboard_visible() &&
          !keyboard::IsFullscreenHandwritingVirtualKeyboardEnabled());
}

void ChromeKeyboardUI::AddBoundsChangedObserver(aura::Window* window) {
  aura::Window* target_window = window ? window->GetToplevelWindow() : nullptr;
  if (target_window)
    window_bounds_observer_->AddObservedWindow(target_window);
}

void ChromeKeyboardUI::SetShadowAroundKeyboard() {
  aura::Window* contents_window = keyboard_contents_->GetNativeView();
  if (!contents_window->parent())
    return;

  if (!shadow_) {
    shadow_ = std::make_unique<ui::Shadow>();
    shadow_->Init(wm::kShadowElevationActiveWindow);
    shadow_->layer()->SetVisible(true);
    contents_window->parent()->layer()->Add(shadow_->layer());
  }

  shadow_->SetContentBounds(contents_window->bounds());
}

void ChromeKeyboardUI::SetupWebContents(content::WebContents* contents) {
  extensions::SetViewType(contents, extensions::VIEW_TYPE_COMPONENT);
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      contents);
  Observe(contents);
}

ui::InputMethod* ChromeKeyboardUI::GetInputMethod() {
  aura::Window* root_window = ash::Shell::GetRootWindowForNewWindows();
  DCHECK(root_window);
  return root_window->GetHost()->GetInputMethod();
}

void ChromeKeyboardUI::SetController(keyboard::KeyboardController* controller) {
  // During KeyboardController destruction, controller can be set to null.
  if (!controller) {
    DCHECK(keyboard_controller());
    keyboard_controller()->RemoveObserver(observer_.get());
    KeyboardUI::SetController(nullptr);
    return;
  }
  KeyboardUI::SetController(controller);
  observer_ =
      std::make_unique<AshKeyboardControllerObserver>(browser_context());
  keyboard_controller()->AddObserver(observer_.get());
}

void ChromeKeyboardUI::ShowKeyboardContainer(aura::Window* container) {
  KeyboardUI::ShowKeyboardContainer(container);
}

void ChromeKeyboardUI::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetDefaultForBrowserContext(browser_context());
  DCHECK(zoom_map);
  int render_process_id = render_view_host->GetProcess()->GetID();
  int render_view_id = render_view_host->GetRoutingID();
  zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id, 0);
}
