// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/ui_controls_internal_win.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/display/win/screen_win.h"
#include "ui/events/keycodes/keyboard_code_conversion_win.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/point.h"

namespace {

// InputDispatcher ------------------------------------------------------------

// InputDispatcher is used to listen for a mouse/keyboard event. Only one
// instance may be alive at a time. The callback is run when the appropriate
// event is received.
class InputDispatcher : public base::RefCounted<InputDispatcher> {
 public:
  // Constructs a dispatcher that will invoke |callback| when |message_type| is
  // received. The returned instance does not hold a ref on itself to keep it
  // alive while waiting for messages. The caller is responsible for adding one
  // ref before returning to the message loop. The instance will release this
  // reference when the message is received.
  static scoped_refptr<InputDispatcher> CreateForMessage(
      WPARAM message_type,
      base::OnceClosure callback);

  // Constructs a dispatcher that will invoke |callback| when a mouse move
  // message has been received. Upon receipt, an error message is logged if the
  // destination of the move is not |screen_point|. |callback| is run regardless
  // after a sufficiently long delay. This generally happens when another
  // process has a window over the test's window, or if |screen_point| is not
  // over a window owned by the test. The returned instance does not hold a ref
  // on itself to keep it alive while waiting for messages. The caller is
  // responsible for adding one ref before returning to the message loop. The
  // instance will release this reference when the message is received.
  static scoped_refptr<InputDispatcher> CreateForMouseMove(
      const gfx::Point& screen_point,
      base::OnceClosure callback);

 private:
  template <typename T, typename... Args>
  friend scoped_refptr<T> base::MakeRefCounted(Args&&... args);
  friend class base::RefCounted<InputDispatcher>;

  InputDispatcher(WPARAM message_waiting_for,
                  const gfx::Point& screen_point,
                  base::OnceClosure callback);
  ~InputDispatcher();

  // Installs the dispatcher as the current hook.
  void InstallHook();

  // Uninstalls the hook set in InstallHook.
  void UninstallHook();

  // Callback from hook when a mouse message is received.
  static LRESULT CALLBACK MouseHook(int n_code, WPARAM w_param, LPARAM l_param);

  // Callback from hook when a key message is received.
  static LRESULT CALLBACK KeyHook(int n_code, WPARAM w_param, LPARAM l_param);

  // Invoked from the hook. If |message_id| matches message_waiting_for_
  // MatchingMessageFound is invoked. |mouse_hook_struct| contains extra
  // information about the mouse event.
  void DispatchedMessage(UINT message_id,
                         const MOUSEHOOKSTRUCT* mouse_hook_struct);

  // Invoked when a matching event is found. Uninstalls the hook and schedules
  // an event that runs the callback.
  void MatchingMessageFound();

  // Invoked when the hook for a mouse move is not called within a reasonable
  // time. This likely means that a window from another process is over a test
  // window, so the event does not reach this process.
  void OnTimeout();

  // The current dispatcher if a hook is installed; otherwise, nullptr;
  static InputDispatcher* current_dispatcher_;

  // Return value from SetWindowsHookEx.
  static HHOOK next_hook_;

  THREAD_CHECKER(thread_checker_);

  // The callback to run when the desired message is received.
  base::OnceClosure callback_;

  // The message on which the instance is waiting -- unsed for WM_KEYUP
  // messages.
  const WPARAM message_waiting_for_;

  // The desired mouse position for a mouse move event.
  const gfx::Point expected_mouse_location_;

  base::WeakPtrFactory<InputDispatcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputDispatcher);
};

// static
InputDispatcher* InputDispatcher::current_dispatcher_ = nullptr;

// static
HHOOK InputDispatcher::next_hook_ = nullptr;

// static
scoped_refptr<InputDispatcher> InputDispatcher::CreateForMessage(
    WPARAM message_type,
    base::OnceClosure callback) {
  DCHECK_NE(message_type, static_cast<WPARAM>(WM_MOUSEMOVE));
  return base::MakeRefCounted<InputDispatcher>(message_type, gfx::Point(0, 0),
                                               std::move(callback));
}

