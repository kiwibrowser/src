// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_watchdog_thread.h"

#include <errno.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/alias.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/power_monitor/power_monitor.h"
#include "base/process/process.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(USE_X11)
#include "ui/gfx/x/x11.h"
#endif

namespace gpu {
namespace {

#if defined(CYGPROFILE_INSTRUMENTATION)
const int kGpuTimeout = 30000;
#elif defined(OS_WIN)
// Use a slightly longer timeout on Windows due to prevalence of slow and
// infected machines.
const int kGpuTimeout = 15000;
#else
const int kGpuTimeout = 10000;
#endif

#if defined(USE_X11)
const base::FilePath::CharType kTtyFilePath[] =
    FILE_PATH_LITERAL("/sys/class/tty/tty0/active");
const unsigned char text[20] = "check";
#endif

}  // namespace

GpuWatchdogThread::GpuWatchdogThread()
    : base::Thread("Watchdog"),
      watched_message_loop_(base::MessageLoop::current()),
      timeout_(base::TimeDelta::FromMilliseconds(kGpuTimeout)),
      armed_(false),
      task_observer_(this),
      use_thread_cpu_time_(true),
      responsive_acknowledge_count_(0),
#if defined(OS_WIN)
      watched_thread_handle_(0),
      arm_cpu_time_(),
#endif
      suspension_counter_(this),
#if defined(USE_X11)
      display_(NULL),
      window_(0),
      atom_(x11::None),
      host_tty_(-1),
#endif
      weak_factory_(this) {
  base::subtle::NoBarrier_Store(&awaiting_acknowledge_, false);

#if defined(OS_WIN)
  // GetCurrentThread returns a pseudo-handle that cannot be used by one thread
  // to identify another. DuplicateHandle creates a "real" handle that can be
  // used for this purpose.
  BOOL result = DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                                GetCurrentProcess(), &watched_thread_handle_,
                                THREAD_QUERY_INFORMATION, FALSE, 0);
  DCHECK(result);
#endif

#if defined(USE_X11)
  tty_file_ = base::OpenFile(base::FilePath(kTtyFilePath), "r");
  SetupXServer();
#endif
  watched_message_loop_->AddTaskObserver(&task_observer_);
}

// static
std::unique_ptr<GpuWatchdogThread> GpuWatchdogThread::Create() {
  auto watchdog_thread = base::WrapUnique(new GpuWatchdogThread);
  base::Thread::Options options;
  options.timer_slack = base::TIMER_SLACK_MAXIMUM;
  watchdog_thread->StartWithOptions(options);
  return watchdog_thread;
}

void GpuWatchdogThread::CheckArmed() {
  // If the watchdog is |awaiting_acknowledge_|, reset this variable to false
  // and post an acknowledge task now. No barrier is needed as
  // |awaiting_acknowledge_| is only ever read from this thread.
  if (base::subtle::NoBarrier_CompareAndSwap(&awaiting_acknowledge_, true,
                                             false)) {
    // Called on the monitored thread. Responds with OnAcknowledge. Cannot use
    // the method factory. As we stop the task runner before destroying this
    // class, the unretained reference will always outlive the task.
    task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&GpuWatchdogThread::OnAcknowledge, base::Unretained(this)));
  }
}

void GpuWatchdogThread::ReportProgress() {
  CheckArmed();
}

void GpuWatchdogThread::OnBackgrounded() {
  // As we stop the task runner before destroying this class, the unretained
  // reference will always outlive the task.
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuWatchdogThread::OnBackgroundedOnWatchdogThread,
                     base::Unretained(this)));
}

void GpuWatchdogThread::OnForegrounded() {
  // As we stop the task runner before destroying this class, the unretained
  // reference will always outlive the task.
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuWatchdogThread::OnForegroundedOnWatchdogThread,
                     base::Unretained(this)));
}

void GpuWatchdogThread::Init() {
  // Schedule the first check.
  OnCheck(false);
}

void GpuWatchdogThread::CleanUp() {
  weak_factory_.InvalidateWeakPtrs();
}

GpuWatchdogThread::GpuWatchdogTaskObserver::GpuWatchdogTaskObserver(
    GpuWatchdogThread* watchdog)
    : watchdog_(watchdog) {}

GpuWatchdogThread::GpuWatchdogTaskObserver::~GpuWatchdogTaskObserver() =
    default;

