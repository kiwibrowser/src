// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/local_input_monitor.h"

#include <sys/select.h>
#include <unistd.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "remoting/host/client_session_control.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"
#include "ui/gfx/x/x11.h"

namespace remoting {

namespace {

class LocalInputMonitorX11 : public LocalInputMonitor {
 public:
  LocalInputMonitorX11(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
      base::WeakPtr<ClientSessionControl> client_session_control);
  ~LocalInputMonitorX11() override;

 private:
  // The actual implementation resides in LocalInputMonitorX11::Core class.
  class Core : public base::RefCountedThreadSafe<Core> {
   public:
    Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
         scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
         base::WeakPtr<ClientSessionControl> client_session_control);

    void Start();
    void Stop();

   private:
    friend class base::RefCountedThreadSafe<Core>;
    ~Core();

    void StartOnInputThread();
    void StopOnInputThread();

    // Called when there are pending X events.
    void OnPendingXEvents();

    // Processes key and mouse events.
    void ProcessXEvent(xEvent* event);

    static void ProcessReply(XPointer self, XRecordInterceptData* data);

    // Task runner on which public methods of this class must be called.
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

    // Task runner on which X Window events are received.
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner_;

    // Points to the object receiving mouse event notifications and session
    // disconnect requests.
    base::WeakPtr<ClientSessionControl> client_session_control_;

    // Controls watching X events.
    std::unique_ptr<base::FileDescriptorWatcher::Controller> controller_;

    // True when Alt is pressed.
    bool alt_pressed_;

    // True when Ctrl is pressed.
    bool ctrl_pressed_;

    Display* display_;
    Display* x_record_display_;
    XRecordRange* x_record_range_[2];
    XRecordContext x_record_context_;

    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  scoped_refptr<Core> core_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(LocalInputMonitorX11);
};

LocalInputMonitorX11::LocalInputMonitorX11(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : core_(new Core(caller_task_runner,
                     input_task_runner,
                     client_session_control)) {
  core_->Start();
}

LocalInputMonitorX11::~LocalInputMonitorX11() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  core_->Stop();
}

LocalInputMonitorX11::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : caller_task_runner_(caller_task_runner),
      input_task_runner_(input_task_runner),
      client_session_control_(client_session_control),
      alt_pressed_(false),
      ctrl_pressed_(false),
      display_(nullptr),
      x_record_display_(nullptr),
      x_record_context_(0) {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  DCHECK(client_session_control_.get());

  x_record_range_[0] = nullptr;
  x_record_range_[1] = nullptr;
}

void LocalInputMonitorX11::Core::Start() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  input_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Core::StartOnInputThread, this));
}

void LocalInputMonitorX11::Core::Stop() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  input_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Core::StopOnInputThread, this));
}

LocalInputMonitorX11::Core::~Core() {
  DCHECK(!display_);
  DCHECK(!x_record_display_);
  DCHECK(!x_record_range_[0]);
  DCHECK(!x_record_range_[1]);
  DCHECK(!x_record_context_);
}