// static
scoped_refptr<InputDispatcher> InputDispatcher::CreateForMouseMove(
    const gfx::Point& screen_point,
    base::OnceClosure callback) {
  return base::MakeRefCounted<InputDispatcher>(WM_MOUSEMOVE, screen_point,
                                               std::move(callback));
}

InputDispatcher::InputDispatcher(WPARAM message_waiting_for,
                                 const gfx::Point& screen_point,
                                 base::OnceClosure callback)
    : callback_(std::move(callback)),
      message_waiting_for_(message_waiting_for),
      expected_mouse_location_(screen_point),
      weak_factory_(this) {
  InstallHook();
}

InputDispatcher::~InputDispatcher() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  UninstallHook();
}

void InputDispatcher::InstallHook() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!current_dispatcher_);

  current_dispatcher_ = this;

  int hook_type;
  HOOKPROC hook_function;
  if (message_waiting_for_ == WM_KEYUP) {
    hook_type = WH_KEYBOARD;
    hook_function = &KeyHook;
  } else {
    // WH_CALLWNDPROCRET does not generate mouse messages for some reason.
    hook_type = WH_MOUSE;
    hook_function = &MouseHook;
    if (message_waiting_for_ == WM_MOUSEMOVE) {
      // Things don't go well with move events sometimes. Bail out if it takes
      // too long.
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&InputDispatcher::OnTimeout,
                         weak_factory_.GetWeakPtr()),
          TestTimeouts::action_timeout());
    }
  }
  next_hook_ =
      SetWindowsHookEx(hook_type, hook_function, nullptr, GetCurrentThreadId());
  DCHECK(next_hook_);
}

void InputDispatcher::UninstallHook() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (current_dispatcher_ == this) {
    current_dispatcher_ = nullptr;
    UnhookWindowsHookEx(next_hook_);
    next_hook_ = nullptr;
    weak_factory_.InvalidateWeakPtrs();
  }
}

// static
LRESULT CALLBACK InputDispatcher::MouseHook(int n_code,
                                            WPARAM w_param,
                                            LPARAM l_param) {
  HHOOK next_hook = next_hook_;
  if (n_code == HC_ACTION) {
    DCHECK(current_dispatcher_);
    current_dispatcher_->DispatchedMessage(
        w_param, reinterpret_cast<MOUSEHOOKSTRUCT*>(l_param));
  }
  return CallNextHookEx(next_hook, n_code, w_param, l_param);
}

// static
LRESULT CALLBACK InputDispatcher::KeyHook(int n_code,
                                          WPARAM w_param,
                                          LPARAM l_param) {
  HHOOK next_hook = next_hook_;
  if (n_code == HC_ACTION) {
    DCHECK(current_dispatcher_);
    if (l_param & (1 << 30)) {
      // Only send on key up.
      current_dispatcher_->MatchingMessageFound();
    }
  }
  return CallNextHookEx(next_hook, n_code, w_param, l_param);
}

void InputDispatcher::DispatchedMessage(
    UINT message_id,
    const MOUSEHOOKSTRUCT* mouse_hook_struct) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (message_id == message_waiting_for_) {
    if (message_id == WM_MOUSEMOVE) {
      // Verify that the mouse ended up at the desired location.
      LOG_IF(ERROR,
             expected_mouse_location_ != gfx::Point(mouse_hook_struct->pt))
          << "Mouse moved to (" << mouse_hook_struct->pt.x << ", "
          << mouse_hook_struct->pt.y << ") rather than ("
          << expected_mouse_location_.x() << ", "
          << expected_mouse_location_.y()
          << "); check the math in SendMouseMoveImpl.";
    }
    MatchingMessageFound();
  } else if ((message_waiting_for_ == WM_LBUTTONDOWN &&
              message_id == WM_LBUTTONDBLCLK) ||
             (message_waiting_for_ == WM_MBUTTONDOWN &&
              message_id == WM_MBUTTONDBLCLK) ||
             (message_waiting_for_ == WM_RBUTTONDOWN &&
              message_id == WM_RBUTTONDBLCLK)) {
    LOG(WARNING) << "Double click event being treated as single-click. "
                 << "This may result in different event processing behavior. "
                 << "If you need a single click try moving the mouse between "
                 << "down events.";
    MatchingMessageFound();
  }
}

