// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include <stdint.h>

#include <memory>

#include "services/device/wake_lock/power_save_blocker/power_save_blocker.h"

// Xlib #defines Status, but we can't have that for some of our headers.
#ifdef Status
#undef Status
#endif

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/nix/xdg_util.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "ui/gfx/switches.h"
#include "ui/gfx/x/x11_types.h"

namespace {

enum DBusAPI {
  NO_API,           // Disable. No supported API available.
  GNOME_API,        // Use the GNOME API. (Supports more features.)
  FREEDESKTOP_API,  // Use the FreeDesktop API, for KDE4, KDE5, and XFCE.
};

// Inhibit flags defined in the org.gnome.SessionManager interface.
// Can be OR'd together and passed as argument to the Inhibit() method
// to specify which power management features we want to suspend.
enum GnomeAPIInhibitFlags {
  INHIBIT_LOGOUT = 1,
  INHIBIT_SWITCH_USER = 2,
  INHIBIT_SUSPEND_SESSION = 4,
  INHIBIT_MARK_SESSION_IDLE = 8
};

const char kGnomeAPIServiceName[] = "org.gnome.SessionManager";
const char kGnomeAPIInterfaceName[] = "org.gnome.SessionManager";
const char kGnomeAPIObjectPath[] = "/org/gnome/SessionManager";

const char kFreeDesktopAPIPowerServiceName[] =
    "org.freedesktop.PowerManagement";
const char kFreeDesktopAPIPowerInterfaceName[] =
    "org.freedesktop.PowerManagement.Inhibit";
const char kFreeDesktopAPIPowerObjectPath[] =
    "/org/freedesktop/PowerManagement/Inhibit";

const char kFreeDesktopAPIScreenServiceName[] = "org.freedesktop.ScreenSaver";
const char kFreeDesktopAPIScreenInterfaceName[] = "org.freedesktop.ScreenSaver";
const char kFreeDesktopAPIScreenObjectPath[] = "/org/freedesktop/ScreenSaver";

}  // namespace

namespace device {

class PowerSaveBlocker::Delegate
    : public base::RefCountedThreadSafe<PowerSaveBlocker::Delegate> {
 public:
  // Picks an appropriate D-Bus API to use based on the desktop environment.
  Delegate(mojom::WakeLockType type,
           const std::string& description,
           bool freedesktop_only,
           scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
           scoped_refptr<base::SingleThreadTaskRunner> blocking_task_runner);

  // Post a task to initialize the delegate on the UI thread, which will itself
  // then post a task to apply the power save block on the FILE thread.
  void Init();

  // Post a task to remove the power save block on the FILE thread, unless it
  // hasn't yet been applied, in which case we just prevent it from applying.
  void CleanUp();

 private:
  friend class base::RefCountedThreadSafe<Delegate>;
  ~Delegate() {}

  // Selects an appropriate D-Bus API to use for this object. Must be called on
  // the UI thread. Checks enqueue_apply_ once an API has been selected, and
  // enqueues a call back to ApplyBlock() if it is true. See the comments for
  // enqueue_apply_ below.
  void InitOnUIThread();

  // Returns true if ApplyBlock() / RemoveBlock() should be called.
  bool ShouldBlock() const;

  // Apply or remove the power save block, respectively. These methods should be
  // called once each, on the same thread, per instance. They block waiting for
  // the action to complete (with a timeout); the thread must thus allow I/O.
  void ApplyBlock();
  void RemoveBlock();

  // Asynchronous callback functions for ApplyBlock and RemoveBlock.
  // Functions do not receive ownership of |response|.
  void ApplyBlockFinished(dbus::Response* response);
  void RemoveBlockFinished(dbus::Response* response);

  // Wrapper for XScreenSaverSuspend. Checks whether the X11 Screen Saver
  // Extension is available first. If it isn't, this is a no-op.
  // Must be called on the UI thread.
  void XSSSuspendSet(bool suspend);

  // If no other method is available (i.e. not running under a Desktop
  // Environment) check whether the X11 Screen Saver Extension can be used
  // to disable the screen saver. Must be called on the UI thread.
  bool XSSAvailable();

  // Returns an appropriate D-Bus API to use based on the desktop environment.
  DBusAPI SelectAPI();

  const mojom::WakeLockType type_;
  const std::string description_;
  const bool freedesktop_only_;

  // Initially, we post a message to the UI thread to select an API. When it
  // finishes, it will post a message to the FILE thread to perform the actual
  // application of the block, unless enqueue_apply_ is false. We set it to
  // false when we post that message, or when RemoveBlock() is called before
  // ApplyBlock() has run. Both api_ and enqueue_apply_ are guarded by lock_.
  DBusAPI api_;
  bool enqueue_apply_;
  base::Lock lock_;

  // Indicates that a D-Bus power save blocking request is in flight.
  bool block_inflight_;
  // Used to detect erronous redundant calls to RemoveBlock().
  bool unblock_inflight_;
  // Indicates that RemoveBlock() is called before ApplyBlock() has finished.
  // If it's true, then the RemoveBlock() call will be processed immediately
  // after ApplyBlock() has finished.
  bool enqueue_unblock_;

  scoped_refptr<dbus::Bus> bus_;

  // The cookie that identifies our inhibit request,
  // or 0 if there is no active inhibit request.
  uint32_t inhibit_cookie_;

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> blocking_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

PowerSaveBlocker::Delegate::Delegate(
    mojom::WakeLockType type,
    const std::string& description,
    bool freedesktop_only,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> blocking_task_runner)
    : type_(type),
      description_(description),
      freedesktop_only_(freedesktop_only),
      api_(NO_API),
      enqueue_apply_(false),
      inhibit_cookie_(0),
      ui_task_runner_(ui_task_runner),
      blocking_task_runner_(blocking_task_runner) {
  // We're on the client's thread here, so we don't allocate the dbus::Bus
  // object yet. We'll do it later in ApplyBlock(), on the FILE thread.
}

void PowerSaveBlocker::Delegate::Init() {
  base::AutoLock lock(lock_);
  DCHECK(!enqueue_apply_);
  enqueue_apply_ = true;
  block_inflight_ = false;
  unblock_inflight_ = false;
  enqueue_unblock_ = false;
  ui_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&Delegate::InitOnUIThread, this));
}

