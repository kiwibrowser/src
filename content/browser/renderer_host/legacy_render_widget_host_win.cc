// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/legacy_render_widget_host_win.h"

#include <objbase.h>

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/win/win_util.h"
#include "content/browser/accessibility/browser_accessibility_manager_win.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "content/browser/accessibility/browser_accessibility_win.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/public/common/content_switches.h"
#include "ui/accessibility/platform/ax_system_caret_win.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/view_prop.h"
#include "ui/base/win/direct_manipulation.h"
#include "ui/base/win/internal_constants.h"
#include "ui/base/win/window_event_target.h"
#include "ui/compositor/compositor.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

// A custom MSAA object id used to determine if a screen reader or some
// other client is listening on MSAA events - if so, we enable full web
// accessibility support.
const int kIdScreenReaderHoneyPot = 1;

// DirectManipulation needs to poll for new events every frame while finger
// gesturing on touchpad.
class CompositorAnimationObserverForDirectManipulation
    : public ui::CompositorAnimationObserver {
 public:
  CompositorAnimationObserverForDirectManipulation(
      LegacyRenderWidgetHostHWND* render_widget_host_hwnd,
      ui::Compositor* compositor)
      : render_widget_host_hwnd_(render_widget_host_hwnd),
        compositor_(compositor) {
    DCHECK(compositor_);
    compositor_->AddAnimationObserver(this);
  }

  ~CompositorAnimationObserverForDirectManipulation() override {
    if (compositor_)
      compositor_->RemoveAnimationObserver(this);
  }

  // ui::CompositorAnimationObserver
  void OnAnimationStep(base::TimeTicks timestamp) override {
    render_widget_host_hwnd_->PollForNextEvent();
  }

  // ui::CompositorAnimationObserver
  void OnCompositingShuttingDown(ui::Compositor* compositor) override {
    compositor->RemoveAnimationObserver(this);
    compositor_ = nullptr;
  }

 private:
  LegacyRenderWidgetHostHWND* render_widget_host_hwnd_;
  ui::Compositor* compositor_;

  DISALLOW_COPY_AND_ASSIGN(CompositorAnimationObserverForDirectManipulation);
};

// static
LegacyRenderWidgetHostHWND* LegacyRenderWidgetHostHWND::Create(
    HWND parent) {
  // content_unittests passes in the desktop window as the parent. We allow
  // the LegacyRenderWidgetHostHWND instance to be created in this case for
  // these tests to pass.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableLegacyIntermediateWindow) ||
      (!GetWindowEventTarget(parent) && parent != ::GetDesktopWindow()))
    return nullptr;

  LegacyRenderWidgetHostHWND* legacy_window_instance =
      new LegacyRenderWidgetHostHWND(parent);
  // If we failed to create the child, or if the switch to disable the legacy
  // window is passed in, then return NULL.
  if (!::IsWindow(legacy_window_instance->hwnd())) {
    delete legacy_window_instance;
    return NULL;
  }
  legacy_window_instance->Init();
  return legacy_window_instance;
}

void LegacyRenderWidgetHostHWND::Destroy() {
  // Stop the AnimationObserver when window close.
  DestroyAnimationObserver();
  host_ = nullptr;
  if (::IsWindow(hwnd()))
    ::DestroyWindow(hwnd());
}

void LegacyRenderWidgetHostHWND::UpdateParent(HWND parent) {
  if (GetWindowEventTarget(GetParent()))
    GetWindowEventTarget(GetParent())->HandleParentChanged();
  // Stop the AnimationObserver when window hide. eg. tab switch, move tab to
  // another window.
  DestroyAnimationObserver();
  ::SetParent(hwnd(), parent);
}

HWND LegacyRenderWidgetHostHWND::GetParent() {
  return ::GetParent(hwnd());
}

void LegacyRenderWidgetHostHWND::Show() {
  ::ShowWindow(hwnd(), SW_SHOW);
  if (direct_manipulation_helper_)
    direct_manipulation_helper_->Activate();
}