void InputDispatcher::MatchingMessageFound() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  UninstallHook();
  // The hook proc is invoked before the message is process. Post a task to run
  // the callback so that handling of this event completes first.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                std::move(callback_));
  Release();
}

void InputDispatcher::OnTimeout() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  LOG(ERROR) << "Timed out waiting for mouse move event. The test will now "
                "continue, but may fail.";
  MatchingMessageFound();
}

// Private functions ----------------------------------------------------------

UINT MapVirtualKeyToScanCode(UINT code) {
  UINT ret_code = MapVirtualKey(code, MAPVK_VK_TO_VSC);
  // We have to manually mark the following virtual
  // keys as extended or else their scancodes depend
  // on NumLock state.
  // For ex. VK_DOWN will be mapped onto either DOWN or NumPad2
  // depending on NumLock state which can lead to tests failures.
  switch (code) {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_NEXT:
    case VK_PRIOR:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
      ret_code |= KF_EXTENDED;
      break;
    default:
      break;
  }
  return ret_code;
}

// Whether scan code should be used for |key|.
// When sending keyboard events by SendInput() function, Windows does not
// "smartly" add scan code if virtual key-code is used. So these key events
// won't have scan code or DOM UI Event code string.
// But we cannot blindly send all events with scan code. For some layout
// dependent keys, the Windows may not translate them to what they used to be,
// because the test cases are usually running in headless environment with
// default keyboard layout. So fall back to use virtual key code for these keys.
bool ShouldSendThroughScanCode(ui::KeyboardCode key) {
  const DWORD native_code = ui::WindowsKeyCodeForKeyboardCode(key);
  const DWORD scan_code = MapVirtualKeyToScanCode(native_code);
  return native_code == MapVirtualKey(scan_code, MAPVK_VSC_TO_VK);
}

// Populate the INPUT structure with the appropriate keyboard event
// parameters required by SendInput
bool FillKeyboardInput(ui::KeyboardCode key, INPUT* input, bool key_up) {
  memset(input, 0, sizeof(INPUT));
  input->type = INPUT_KEYBOARD;
  input->ki.wVk = ui::WindowsKeyCodeForKeyboardCode(key);
  if (ShouldSendThroughScanCode(key)) {
    input->ki.wScan = MapVirtualKeyToScanCode(input->ki.wVk);
    // When KEYEVENTF_SCANCODE is used, ki.wVk is ignored, so we do not need to
    // clear it.
    input->ki.dwFlags = KEYEVENTF_SCANCODE;
    if ((input->ki.wScan & 0xFF00) != 0)
      input->ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
  }
  if (key_up)
    input->ki.dwFlags |= KEYEVENTF_KEYUP;

  return true;
}

}  // namespace