void PowerSaveBlocker::Delegate::CleanUp() {
  base::AutoLock lock(lock_);
  if (enqueue_apply_) {
    // If a call to ApplyBlock() has not yet been enqueued because we are still
    // initializing on the UI thread, then just cancel it. We don't need to
    // remove the block because we haven't even applied it yet.
    enqueue_apply_ = false;
  } else {
    if (ShouldBlock()) {
      blocking_task_runner_->PostTask(FROM_HERE,
                                      base::Bind(&Delegate::RemoveBlock, this));
    }

    ui_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Delegate::XSSSuspendSet, this, false));
  }
}

void PowerSaveBlocker::Delegate::InitOnUIThread() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  base::AutoLock lock(lock_);
  api_ = SelectAPI();

  if (enqueue_apply_) {
    if (ShouldBlock()) {
      // The thread we use here becomes the origin and D-Bus thread for the
      // D-Bus library, so we need to use the same thread above for
      // RemoveBlock(). It must be a thread that allows I/O operations, so we
      // use the FILE thread.
      blocking_task_runner_->PostTask(FROM_HERE,
                                      base::Bind(&Delegate::ApplyBlock, this));
    }
    XSSSuspendSet(true);
  }
  enqueue_apply_ = false;
}

bool PowerSaveBlocker::Delegate::ShouldBlock() const {
  return freedesktop_only_ ? api_ == FREEDESKTOP_API : api_ != NO_API;
}