void GpuWatchdogThread::GpuWatchdogTaskObserver::WillProcessTask(
    const base::PendingTask& pending_task) {
  watchdog_->CheckArmed();
}

void GpuWatchdogThread::GpuWatchdogTaskObserver::DidProcessTask(
    const base::PendingTask& pending_task) {}

GpuWatchdogThread::SuspensionCounter::SuspensionCounterRef::
    SuspensionCounterRef(SuspensionCounter* counter)
    : counter_(counter) {
  counter_->OnAddRef();
}

GpuWatchdogThread::SuspensionCounter::SuspensionCounterRef::
    ~SuspensionCounterRef() {
  counter_->OnReleaseRef();
}

GpuWatchdogThread::SuspensionCounter::SuspensionCounter(
    GpuWatchdogThread* watchdog_thread)
    : watchdog_thread_(watchdog_thread) {
  // This class will only be used on the watchdog thread, but is constructed on
  // the main thread. Detach.
  DETACH_FROM_SEQUENCE(watchdog_thread_sequence_checker_);
}

std::unique_ptr<GpuWatchdogThread::SuspensionCounter::SuspensionCounterRef>
GpuWatchdogThread::SuspensionCounter::Take() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watchdog_thread_sequence_checker_);
  return std::make_unique<SuspensionCounterRef>(this);
}

bool GpuWatchdogThread::SuspensionCounter::HasRefs() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watchdog_thread_sequence_checker_);
  return suspend_count_ > 0;
}

void GpuWatchdogThread::SuspensionCounter::OnWatchdogThreadStopped() {
  DETACH_FROM_SEQUENCE(watchdog_thread_sequence_checker_);

  // Null the |watchdog_thread_| ptr at shutdown to avoid trying to suspend or
  // resume after the thread is stopped.
  watchdog_thread_ = nullptr;
}

void GpuWatchdogThread::SuspensionCounter::OnAddRef() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watchdog_thread_sequence_checker_);
  suspend_count_++;
  if (watchdog_thread_ && suspend_count_ == 1)
    watchdog_thread_->SuspendStateChanged();
}

void GpuWatchdogThread::SuspensionCounter::OnReleaseRef() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watchdog_thread_sequence_checker_);
  DCHECK_GT(suspend_count_, 0u);
  suspend_count_--;
  if (watchdog_thread_ && suspend_count_ == 0)
    watchdog_thread_->SuspendStateChanged();
}

GpuWatchdogThread::~GpuWatchdogThread() {
  Stop();
  suspension_counter_.OnWatchdogThreadStopped();

#if defined(OS_WIN)
  CloseHandle(watched_thread_handle_);
#endif

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->RemoveObserver(this);

#if defined(USE_X11)
  if (tty_file_)
    fclose(tty_file_);
  if (display_) {
    DCHECK(window_);
    XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
  }
#endif

  watched_message_loop_->RemoveTaskObserver(&task_observer_);
}

void GpuWatchdogThread::OnAcknowledge() {
  CHECK(base::PlatformThread::CurrentId() == GetThreadId());

  // The check has already been acknowledged and another has already been
  // scheduled by a previous call to OnAcknowledge. It is normal for a
  // watched thread to see armed_ being true multiple times before
  // the OnAcknowledge task is run on the watchdog thread.
  if (!armed_)
    return;

  // Revoke any pending hang termination.
  weak_factory_.InvalidateWeakPtrs();
  armed_ = false;

  if (suspension_counter_.HasRefs()) {
    responsive_acknowledge_count_ = 0;
    return;
  }

  base::Time current_time = base::Time::Now();

  // The watchdog waits until at least 6 consecutive checks have returned in
  // less than 50 ms before it will start ignoring the CPU time in determining
  // whether to timeout. This is a compromise to allow startups that are slow
  // due to disk contention to avoid timing out, but once the GPU process is
  // running smoothly the watchdog will be able to detect hangs that don't use
  // the CPU.
  if ((current_time - check_time_) < base::TimeDelta::FromMilliseconds(50))
    responsive_acknowledge_count_++;
  else
    responsive_acknowledge_count_ = 0;

  if (responsive_acknowledge_count_ >= 6)
    use_thread_cpu_time_ = false;

  // If it took a long time for the acknowledgement, assume the computer was
  // recently suspended.
  bool was_suspended = (current_time > suspension_timeout_);

  // The monitored thread has responded. Post a task to check it again.
  task_runner()->PostDelayedTask(
      FROM_HERE, base::Bind(&GpuWatchdogThread::OnCheck,
                            weak_factory_.GetWeakPtr(), was_suspended),
      0.5 * timeout_);
}

