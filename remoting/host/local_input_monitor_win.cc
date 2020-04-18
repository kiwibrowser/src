// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/local_input_monitor.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/win/message_window.h"
#include "remoting/host/client_session_control.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

namespace remoting {

namespace {

// From the HID Usage Tables specification.
const USHORT kGenericDesktopPage = 1;
const USHORT kMouseUsage = 2;

class LocalInputMonitorWin : public LocalInputMonitor {
 public:
  LocalInputMonitorWin(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      base::WeakPtr<ClientSessionControl> client_session_control);
  ~LocalInputMonitorWin() override;

 private:
  // The actual implementation resides in LocalInputMonitorWin::Core class.
  class Core : public base::RefCountedThreadSafe<Core> {
   public:
    Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
         scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
         base::WeakPtr<ClientSessionControl> client_session_control);

    void Start();
    void Stop();

   private:
    friend class base::RefCountedThreadSafe<Core>;
    virtual ~Core();

    void StartOnUiThread();
    void StopOnUiThread();

    // Handles WM_INPUT messages.
    LRESULT OnInput(HRAWINPUT input_handle);

    // Handles messages received by |window_|.
    bool HandleMessage(UINT message,
                       WPARAM wparam,
                       LPARAM lparam,
                       LRESULT* result);

    // Task runner on which public methods of this class must be called.
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

    // Task runner on which |window_| is created.
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

    // Used to receive raw input.
    std::unique_ptr<base::win::MessageWindow> window_;

    // Points to the object receiving mouse event notifications.
    base::WeakPtr<ClientSessionControl> client_session_control_;

    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  scoped_refptr<Core> core_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(LocalInputMonitorWin);
};

LocalInputMonitorWin::LocalInputMonitorWin(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : core_(new Core(caller_task_runner,
                     ui_task_runner,
                     client_session_control)) {
  core_->Start();
}

LocalInputMonitorWin::~LocalInputMonitorWin() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  core_->Stop();
}

LocalInputMonitorWin::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : caller_task_runner_(caller_task_runner),
      ui_task_runner_(ui_task_runner),
      client_session_control_(client_session_control) {
  DCHECK(client_session_control_);
}

void LocalInputMonitorWin::Core::Start() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  ui_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&Core::StartOnUiThread, this));
}

void LocalInputMonitorWin::Core::Stop() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  ui_task_runner_->PostTask(FROM_HERE, base::Bind(&Core::StopOnUiThread, this));
}

LocalInputMonitorWin::Core::~Core() {
  DCHECK(!window_);
}

void LocalInputMonitorWin::Core::StartOnUiThread() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  window_.reset(new base::win::MessageWindow());
  if (!window_->Create(base::Bind(&Core::HandleMessage,
                                  base::Unretained(this)))) {
    PLOG(ERROR) << "Failed to create the raw input window";
    window_.reset();

    // If the local input cannot be monitored, the remote user can take over
    // the session. Disconnect the session now to prevent this.
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&ClientSessionControl::DisconnectSession,
                              client_session_control_, protocol::OK));
  }
}

void LocalInputMonitorWin::Core::StopOnUiThread() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  // Stop receiving  raw mouse input.
  if (window_) {
    RAWINPUTDEVICE device = {0};
    device.dwFlags = RIDEV_REMOVE;
    device.usUsagePage = kGenericDesktopPage;
    device.usUsage = kMouseUsage;
    device.hwndTarget = nullptr;

    // The error is harmless, ignore it.
    RegisterRawInputDevices(&device, 1, sizeof(device));
  }

  window_.reset();
}

LRESULT LocalInputMonitorWin::Core::OnInput(HRAWINPUT input_handle) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  // Get the size of the input record.
  UINT size = 0;
  UINT result = GetRawInputData(input_handle,
                                RID_INPUT,
                                nullptr,
                                &size,
                                sizeof(RAWINPUTHEADER));
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputData() failed";
    return 0;
  }

  // Retrieve the input record itself.
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  RAWINPUT* input = reinterpret_cast<RAWINPUT*>(buffer.get());
  result = GetRawInputData(input_handle,
                           RID_INPUT,
                           buffer.get(),
                           &size,
                           sizeof(RAWINPUTHEADER));
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputData() failed";
    return 0;
  }

  // Notify the observer about mouse events generated locally. Remote (injected)
  // mouse events do not specify a device handle (based on observed behavior).
  if (input->header.dwType == RIM_TYPEMOUSE &&
      input->header.hDevice != nullptr) {
    POINT position;
    if (!GetCursorPos(&position)) {
      position.x = 0;
      position.y = 0;
    }

    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&ClientSessionControl::OnLocalMouseMoved,
                              client_session_control_,
                              webrtc::DesktopVector(position.x, position.y)));
  }

  return DefRawInputProc(&input, 1, sizeof(RAWINPUTHEADER));
}

bool LocalInputMonitorWin::Core::HandleMessage(
    UINT message, WPARAM wparam, LPARAM lparam, LRESULT* result) {
  switch (message) {
    case WM_CREATE: {
      // Register to receive raw mouse input.
      RAWINPUTDEVICE device = {0};
      device.dwFlags = RIDEV_INPUTSINK;
      device.usUsagePage = kGenericDesktopPage;
      device.usUsage = kMouseUsage;
      device.hwndTarget = window_->hwnd();
      if (RegisterRawInputDevices(&device, 1, sizeof(device))) {
        *result = 0;
      } else {
        PLOG(ERROR) << "RegisterRawInputDevices() failed";
        *result = -1;
      }
      return true;
    }

    case WM_INPUT:
      *result = OnInput(reinterpret_cast<HRAWINPUT>(lparam));
      return true;

    default:
      return false;
  }
}

}  // namespace

std::unique_ptr<LocalInputMonitor> LocalInputMonitor::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control) {
  return base::WrapUnique(new LocalInputMonitorWin(
      caller_task_runner, ui_task_runner, client_session_control));
}

}  // namespace remoting