void LegacyRenderWidgetHostHWND::Hide() {
  ::ShowWindow(hwnd(), SW_HIDE);
  if (direct_manipulation_helper_)
    direct_manipulation_helper_->Deactivate();
}

void LegacyRenderWidgetHostHWND::SetBounds(const gfx::Rect& bounds) {
  gfx::Rect bounds_in_pixel = display::win::ScreenWin::DIPToClientRect(hwnd(),
                                                                       bounds);
  ::SetWindowPos(hwnd(), NULL, bounds_in_pixel.x(), bounds_in_pixel.y(),
                 bounds_in_pixel.width(), bounds_in_pixel.height(),
                 SWP_NOREDRAW);
  if (direct_manipulation_helper_)
    direct_manipulation_helper_->SetSize(bounds_in_pixel.size());
}

void LegacyRenderWidgetHostHWND::MoveCaretTo(const gfx::Rect& bounds) {
  DCHECK(ax_system_caret_);
  ax_system_caret_->MoveCaretTo(bounds);
}

void LegacyRenderWidgetHostHWND::OnFinalMessage(HWND hwnd) {
  if (host_) {
    host_->OnLegacyWindowDestroyed();
    host_ = NULL;
  }

  // Re-enable flicks for just a moment
  base::win::EnableFlicks(hwnd);

  delete this;
}

LegacyRenderWidgetHostHWND::LegacyRenderWidgetHostHWND(HWND parent)
    : mouse_tracking_enabled_(false), host_(nullptr) {
  RECT rect = {0};
  Base::Create(parent, rect, L"Chrome Legacy Window",
               WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
               WS_EX_TRANSPARENT);
  // We create a system caret regardless of accessibility mode since not all
  // assistive software that makes use of a caret is classified as a screen
  // reader, e.g. the built-in Windows Magnifier.
  ax_system_caret_ = std::make_unique<ui::AXSystemCaretWin>(hwnd());
}

LegacyRenderWidgetHostHWND::~LegacyRenderWidgetHostHWND() {
  DCHECK(!::IsWindow(hwnd()));
}

bool LegacyRenderWidgetHostHWND::Init() {
  // Only register a touch window if we are using WM_TOUCH.
  if (!features::IsUsingWMPointerForTouch())
    RegisterTouchWindow(hwnd(), TWF_WANTPALM);

  HRESULT hr = ::CreateStdAccessibleObject(hwnd(), OBJID_WINDOW,
                                           IID_PPV_ARGS(&window_accessible_));
  DCHECK(SUCCEEDED(hr));

  ui::AXMode mode =
      BrowserAccessibilityStateImpl::GetInstance()->accessibility_mode();
  if (!mode.has_mode(ui::AXMode::kNativeAPIs)) {
    // Attempt to detect screen readers or other clients who want full
    // accessibility support, by seeing if they respond to this event.
    NotifyWinEvent(EVENT_SYSTEM_ALERT, hwnd(), kIdScreenReaderHoneyPot,
                   CHILDID_SELF);
  }

  // Direct Manipulation is enabled on Windows 10+. The CreateInstance function
  // returns NULL if Direct Manipulation is not available.
  direct_manipulation_helper_ =
      ui::win::DirectManipulationHelper::CreateInstance(
          hwnd(), GetWindowEventTarget(GetParent()));

  // Disable pen flicks (http://crbug.com/506977)
  base::win::DisableFlicks(hwnd());

  return !!SUCCEEDED(hr);
}

// static
ui::WindowEventTarget* LegacyRenderWidgetHostHWND::GetWindowEventTarget(
    HWND parent) {
  return reinterpret_cast<ui::WindowEventTarget*>(ui::ViewProp::GetValue(
      parent, ui::WindowEventTarget::kWin32InputEventTarget));
}

LRESULT LegacyRenderWidgetHostHWND::OnEraseBkGnd(UINT message,
                                                 WPARAM w_param,
                                                 LPARAM l_param) {
  return 1;
}