void GpuWatchdogThread::OnCheck(bool after_suspend) {
  CHECK(base::PlatformThread::CurrentId() == GetThreadId());

  // Do not create any new termination tasks if one has already been created
  // or the system is suspended.
  if (armed_ || suspension_counter_.HasRefs())
    return;

  armed_ = true;

  // Must set |awaiting_acknowledge_| before posting the task.  This task might
  // be the only task that will activate the TaskObserver on the watched thread
  // and it must not miss the false -> true transition. No barrier is needed
  // here, as the PostTask which follows contains a barrier.
  base::subtle::NoBarrier_Store(&awaiting_acknowledge_, true);

#if defined(OS_WIN)
  arm_cpu_time_ = GetWatchedThreadTime();

  QueryUnbiasedInterruptTime(&arm_interrupt_time_);
#endif

  check_time_ = base::Time::Now();
  check_timeticks_ = base::TimeTicks::Now();
  // Immediately after the computer is woken up from being suspended it might
  // be pretty sluggish, so allow some extra time before the next timeout.
  base::TimeDelta timeout = timeout_ * (after_suspend ? 3 : 1);
  suspension_timeout_ = check_time_ + timeout * 2;

  // Post a task to the monitored thread that does nothing but wake up the
  // TaskObserver. Any other tasks that are pending on the watched thread will
  // also wake up the observer. This simply ensures there is at least one.
  watched_message_loop_->task_runner()->PostTask(FROM_HERE, base::DoNothing());

  // Post a task to the watchdog thread to exit if the monitored thread does
  // not respond in time.
  task_runner()->PostDelayedTask(FROM_HERE,
                                 base::Bind(&GpuWatchdogThread::OnCheckTimeout,
                                            weak_factory_.GetWeakPtr()),
                                 timeout);
}

void GpuWatchdogThread::OnCheckTimeout() {
  // Should not get here while the system is suspended.
  DCHECK(!suspension_counter_.HasRefs());

  // If the watchdog woke up significantly behind schedule, disarm and reset
  // the watchdog check. This is to prevent the watchdog thread from terminating
  // when a machine wakes up from sleep or hibernation, which would otherwise
  // appear to be a hang.
  if (base::Time::Now() > suspension_timeout_) {
    armed_ = false;
    OnCheck(true);
    return;
  }

  if (!base::subtle::NoBarrier_Load(&awaiting_acknowledge_)) {
    // This should be possible only when CheckArmed() has been called but
    // OnAcknowledge() hasn't.
    // In this case the watched thread might need more time to finish posting
    // OnAcknowledge task.

    // Continue with the termination after an additional delay.
    task_runner()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&GpuWatchdogThread::DeliberatelyTerminateToRecoverFromHang,
                   weak_factory_.GetWeakPtr()),
        0.5 * timeout_);

    // Post a task that does nothing on the watched thread to bump its priority
    // and make it more likely to get scheduled.
    watched_message_loop_->task_runner()->PostTask(FROM_HERE,
                                                   base::DoNothing());
    return;
  }

  DeliberatelyTerminateToRecoverFromHang();
}

// Use the --disable-gpu-watchdog command line switch to disable this.
void GpuWatchdogThread::DeliberatelyTerminateToRecoverFromHang() {
  // Should not get here while the system is suspended.
  DCHECK(!suspension_counter_.HasRefs());

  if (alternative_terminate_for_testing_) {
    alternative_terminate_for_testing_.Run();
    return;
  }

#if defined(OS_WIN)
  // Defer termination until a certain amount of CPU time has elapsed on the
  // watched thread.
  base::ThreadTicks current_cpu_time = GetWatchedThreadTime();
  base::TimeDelta time_since_arm = current_cpu_time - arm_cpu_time_;
  if (use_thread_cpu_time_ && (time_since_arm < timeout_)) {
    task_runner()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&GpuWatchdogThread::DeliberatelyTerminateToRecoverFromHang,
                   weak_factory_.GetWeakPtr()),
        timeout_ - time_since_arm);
    return;
  }