void LocalInputMonitorX11::Core::StartOnInputThread() {
  DCHECK(input_task_runner_->BelongsToCurrentThread());
  DCHECK(!display_);
  DCHECK(!x_record_display_);
  DCHECK(!x_record_range_[0]);
  DCHECK(!x_record_range_[1]);
  DCHECK(!x_record_context_);

  // TODO(jamiewalch): We should pass the display in. At that point, since
  // XRecord needs a private connection to the X Server for its data channel
  // and both channels are used from a separate thread, we'll need to duplicate
  // them with something like the following:
  //   XOpenDisplay(DisplayString(display));
  display_ = XOpenDisplay(nullptr);
  x_record_display_ = XOpenDisplay(nullptr);
  if (!display_ || !x_record_display_) {
    LOG(ERROR) << "Couldn't open X display";
    return;
  }

  int xr_opcode, xr_event, xr_error;
  if (!XQueryExtension(display_, "RECORD", &xr_opcode, &xr_event, &xr_error)) {
    LOG(ERROR) << "X Record extension not available.";
    return;
  }

  x_record_range_[0] = XRecordAllocRange();
  x_record_range_[1] = XRecordAllocRange();
  if (!x_record_range_[0] || !x_record_range_[1]) {
    LOG(ERROR) << "XRecordAllocRange failed.";
    return;
  }
  x_record_range_[0]->device_events.first = MotionNotify;
  x_record_range_[0]->device_events.last = MotionNotify;
  x_record_range_[1]->device_events.first = KeyPress;
  x_record_range_[1]->device_events.last = KeyRelease;
  XRecordClientSpec client_spec = XRecordAllClients;

  x_record_context_ = XRecordCreateContext(
      x_record_display_, 0, &client_spec, 1, x_record_range_,
      arraysize(x_record_range_));
  if (!x_record_context_) {
    LOG(ERROR) << "XRecordCreateContext failed.";
    return;
  }

  if (!XRecordEnableContextAsync(x_record_display_, x_record_context_,
                                 &Core::ProcessReply,
                                 reinterpret_cast<XPointer>(this))) {
    LOG(ERROR) << "XRecordEnableContextAsync failed.";
    return;
  }

  // Register OnPendingXEvents() to be called every time there is
  // something to read from |x_record_display_|.
  controller_ = base::FileDescriptorWatcher::WatchReadable(
      ConnectionNumber(x_record_display_),
      base::Bind(&Core::OnPendingXEvents, base::Unretained(this)));

  // Fetch pending events if any.
  while (XPending(x_record_display_)) {
    XEvent ev;
    XNextEvent(x_record_display_, &ev);
  }
}

void LocalInputMonitorX11::Core::StopOnInputThread() {
  DCHECK(input_task_runner_->BelongsToCurrentThread());

  // Context must be disabled via the control channel because we can't send
  // any X protocol traffic over the data channel while it's recording.
  if (x_record_context_) {
    XRecordDisableContext(display_, x_record_context_);
    XFlush(display_);
  }

  controller_.reset();

  if (x_record_range_[0]) {
    XFree(x_record_range_[0]);
    x_record_range_[0] = nullptr;
  }
  if (x_record_range_[1]) {
    XFree(x_record_range_[1]);
    x_record_range_[1] = nullptr;
  }
  if (x_record_context_) {
    XRecordFreeContext(x_record_display_, x_record_context_);
    x_record_context_ = 0;
  }
  if (x_record_display_) {
    XCloseDisplay(x_record_display_);
    x_record_display_ = nullptr;
  }
  if (display_) {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}

void LocalInputMonitorX11::Core::OnPendingXEvents() {
  DCHECK(input_task_runner_->BelongsToCurrentThread());

  // Fetch pending events if any.
  while (XPending(x_record_display_)) {
    XEvent ev;
    XNextEvent(x_record_display_, &ev);
  }
}

void LocalInputMonitorX11::Core::ProcessXEvent(xEvent* event) {
  DCHECK(input_task_runner_->BelongsToCurrentThread());

  if (event->u.u.type == MotionNotify) {
    webrtc::DesktopVector position(event->u.keyButtonPointer.rootX,
                                   event->u.keyButtonPointer.rootY);
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&ClientSessionControl::OnLocalMouseMoved,
                              client_session_control_,
                              position));
  } else {
    int key_code = event->u.u.detail;
    bool down = event->u.u.type == KeyPress;
    KeySym key_sym = XkbKeycodeToKeysym(display_, key_code, 0, 0);
    if (key_sym == XK_Control_L || key_sym == XK_Control_R) {
      ctrl_pressed_ = down;
    } else if (key_sym == XK_Alt_L || key_sym == XK_Alt_R) {
      alt_pressed_ = down;
    } else if (key_sym == XK_Escape && down && alt_pressed_ && ctrl_pressed_) {
      caller_task_runner_->PostTask(
          FROM_HERE, base::Bind(&ClientSessionControl::DisconnectSession,
                                client_session_control_, protocol::OK));
    }
  }
}

// static
void LocalInputMonitorX11::Core::ProcessReply(XPointer self,
                                                XRecordInterceptData* data) {
  if (data->category == XRecordFromServer) {
    xEvent* event = reinterpret_cast<xEvent*>(data->data);
    reinterpret_cast<Core*>(self)->ProcessXEvent(event);
  }
  XRecordFreeData(data);
}

}  // namespace

std::unique_ptr<LocalInputMonitor> LocalInputMonitor::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control) {
  return base::WrapUnique(new LocalInputMonitorX11(
      caller_task_runner, input_task_runner, client_session_control));
}

}  // namespace remoting