void PowerSaveBlocker::Delegate::ApplyBlock() {
  DCHECK(blocking_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(!bus_);  // ApplyBlock() should only be called once.
  DCHECK(!block_inflight_);

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SESSION;
  options.connection_type = dbus::Bus::PRIVATE;
  bus_ = new dbus::Bus(options);

  scoped_refptr<dbus::ObjectProxy> object_proxy;
  std::unique_ptr<dbus::MethodCall> method_call;
  std::unique_ptr<dbus::MessageWriter> message_writer;

  switch (api_) {
    case NO_API:
      NOTREACHED();  // We should never call this method with this value.
      return;
    case GNOME_API:
      object_proxy = bus_->GetObjectProxy(
          kGnomeAPIServiceName, dbus::ObjectPath(kGnomeAPIObjectPath));
      method_call.reset(
          new dbus::MethodCall(kGnomeAPIInterfaceName, "Inhibit"));
      message_writer.reset(new dbus::MessageWriter(method_call.get()));
      // The arguments of the method are:
      //     app_id:        The application identifier
      //     toplevel_xid:  The toplevel X window identifier
      //     reason:        The reason for the inhibit
      //     flags:         Flags that spefify what should be inhibited
      message_writer->AppendString(
          base::CommandLine::ForCurrentProcess()->GetProgram().value());
      message_writer->AppendUint32(0);  // should be toplevel_xid
      message_writer->AppendString(description_);
      {
        uint32_t flags = 0;
        switch (type_) {
          case mojom::WakeLockType::kPreventDisplaySleep:
          case mojom::WakeLockType::kPreventDisplaySleepAllowDimming:
            flags |= INHIBIT_MARK_SESSION_IDLE;
            flags |= INHIBIT_SUSPEND_SESSION;
            break;
          case mojom::WakeLockType::kPreventAppSuspension:
            flags |= INHIBIT_SUSPEND_SESSION;
            break;
        }
        message_writer->AppendUint32(flags);
      }
      break;
    case FREEDESKTOP_API:
      switch (type_) {
        case mojom::WakeLockType::kPreventDisplaySleep:
        case mojom::WakeLockType::kPreventDisplaySleepAllowDimming:
          object_proxy = bus_->GetObjectProxy(
              kFreeDesktopAPIScreenServiceName,
              dbus::ObjectPath(kFreeDesktopAPIScreenObjectPath));
          method_call.reset(new dbus::MethodCall(
              kFreeDesktopAPIScreenInterfaceName, "Inhibit"));
          break;
        case mojom::WakeLockType::kPreventAppSuspension:
          object_proxy = bus_->GetObjectProxy(
              kFreeDesktopAPIPowerServiceName,
              dbus::ObjectPath(kFreeDesktopAPIPowerObjectPath));
          method_call.reset(new dbus::MethodCall(
              kFreeDesktopAPIPowerInterfaceName, "Inhibit"));
          break;
      }
      message_writer.reset(new dbus::MessageWriter(method_call.get()));
      // The arguments of the method are:
      //     app_id:        The application identifier
      //     reason:        The reason for the inhibit
      message_writer->AppendString(
          base::CommandLine::ForCurrentProcess()->GetProgram().value());
      message_writer->AppendString(description_);
      break;
  }

  block_inflight_ = true;
  object_proxy->CallMethod(
      method_call.get(), dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::Bind(&PowerSaveBlocker::Delegate::ApplyBlockFinished, this));
}

void PowerSaveBlocker::Delegate::ApplyBlockFinished(dbus::Response* response) {
  DCHECK(blocking_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(bus_);
  DCHECK(block_inflight_);
  block_inflight_ = false;

  if (response) {
    // The method returns an inhibit_cookie, used to uniquely identify
    // this request. It should be used as an argument to Uninhibit()
    // in order to remove the request.
    dbus::MessageReader message_reader(response);
    if (!message_reader.PopUint32(&inhibit_cookie_))
      LOG(ERROR) << "Invalid Inhibit() response: " << response->ToString();
  } else {
    LOG(ERROR) << "No response to Inhibit() request!";
  }

  if (enqueue_unblock_) {
    enqueue_unblock_ = false;
    // RemoveBlock() was called while the Inhibit operation was in flight,
    // so go ahead and remove the block now.
    blocking_task_runner_->PostTask(FROM_HERE,
                                    base::Bind(&Delegate::RemoveBlock, this));
  }
}

void PowerSaveBlocker::Delegate::RemoveBlock() {
  DCHECK(blocking_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(bus_);  // RemoveBlock() should only be called once.
  DCHECK(!unblock_inflight_);

  if (block_inflight_) {
    DCHECK(!enqueue_unblock_);
    // Can't call RemoveBlock until ApplyBlock's async operation has
    // finished. Enqueue it for execution once ApplyBlock is done.
    enqueue_unblock_ = true;
    return;
  }

  scoped_refptr<dbus::ObjectProxy> object_proxy;
  std::unique_ptr<dbus::MethodCall> method_call;

  switch (api_) {
    case NO_API:
      NOTREACHED();  // We should never call this method with this value.
      return;
    case GNOME_API:
      object_proxy = bus_->GetObjectProxy(
          kGnomeAPIServiceName, dbus::ObjectPath(kGnomeAPIObjectPath));
      method_call.reset(
          new dbus::MethodCall(kGnomeAPIInterfaceName, "Uninhibit"));
      break;
    case FREEDESKTOP_API:
      switch (type_) {
        case mojom::WakeLockType::kPreventDisplaySleep:
        case mojom::WakeLockType::kPreventDisplaySleepAllowDimming:
          object_proxy = bus_->GetObjectProxy(
              kFreeDesktopAPIScreenServiceName,
              dbus::ObjectPath(kFreeDesktopAPIScreenObjectPath));
          method_call.reset(new dbus::MethodCall(
              kFreeDesktopAPIScreenInterfaceName, "UnInhibit"));
          break;
        case mojom::WakeLockType::kPreventAppSuspension:
          object_proxy = bus_->GetObjectProxy(
              kFreeDesktopAPIPowerServiceName,
              dbus::ObjectPath(kFreeDesktopAPIPowerObjectPath));
          method_call.reset(new dbus::MethodCall(
              kFreeDesktopAPIPowerInterfaceName, "UnInhibit"));
          break;
      }
      break;
  }

  dbus::MessageWriter message_writer(method_call.get());
  message_writer.AppendUint32(inhibit_cookie_);
  unblock_inflight_ = true;
  object_proxy->CallMethod(
      method_call.get(), dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::Bind(&PowerSaveBlocker::Delegate::RemoveBlockFinished, this));
}

void PowerSaveBlocker::Delegate::RemoveBlockFinished(dbus::Response* response) {
  DCHECK(blocking_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(bus_);
  unblock_inflight_ = false;

  if (!response)
    LOG(ERROR) << "No response to Uninhibit() request!";
  // We don't care about checking the result. We assume it works; we can't
  // really do anything about it anyway if it fails.
  inhibit_cookie_ = 0;

  bus_->ShutdownAndBlock();
  bus_ = nullptr;
}

void PowerSaveBlocker::Delegate::XSSSuspendSet(bool suspend) {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());

  if (!XSSAvailable())
    return;

  XDisplay* display = gfx::GetXDisplay();
  XScreenSaverSuspend(display, suspend);
}

bool PowerSaveBlocker::Delegate::XSSAvailable() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  // X Screen Saver isn't accessible in headless mode.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kHeadless))
    return false;
  XDisplay* display = gfx::GetXDisplay();
  int dummy;
  int major;
  int minor;

  if (!XScreenSaverQueryExtension(display, &dummy, &dummy))
    return false;

  if (!XScreenSaverQueryVersion(display, &major, &minor))
    return false;

  return major > 1 || (major == 1 && minor >= 1);
}

DBusAPI PowerSaveBlocker::Delegate::SelectAPI() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  // Power saving APIs are not accessible in headless mode.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kHeadless))
    return NO_API;

  // TODO(thomasanderson): Query the GNOME and Freedesktop APIs for availability
  // directly rather than guessing based on the desktop environment.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  switch (base::nix::GetDesktopEnvironment(env.get())) {
    case base::nix::DESKTOP_ENVIRONMENT_CINNAMON:
    case base::nix::DESKTOP_ENVIRONMENT_GNOME:
    case base::nix::DESKTOP_ENVIRONMENT_PANTHEON:
    case base::nix::DESKTOP_ENVIRONMENT_UNITY:
      return GNOME_API;
    case base::nix::DESKTOP_ENVIRONMENT_XFCE:
    case base::nix::DESKTOP_ENVIRONMENT_KDE4:
    case base::nix::DESKTOP_ENVIRONMENT_KDE5:
      return FREEDESKTOP_API;
    case base::nix::DESKTOP_ENVIRONMENT_KDE3:
    case base::nix::DESKTOP_ENVIRONMENT_OTHER:
      // Not supported.
      break;
  }
  return NO_API;
}

PowerSaveBlocker::PowerSaveBlocker(
    mojom::WakeLockType type,
    mojom::WakeLockReason reason,
    const std::string& description,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> blocking_task_runner)
    : delegate_(new Delegate(type,
                             description,
                             false /* freedesktop_only */,
                             ui_task_runner,
                             blocking_task_runner)),
      ui_task_runner_(ui_task_runner),
      blocking_task_runner_(blocking_task_runner) {
  delegate_->Init();

  if (type == mojom::WakeLockType::kPreventDisplaySleep ||
      type == mojom::WakeLockType::kPreventDisplaySleepAllowDimming) {
    freedesktop_suspend_delegate_ = new Delegate(
        mojom::WakeLockType::kPreventAppSuspension, description,
        true /* freedesktop_only */, ui_task_runner, blocking_task_runner);
    freedesktop_suspend_delegate_->Init();
  }
}

PowerSaveBlocker::~PowerSaveBlocker() {
  delegate_->CleanUp();
  if (freedesktop_suspend_delegate_)
    freedesktop_suspend_delegate_->CleanUp();
}

}  // namespace device