LRESULT LegacyRenderWidgetHostHWND::OnGetObject(UINT message,
                                                WPARAM w_param,
                                                LPARAM l_param) {
  // Only the lower 32 bits of l_param are valid when checking the object id
  // because it sometimes gets sign-extended incorrectly (but not always).
  DWORD obj_id = static_cast<DWORD>(static_cast<DWORD_PTR>(l_param));

  if (kIdScreenReaderHoneyPot == obj_id) {
    // When an MSAA client has responded to our fake event on this id,
    // enable basic accessibility support. (Full screen reader support is
    // detected later when specific more advanced APIs are accessed.)
    BrowserAccessibilityStateImpl::GetInstance()->AddAccessibilityModeFlags(
        ui::AXMode::kNativeAPIs | ui::AXMode::kWebContents);
    return static_cast<LRESULT>(0L);
  }

  if (!host_)
    return static_cast<LRESULT>(0L);

  if (static_cast<DWORD>(OBJID_CLIENT) == obj_id) {
    RenderWidgetHostImpl* rwhi =
        RenderWidgetHostImpl::From(host_->GetRenderWidgetHost());
    if (!rwhi)
      return static_cast<LRESULT>(0L);

    BrowserAccessibilityManagerWin* manager =
        static_cast<BrowserAccessibilityManagerWin*>(
            rwhi->GetRootBrowserAccessibilityManager());
    if (!manager || !manager->GetRoot())
      return static_cast<LRESULT>(0L);

    Microsoft::WRL::ComPtr<IAccessible> root(
        ToBrowserAccessibilityWin(manager->GetRoot())->GetCOM());
    return LresultFromObject(IID_IAccessible, w_param,
                             static_cast<IAccessible*>(root.Detach()));
  }

  if (static_cast<DWORD>(OBJID_CARET) == obj_id && host_->HasFocus()) {
    DCHECK(ax_system_caret_);
    Microsoft::WRL::ComPtr<IAccessible> ax_system_caret_accessible =
        ax_system_caret_->GetCaret();
    return LresultFromObject(IID_IAccessible, w_param,
                             ax_system_caret_accessible.Detach());
  }

  return static_cast<LRESULT>(0L);
}

