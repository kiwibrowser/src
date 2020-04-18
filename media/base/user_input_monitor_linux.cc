// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/user_input_monitor.h"

#include <stddef.h>
#include <sys/select.h>
#include <unistd.h>
#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "media/base/keyboard_event_counter.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"

namespace media {
namespace {

// This is the actual implementation of event monitoring. It's separated from
// UserInputMonitorLinux since it needs to be deleted on the IO thread.
class UserInputMonitorLinuxCore
    : public base::SupportsWeakPtr<UserInputMonitorLinuxCore>,
      public base::MessageLoopCurrent::DestructionObserver {
 public:
  explicit UserInputMonitorLinuxCore(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~UserInputMonitorLinuxCore() override;

  // DestructionObserver overrides.
  void WillDestroyCurrentMessageLoop() override;

  uint32_t GetKeyPressCount() const;
  void StartMonitor();
  void StartMonitorWithMapping(base::WritableSharedMemoryMapping mapping);
  void StopMonitor();

 private:
  void OnXEvent();

  // Processes key events.
  void ProcessXEvent(xEvent* event);
  static void ProcessReply(XPointer self, XRecordInterceptData* data);

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Used for sharing key press count value.
  std::unique_ptr<base::WritableSharedMemoryMapping> key_press_count_mapping_;

  //
  // The following members should only be accessed on the IO thread.
  //
  std::unique_ptr<base::FileDescriptorWatcher::Controller> watch_controller_;
  Display* x_control_display_;
  Display* x_record_display_;
  XRecordRange* x_record_range_;
  XRecordContext x_record_context_;
  KeyboardEventCounter counter_;

  DISALLOW_COPY_AND_ASSIGN(UserInputMonitorLinuxCore);
};

class UserInputMonitorLinux : public UserInputMonitorBase {
 public:
  explicit UserInputMonitorLinux(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~UserInputMonitorLinux() override;

  // Public UserInputMonitor overrides.
  uint32_t GetKeyPressCount() const override;

 private:
  // Private UserInputMonitor overrides.
  void StartKeyboardMonitoring() override;
  void StartKeyboardMonitoring(
      base::WritableSharedMemoryMapping mapping) override;
  void StopKeyboardMonitoring() override;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  UserInputMonitorLinuxCore* core_;

  DISALLOW_COPY_AND_ASSIGN(UserInputMonitorLinux);
};

UserInputMonitorLinuxCore::UserInputMonitorLinuxCore(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : io_task_runner_(io_task_runner),
      x_control_display_(NULL),
      x_record_display_(NULL),
      x_record_range_(NULL),
      x_record_context_(0) {}

UserInputMonitorLinuxCore::~UserInputMonitorLinuxCore() {
  DCHECK(!x_control_display_);
  DCHECK(!x_record_display_);
  DCHECK(!x_record_range_);
  DCHECK(!x_record_context_);
}

void UserInputMonitorLinuxCore::WillDestroyCurrentMessageLoop() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  StopMonitor();
}

uint32_t UserInputMonitorLinuxCore::GetKeyPressCount() const {
  return counter_.GetKeyPressCount();
}

void UserInputMonitorLinuxCore::StartMonitor() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // TODO(jamiewalch): We should pass the display in. At that point, since
  // XRecord needs a private connection to the X Server for its data channel
  // and both channels are used from a separate thread, we'll need to duplicate
  // them with something like the following:
  //   XOpenDisplay(DisplayString(display));
  if (!x_control_display_)
    x_control_display_ = gfx::OpenNewXDisplay();

  if (!x_record_display_)
    x_record_display_ = gfx::OpenNewXDisplay();

  if (!x_control_display_ || !x_record_display_) {
    LOG(ERROR) << "Couldn't open X display";
    StopMonitor();
    return;
  }

  int xr_opcode, xr_event, xr_error;
  if (!XQueryExtension(
           x_control_display_, "RECORD", &xr_opcode, &xr_event, &xr_error)) {
    LOG(ERROR) << "X Record extension not available.";
    StopMonitor();
    return;
  }

  if (!x_record_range_)
    x_record_range_ = XRecordAllocRange();

  if (!x_record_range_) {
    LOG(ERROR) << "XRecordAllocRange failed.";
    StopMonitor();
    return;
  }

  x_record_range_->device_events.first = KeyPress;
  x_record_range_->device_events.last = KeyRelease;

  if (x_record_context_) {
    XRecordDisableContext(x_control_display_, x_record_context_);
    XFlush(x_control_display_);
    XRecordFreeContext(x_record_display_, x_record_context_);
    x_record_context_ = 0;
  }
  int number_of_ranges = 1;

  XRecordClientSpec client_spec = XRecordAllClients;
  x_record_context_ =
      XRecordCreateContext(x_record_display_, 0, &client_spec, 1,
                           &x_record_range_, number_of_ranges);
  if (!x_record_context_) {
    LOG(ERROR) << "XRecordCreateContext failed.";
    StopMonitor();
    return;
  }

  if (!XRecordEnableContextAsync(x_record_display_,
                                 x_record_context_,
                                 &UserInputMonitorLinuxCore::ProcessReply,
                                 reinterpret_cast<XPointer>(this))) {
    LOG(ERROR) << "XRecordEnableContextAsync failed.";
    StopMonitor();
    return;
  }

  // Register OnXEvent() to be called every time there is something to read
  // from |x_record_display_|.
  watch_controller_ = base::FileDescriptorWatcher::WatchReadable(
      ConnectionNumber(x_record_display_),
      base::Bind(&UserInputMonitorLinuxCore::OnXEvent, base::Unretained(this)));

  // Start observing message loop destruction if we start monitoring the first
  // event.
  base::MessageLoopCurrent::Get()->AddDestructionObserver(this);

  // Fetch pending events if any.
  OnXEvent();
}

void UserInputMonitorLinuxCore::StartMonitorWithMapping(
    base::WritableSharedMemoryMapping mapping) {
  StartMonitor();
  key_press_count_mapping_ =
      std::make_unique<base::WritableSharedMemoryMapping>(std::move(mapping));
}

void UserInputMonitorLinuxCore::StopMonitor() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (x_record_range_) {
    XFree(x_record_range_);
    x_record_range_ = NULL;
  }

  // Context must be disabled via the control channel because we can't send
  // any X protocol traffic over the data channel while it's recording.
  if (x_record_context_) {
    XRecordDisableContext(x_control_display_, x_record_context_);
    XFlush(x_control_display_);
    XRecordFreeContext(x_record_display_, x_record_context_);
    x_record_context_ = 0;

    watch_controller_.reset();
  }
  if (x_record_display_) {
    XCloseDisplay(x_record_display_);
    x_record_display_ = NULL;
  }
  if (x_control_display_) {
    XCloseDisplay(x_control_display_);
    x_control_display_ = NULL;
  }

  key_press_count_mapping_.reset();

  // Stop observing message loop destruction if no event is being monitored.
  base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);
}

void UserInputMonitorLinuxCore::OnXEvent() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  XEvent event;
  // Fetch pending events if any.
  while (XPending(x_record_display_)) {
    XNextEvent(x_record_display_, &event);
  }
}