#endif

#if defined(USE_X11)
  if (display_) {
    DCHECK(window_);
    XWindowAttributes attributes;
    XGetWindowAttributes(display_, window_, &attributes);

    XSelectInput(display_, window_, PropertyChangeMask);
    SetupXChangeProp();

    XFlush(display_);

    // We wait for the property change event with a timeout. If it arrives we
    // know that X is responsive and is not the cause of the watchdog trigger,
    // so we should terminate. If it times out, it may be due to X taking a long
    // time, but terminating won't help, so ignore the watchdog trigger.
    XEvent event_return;
    base::TimeTicks deadline = base::TimeTicks::Now() + timeout_;
    while (true) {
      base::TimeDelta delta = deadline - base::TimeTicks::Now();
      if (delta < base::TimeDelta()) {
        return;
      } else {
        while (XCheckWindowEvent(display_, window_, PropertyChangeMask,
                                 &event_return)) {
          if (MatchXEventAtom(&event_return))
            break;
        }
        struct pollfd fds[1];
        fds[0].fd = XConnectionNumber(display_);
        fds[0].events = POLLIN;
        int status = poll(fds, 1, delta.InMilliseconds());
        if (status == -1) {
          if (errno == EINTR) {
            continue;
          } else {
            LOG(FATAL) << "Lost X connection, aborting.";
            break;
          }
        } else if (status == 0) {
          return;
        } else {
          continue;
        }
      }
    }
  }
#endif

  // For minimal developer annoyance, don't keep terminating. You need to skip
  // the call to base::Process::Terminate below in a debugger for this to be
  // useful.
  static bool terminated = false;
  if (terminated)
    return;

#if defined(OS_WIN)
  if (IsDebuggerPresent())
    return;
#endif

#if defined(USE_X11)
  // Don't crash if we're not on the TTY of our host X11 server.
  int active_tty = GetActiveTTY();
  if (host_tty_ != -1 && active_tty != -1 && host_tty_ != active_tty) {
    return;
  }
#endif

// Store variables so they're available in crash dumps to help determine the
// cause of any hang.
#if defined(OS_WIN)
  ULONGLONG fire_interrupt_time;
  QueryUnbiasedInterruptTime(&fire_interrupt_time);

  // This is the time since the watchdog was armed, in 100ns intervals,
  // ignoring time where the computer is suspended.
  ULONGLONG interrupt_delay = fire_interrupt_time - arm_interrupt_time_;

  base::debug::Alias(&interrupt_delay);
  base::debug::Alias(&current_cpu_time);
  base::debug::Alias(&time_since_arm);

  bool using_thread_ticks = base::ThreadTicks::IsSupported();
  base::debug::Alias(&using_thread_ticks);

  bool using_high_res_timer = base::TimeTicks::IsHighResolution();
  base::debug::Alias(&using_high_res_timer);
#endif

  base::Time current_time = base::Time::Now();
  base::TimeTicks current_timeticks = base::TimeTicks::Now();
  base::debug::Alias(&current_time);
  base::debug::Alias(&current_timeticks);

  int32_t awaiting_acknowledge =
      base::subtle::NoBarrier_Load(&awaiting_acknowledge_);
  base::debug::Alias(&awaiting_acknowledge);

  // Don't log the message to stderr in release builds because the buffer
  // may be full.
  std::string message = base::StringPrintf(
      "The GPU process hung. Terminating after %" PRId64 " ms.",
      timeout_.InMilliseconds());
  logging::LogMessageHandlerFunction handler = logging::GetLogMessageHandler();
  if (handler)
    handler(logging::LOG_ERROR, __FILE__, __LINE__, 0, message);
  DLOG(ERROR) << message;

  // Deliberately crash the process to create a crash dump.
  *((volatile int*)0) = 0x1337;

  terminated = true;
}