namespace ui_controls {
namespace internal {

bool SendKeyPressImpl(HWND window,
                      ui::KeyboardCode key,
                      bool control,
                      bool shift,
                      bool alt,
                      base::OnceClosure task) {
  // SendInput only works as we expect it if one of our windows is the
  // foreground window already.
  HWND target_window = (::GetActiveWindow() &&
                        ::GetWindow(::GetActiveWindow(), GW_OWNER) == window) ?
                       ::GetActiveWindow() :
                       window;
  if (window && ::GetForegroundWindow() != target_window)
    return false;

  scoped_refptr<InputDispatcher> dispatcher;
  if (task)
    dispatcher = InputDispatcher::CreateForMessage(WM_KEYUP, std::move(task));

  // If a pop-up menu is open, it won't receive events sent using SendInput.
  // Check for a pop-up menu using its window class (#32768) and if one
  // exists, send the key event directly there.
  HWND popup_menu = ::FindWindow(L"#32768", 0);
  if (popup_menu != NULL && popup_menu == ::GetTopWindow(NULL)) {
    WPARAM w_param = ui::WindowsKeyCodeForKeyboardCode(key);
    LPARAM l_param = 0;
    ::SendMessage(popup_menu, WM_KEYDOWN, w_param, l_param);
    ::SendMessage(popup_menu, WM_KEYUP, w_param, l_param);

    if (dispatcher)
      dispatcher->AddRef();
    return true;
  }

  INPUT input[8] = {};  // 8, assuming all the modifiers are activated.

  UINT i = 0;
  if (control) {
    if (!FillKeyboardInput(ui::VKEY_CONTROL, &input[i], false))
      return false;
    i++;
  }

  if (shift) {
    if (!FillKeyboardInput(ui::VKEY_SHIFT, &input[i], false))
      return false;
    i++;
  }

  if (alt) {
    if (!FillKeyboardInput(ui::VKEY_LMENU, &input[i], false))
      return false;
    i++;
  }

  if (!FillKeyboardInput(key, &input[i], false))
    return false;
  i++;

  if (!FillKeyboardInput(key, &input[i], true))
    return false;
  i++;

  if (alt) {
    if (!FillKeyboardInput(ui::VKEY_LMENU, &input[i], true))
      return false;
    i++;
  }

  if (shift) {
    if (!FillKeyboardInput(ui::VKEY_SHIFT, &input[i], true))
      return false;
    i++;
  }

  if (control) {
    if (!FillKeyboardInput(ui::VKEY_CONTROL, &input[i], true))
      return false;
    i++;
  }

  if (::SendInput(i, input, sizeof(INPUT)) != i)
    return false;

  if (dispatcher)
    dispatcher->AddRef();

  return true;
}

bool SendMouseMoveImpl(long screen_x, long screen_y, base::OnceClosure task) {
  gfx::Point screen_point =
      display::win::ScreenWin::DIPToScreenPoint({screen_x, screen_y});
  screen_x = screen_point.x();
  screen_y = screen_point.y();

  // Get the max screen coordinate for use in computing the normalized absolute
  // coordinates required by SendInput.
  const int max_x = ::GetSystemMetrics(SM_CXSCREEN) - 1;
  const int max_y = ::GetSystemMetrics(SM_CYSCREEN) - 1;

  // Clamp the inputs.
  if (screen_x < 0)
    screen_x = 0;
  else if (screen_x > max_x)
    screen_x = max_x;
  if (screen_y < 0)
    screen_y = 0;
  else if (screen_y > max_y)
    screen_y = max_y;

  // Check if the mouse is already there.
  POINT current_pos;
  ::GetCursorPos(&current_pos);
  if (screen_x == current_pos.x && screen_y == current_pos.y) {
    if (task)
      base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(task));
    return true;
  }

  // Form the input data containing the normalized absolute coordinates. As of
  // Windows 10 Fall Creators Update, moving to an absolute position of zero
  // does not work. It seems that moving to 1,1 does, though.
  INPUT input = {INPUT_MOUSE};
  input.mi.dx =
      static_cast<LONG>(std::max(1.0, std::ceil(screen_x * (65535.0 / max_x))));
  input.mi.dy =
      static_cast<LONG>(std::max(1.0, std::ceil(screen_y * (65535.0 / max_y))));
  input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

  scoped_refptr<InputDispatcher> dispatcher;
  if (task) {
    dispatcher = InputDispatcher::CreateForMouseMove({screen_x, screen_y},
                                                     std::move(task));
  }

  if (!::SendInput(1, &input, sizeof(input)))
    return false;

  if (dispatcher)
    dispatcher->AddRef();