void UserInputMonitorLinuxCore::ProcessXEvent(xEvent* event) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(event->u.u.type == KeyRelease || event->u.u.type == KeyPress);

  ui::EventType type =
      (event->u.u.type == KeyPress) ? ui::ET_KEY_PRESSED : ui::ET_KEY_RELEASED;

  KeySym key_sym =
      XkbKeycodeToKeysym(x_control_display_, event->u.u.detail, 0, 0);
  ui::KeyboardCode key_code = ui::KeyboardCodeFromXKeysym(key_sym);
  counter_.OnKeyboardEvent(type, key_code);

  // Update count value in shared memory.
  if (key_press_count_mapping_)
    WriteKeyPressMonitorCount(*key_press_count_mapping_, GetKeyPressCount());
}

// static
void UserInputMonitorLinuxCore::ProcessReply(XPointer self,
                                             XRecordInterceptData* data) {
  if (data->category == XRecordFromServer) {
    xEvent* event = reinterpret_cast<xEvent*>(data->data);
    reinterpret_cast<UserInputMonitorLinuxCore*>(self)->ProcessXEvent(event);
  }
  XRecordFreeData(data);
}

//
// Implementation of UserInputMonitorLinux.
//

UserInputMonitorLinux::UserInputMonitorLinux(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : io_task_runner_(io_task_runner),
      core_(new UserInputMonitorLinuxCore(io_task_runner)) {}

UserInputMonitorLinux::~UserInputMonitorLinux() {
  if (!io_task_runner_->DeleteSoon(FROM_HERE, core_))
    delete core_;
}

uint32_t UserInputMonitorLinux::GetKeyPressCount() const {
  return core_->GetKeyPressCount();
}

void UserInputMonitorLinux::StartKeyboardMonitoring() {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UserInputMonitorLinuxCore::StartMonitor,
                                core_->AsWeakPtr()));
}

void UserInputMonitorLinux::StartKeyboardMonitoring(
    base::WritableSharedMemoryMapping mapping) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UserInputMonitorLinuxCore::StartMonitorWithMapping,
                     core_->AsWeakPtr(), std::move(mapping)));
}

void UserInputMonitorLinux::StopKeyboardMonitoring() {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UserInputMonitorLinuxCore::StopMonitor,
                                core_->AsWeakPtr()));
}

}  // namespace

std::unique_ptr<UserInputMonitor> UserInputMonitor::Create(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  return std::make_unique<UserInputMonitorLinux>(io_task_runner);
}

}  // namespace media