// We send keyboard/mouse/touch messages to the parent window via SendMessage.
// While this works, this has the side effect of converting input messages into
// sent messages which changes their priority and could technically result
// in these messages starving other messages in the queue. Additionally
// keyboard/mouse hooks would not see these messages. The alternative approach
// is to set and release capture as needed on the parent to ensure that it
// receives all mouse events. However that was shelved due to possible issues
// with capture changes.
LRESULT LegacyRenderWidgetHostHWND::OnKeyboardRange(UINT message,
                                                    WPARAM w_param,
                                                    LPARAM l_param,
                                                    BOOL& handled) {
  LRESULT ret = 0;
  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    ret = GetWindowEventTarget(GetParent())->HandleKeyboardMessage(
        message, w_param, l_param, &msg_handled);
    handled = msg_handled;
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnMouseRange(UINT message,
                                                 WPARAM w_param,
                                                 LPARAM l_param,
                                                 BOOL& handled) {
  if (message == WM_MOUSEMOVE) {
    if (!mouse_tracking_enabled_) {
      mouse_tracking_enabled_ = true;
      TRACKMOUSEEVENT tme;
      tme.cbSize = sizeof(tme);
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = hwnd();
      tme.dwHoverTime = 0;
      TrackMouseEvent(&tme);
    }
  }
  // The offsets for WM_NCXXX and WM_MOUSEWHEEL and WM_MOUSEHWHEEL messages are
  // in screen coordinates. We should not be converting them to parent
  // coordinates.
  if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) &&
      (message != WM_MOUSEWHEEL && message != WM_MOUSEHWHEEL)) {
    POINT mouse_coords;
    mouse_coords.x = GET_X_LPARAM(l_param);
    mouse_coords.y = GET_Y_LPARAM(l_param);
    ::MapWindowPoints(hwnd(), GetParent(), &mouse_coords, 1);
    l_param = MAKELPARAM(mouse_coords.x, mouse_coords.y);
  }

  LRESULT ret = 0;

  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    ret = GetWindowEventTarget(GetParent())->HandleMouseMessage(
        message, w_param, l_param, &msg_handled);
    handled = msg_handled;
    // If the parent did not handle non client mouse messages, we call
    // DefWindowProc on the message with the parent window handle. This
    // ensures that WM_SYSCOMMAND is generated for the parent and we are
    // out of the picture.
    if (!handled &&
         (message >= WM_NCMOUSEMOVE && message <= WM_NCXBUTTONDBLCLK)) {
      ret = ::DefWindowProc(GetParent(), message, w_param, l_param);
      handled = TRUE;
    }
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnMouseLeave(UINT message,
                                                 WPARAM w_param,
                                                 LPARAM l_param) {
  mouse_tracking_enabled_ = false;
  LRESULT ret = 0;
  if ((::GetCapture() != GetParent()) && GetWindowEventTarget(GetParent())) {
    // We should send a WM_MOUSELEAVE to the parent window only if the mouse
    // has moved outside the bounds of the parent.
    POINT cursor_pos;
    ::GetCursorPos(&cursor_pos);
    if (::WindowFromPoint(cursor_pos) != GetParent()) {
      bool msg_handled = false;
      ret = GetWindowEventTarget(GetParent())->HandleMouseMessage(
          message, w_param, l_param, &msg_handled);
      SetMsgHandled(msg_handled);
    }
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnMouseActivate(UINT message,
                                                    WPARAM w_param,
                                                    LPARAM l_param) {
  // Don't pass this to DefWindowProc. That results in the WM_MOUSEACTIVATE
  // message going all the way to the parent which then messes up state
  // related to focused views, etc. This is because it treats this as if
  // it lost activation.
  // Our dummy window should not interfere with focus and activation in
  // the parent. Return MA_ACTIVATE here ensures that focus state in the parent
  // is preserved. The only exception is if the parent was created with the
  // WS_EX_NOACTIVATE style.
  if (::GetWindowLong(GetParent(), GWL_EXSTYLE) & WS_EX_NOACTIVATE)
    return MA_NOACTIVATE;
  // On Windows, if we select the menu item by touch and if the window at the
  // location is another window on the same thread, that window gets a
  // WM_MOUSEACTIVATE message and ends up activating itself, which is not
  // correct. We workaround this by setting a property on the window at the
  // current cursor location. We check for this property in our
  // WM_MOUSEACTIVATE handler and don't activate the window if the property is
  // set.
  if (::GetProp(hwnd(), ui::kIgnoreTouchMouseActivateForWindow)) {
    ::RemoveProp(hwnd(), ui::kIgnoreTouchMouseActivateForWindow);
    return MA_NOACTIVATE;
  }
  return MA_ACTIVATE;
}

LRESULT LegacyRenderWidgetHostHWND::OnPointer(UINT message,
                                              WPARAM w_param,
                                              LPARAM l_param) {
  LRESULT ret = 0;
  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    ret = GetWindowEventTarget(GetParent())
              ->HandlePointerMessage(message, w_param, l_param, &msg_handled);
    SetMsgHandled(msg_handled);
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnTouch(UINT message,
                                            WPARAM w_param,
                                            LPARAM l_param) {
  LRESULT ret = 0;
  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    ret = GetWindowEventTarget(GetParent())->HandleTouchMessage(
        message, w_param, l_param, &msg_handled);
    SetMsgHandled(msg_handled);
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnScroll(UINT message,
                                             WPARAM w_param,
                                             LPARAM l_param) {
  LRESULT ret = 0;
  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    ret = GetWindowEventTarget(GetParent())->HandleScrollMessage(
        message, w_param, l_param, &msg_handled);
    SetMsgHandled(msg_handled);
  }
  return ret;
}

LRESULT LegacyRenderWidgetHostHWND::OnNCHitTest(UINT message,
                                                WPARAM w_param,
                                                LPARAM l_param) {
  if (GetWindowEventTarget(GetParent())) {
    bool msg_handled = false;
    LRESULT hit_test = GetWindowEventTarget(
        GetParent())->HandleNcHitTestMessage(message, w_param, l_param,
                                             &msg_handled);
    // If the parent returns HTNOWHERE which can happen for popup windows, etc
    // we return HTCLIENT.
    if (hit_test == HTNOWHERE)
      hit_test = HTCLIENT;
    return hit_test;
  }
  return HTNOWHERE;
}

LRESULT LegacyRenderWidgetHostHWND::OnNCPaint(UINT message,
                                              WPARAM w_param,
                                              LPARAM l_param) {
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnPaint(UINT message,
                                            WPARAM w_param,
                                            LPARAM l_param) {
  PAINTSTRUCT ps = {0};
  ::BeginPaint(hwnd(), &ps);
  ::EndPaint(hwnd(), &ps);
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnSetCursor(UINT message,
                                                WPARAM w_param,
                                                LPARAM l_param) {
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnNCCalcSize(UINT message,
                                                 WPARAM w_param,
                                                 LPARAM l_param) {
  // Prevent scrollbars, etc from drawing.
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnSize(UINT message,
                                           WPARAM w_param,
                                           LPARAM l_param) {
  // Certain trackpad drivers on Windows have bugs where in they don't generate
  // WM_MOUSEWHEEL messages for the trackpoint and trackpad scrolling gestures
  // unless there is an entry for Chrome with the class name of the Window.
  // Additionally others check if the window WS_VSCROLL/WS_HSCROLL styles and
  // generate the legacy WM_VSCROLL/WM_HSCROLL messages.
  // We add these styles to ensure that trackpad/trackpoint scrolling
  // work.
  long current_style = ::GetWindowLong(hwnd(), GWL_STYLE);
  ::SetWindowLong(hwnd(), GWL_STYLE,
                  current_style | WS_VSCROLL | WS_HSCROLL);
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnWindowPosChanged(UINT message,
                                                       WPARAM w_param,
                                                       LPARAM l_param) {
  WINDOWPOS* window_pos = reinterpret_cast<WINDOWPOS*>(l_param);
  if (direct_manipulation_helper_) {
    if (window_pos->flags & SWP_SHOWWINDOW) {
      direct_manipulation_helper_->Activate();
    } else if (window_pos->flags & SWP_HIDEWINDOW) {
      direct_manipulation_helper_->Deactivate();
    }
  }
  SetMsgHandled(FALSE);
  return 0;
}

LRESULT LegacyRenderWidgetHostHWND::OnPointerHitTest(UINT message,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
  if (!direct_manipulation_helper_)
    return 0;

  // Update window event target for each DM_POINTERHITTEST.
  if (direct_manipulation_helper_->OnPointerHitTest(
          w_param, GetWindowEventTarget(GetParent()))) {
    if (compositor_animation_observer_) {
      // This is reach if Windows send a DM_POINTERHITTEST before the last
      // DM_POINTERHITTEST receive READY status. We never see this but still
      // worth to handle it.
      return 0;
    }

    CreateAnimationObserver();
  }

  return 0;
}

void LegacyRenderWidgetHostHWND::PollForNextEvent() {
  DCHECK(direct_manipulation_helper_);

  if (!direct_manipulation_helper_->PollForNextEvent())
    DestroyAnimationObserver();
}

void LegacyRenderWidgetHostHWND::CreateAnimationObserver() {
  DCHECK(!compositor_animation_observer_);
  DCHECK(host_);
  DCHECK(host_->GetNativeView()->GetHost());
  DCHECK(host_->GetNativeView()->GetHost()->compositor());

  compositor_animation_observer_ =
      std::make_unique<CompositorAnimationObserverForDirectManipulation>(
          this, host_->GetNativeView()->GetHost()->compositor());
}

void LegacyRenderWidgetHostHWND::DestroyAnimationObserver() {
  compositor_animation_observer_.reset();
}

}  // namespace content