  return true;
}

bool SendMouseEventsImpl(MouseButton type, int state, base::OnceClosure task) {
  DWORD down_flags = MOUSEEVENTF_ABSOLUTE;
  DWORD up_flags = MOUSEEVENTF_ABSOLUTE;
  UINT last_event;

  switch (type) {
    case LEFT:
      down_flags |= MOUSEEVENTF_LEFTDOWN;
      up_flags |= MOUSEEVENTF_LEFTUP;
      last_event = (state & UP) ? WM_LBUTTONUP : WM_LBUTTONDOWN;
      break;

    case MIDDLE:
      down_flags |= MOUSEEVENTF_MIDDLEDOWN;
      up_flags |= MOUSEEVENTF_MIDDLEUP;
      last_event = (state & UP) ? WM_MBUTTONUP : WM_MBUTTONDOWN;
      break;

    case RIGHT:
      down_flags |= MOUSEEVENTF_RIGHTDOWN;
      up_flags |= MOUSEEVENTF_RIGHTUP;
      last_event = (state & UP) ? WM_RBUTTONUP : WM_RBUTTONDOWN;
      break;

    default:
      NOTREACHED();
      return false;
  }

  scoped_refptr<InputDispatcher> dispatcher;
  if (task)
    dispatcher = InputDispatcher::CreateForMessage(last_event, std::move(task));

  INPUT input = { 0 };
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = down_flags;
  if ((state & DOWN) && !::SendInput(1, &input, sizeof(INPUT)))
    return false;

  input.mi.dwFlags = up_flags;
  if ((state & UP) && !::SendInput(1, &input, sizeof(INPUT)))
    return false;

  if (dispatcher)
    dispatcher->AddRef();

  return true;
}

bool SendTouchEventsImpl(int action, int num, int x, int y) {
  const int kTouchesLengthCap = 16;
  DCHECK_LE(num, kTouchesLengthCap);

  using InitializeTouchInjectionFn = BOOL(WINAPI*)(UINT32, DWORD);
  static InitializeTouchInjectionFn initialize_touch_injection =
      reinterpret_cast<InitializeTouchInjectionFn>(GetProcAddress(
          GetModuleHandleA("user32.dll"), "InitializeTouchInjection"));
  if (!initialize_touch_injection ||
      !initialize_touch_injection(num, TOUCH_FEEDBACK_INDIRECT)) {
    return false;
  }

  using InjectTouchInputFn = BOOL(WINAPI*)(UINT32, POINTER_TOUCH_INFO*);
  static InjectTouchInputFn inject_touch_input =
      reinterpret_cast<InjectTouchInputFn>(
          GetProcAddress(GetModuleHandleA("user32.dll"), "InjectTouchInput"));
  if (!inject_touch_input)
    return false;

  POINTER_TOUCH_INFO pointer_touch_info[kTouchesLengthCap];
  for (int i = 0; i < num; i++) {
    POINTER_TOUCH_INFO& contact = pointer_touch_info[i];
    memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));
    contact.pointerInfo.pointerType = PT_TOUCH;
    contact.pointerInfo.pointerId = i;
    contact.pointerInfo.ptPixelLocation.y = y;
    contact.pointerInfo.ptPixelLocation.x = x + 10 * i;

    contact.touchFlags = TOUCH_FLAG_NONE;
    contact.touchMask =
        TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    contact.orientation = 90;
    contact.pressure = 32000;

    // defining contact area
    contact.rcContact.top = contact.pointerInfo.ptPixelLocation.y - 2;
    contact.rcContact.bottom = contact.pointerInfo.ptPixelLocation.y + 2;
    contact.rcContact.left = contact.pointerInfo.ptPixelLocation.x - 2;
    contact.rcContact.right = contact.pointerInfo.ptPixelLocation.x + 2;

    contact.pointerInfo.pointerFlags =
        POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
  }
  // Injecting the touch down on screen
  if (!inject_touch_input(num, pointer_touch_info))
    return false;

  // Injecting the touch move on screen
  if (action & MOVE) {
    for (int i = 0; i < num; i++) {
      POINTER_TOUCH_INFO& contact = pointer_touch_info[i];
      contact.pointerInfo.ptPixelLocation.y = y + 10;
      contact.pointerInfo.ptPixelLocation.x = x + 10 * i + 30;
      contact.pointerInfo.pointerFlags =
          POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
    }
    if (!inject_touch_input(num, pointer_touch_info))
      return false;
  }

  // Injecting the touch up on screen
  if (action & RELEASE) {
    for (int i = 0; i < num; i++) {
      POINTER_TOUCH_INFO& contact = pointer_touch_info[i];
      contact.pointerInfo.ptPixelLocation.y = y + 10;
      contact.pointerInfo.ptPixelLocation.x = x + 10 * i + 30;
      contact.pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_INRANGE;
    }
    if (!inject_touch_input(num, pointer_touch_info))
      return false;
  }

  return true;
}

}  // namespace internal
}  // namespace ui_controls