#if defined(USE_X11)
void GpuWatchdogThread::SetupXServer() {
  display_ = XOpenDisplay(NULL);
  if (display_) {
    window_ =
        XCreateWindow(display_, DefaultRootWindow(display_), 0, 0, 1, 1, 0,
                      CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
    atom_ = XInternAtom(display_, "CHECK", x11::False);
  }
  host_tty_ = GetActiveTTY();
}

void GpuWatchdogThread::SetupXChangeProp() {
  DCHECK(display_);
  XChangeProperty(display_, window_, atom_, XA_STRING, 8, PropModeReplace, text,
                  (arraysize(text) - 1));
}

bool GpuWatchdogThread::MatchXEventAtom(XEvent* event) {
  if (event->xproperty.window == window_ && event->type == PropertyNotify &&
      event->xproperty.atom == atom_)
    return true;

  return false;
}

#endif
void GpuWatchdogThread::AddPowerObserver() {
  // As we stop the task runner before destroying this class, the unretained
  // reference will always outlive the task.
  task_runner()->PostTask(FROM_HERE,
                          base::Bind(&GpuWatchdogThread::OnAddPowerObserver,
                                     base::Unretained(this)));
}

void GpuWatchdogThread::OnAddPowerObserver() {
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  DCHECK(power_monitor);
  power_monitor->AddObserver(this);
}

void GpuWatchdogThread::OnSuspend() {
  power_suspend_ref_ = suspension_counter_.Take();
}

void GpuWatchdogThread::OnResume() {
  power_suspend_ref_.reset();
}

void GpuWatchdogThread::OnBackgroundedOnWatchdogThread() {
  background_suspend_ref_ = suspension_counter_.Take();
}

void GpuWatchdogThread::OnForegroundedOnWatchdogThread() {
  background_suspend_ref_.reset();
}

void GpuWatchdogThread::SuspendStateChanged() {
  if (suspension_counter_.HasRefs()) {
    suspend_time_ = base::Time::Now();
    // When suspending force an acknowledgement to cancel any pending
    // termination tasks.
    OnAcknowledge();
  } else {
    resume_time_ = base::Time::Now();

    // After resuming jump-start the watchdog again.
    armed_ = false;
    OnCheck(true);
  }
}

#if defined(OS_WIN)
base::ThreadTicks GpuWatchdogThread::GetWatchedThreadTime() {
  if (base::ThreadTicks::IsSupported()) {
    // Convert ThreadTicks::Now() to TimeDelta.
    return base::ThreadTicks::GetForThread(
        base::PlatformThreadHandle(watched_thread_handle_));
  } else {
    // Use GetThreadTimes as a backup mechanism.
    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME user_time;
    FILETIME kernel_time;
    BOOL result = GetThreadTimes(watched_thread_handle_, &creation_time,
                                 &exit_time, &kernel_time, &user_time);
    DCHECK(result);

    ULARGE_INTEGER user_time64;
    user_time64.HighPart = user_time.dwHighDateTime;
    user_time64.LowPart = user_time.dwLowDateTime;

    ULARGE_INTEGER kernel_time64;
    kernel_time64.HighPart = kernel_time.dwHighDateTime;
    kernel_time64.LowPart = kernel_time.dwLowDateTime;

    // Time is reported in units of 100 nanoseconds. Kernel and user time are
    // summed to deal with to kinds of hangs. One is where the GPU process is
    // stuck in user level, never calling into the kernel and kernel time is
    // not increasing. The other is where either the kernel hangs and never
    // returns to user level or where user level code
    // calls into kernel level repeatedly, giving up its quanta before it is
    // tracked, for example a loop that repeatedly Sleeps.
    return base::ThreadTicks() +
           base::TimeDelta::FromMilliseconds(static_cast<int64_t>(
               (user_time64.QuadPart + kernel_time64.QuadPart) / 10000));
  }
}
#endif

#if defined(USE_X11)
int GpuWatchdogThread::GetActiveTTY() const {
  char tty_string[8] = {0};
  if (tty_file_ && !fseek(tty_file_, 0, SEEK_SET) &&
      fread(tty_string, 1, 7, tty_file_)) {
    int tty_number;
    size_t num_res = sscanf(tty_string, "tty%d\n", &tty_number);
    if (num_res == 1)
      return tty_number;
  }
  return -1;
}
#endif

void GpuWatchdogThread::SetAlternativeTerminateFunctionForTesting(
    base::RepeatingClosure on_terminate) {
  alternative_terminate_for_testing_ = std::move(on_terminate);
}

void GpuWatchdogThread::SetTimeoutForTesting(base::TimeDelta timeout) {
  timeout_ = timeout;
}

}  // namespace gpu
