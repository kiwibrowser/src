// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_vsync_provider_win.h"

#include <dwmapi.h>
#include <windows.h>

#include <string>

#include "base/atomicops.h"
#include "base/debug/alias.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "base/trace_event/trace_event.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/message_filter.h"

namespace gpu {

namespace {
// Default v-sync interval used when there is no history of v-sync timestamps.
const int kDefaultInterval = 16666;

// Occasionally DWM stops advancing qpcVBlank timestamp. The existing
// implementation can cope with that by adjusting the qpcVBlank value forward
// by a number of v-sync intervals, although the accuracy of adjustment depends
// on accuracy of the calculated v-sync interval. To avoid accumulating the
// error, any DWM values that are more than the threshold number of intervals
// in the past are ignored.
const int kMissingDwmTimestampsThreshold = 7;

// from <D3dkmthk.h>
typedef LONG NTSTATUS;
typedef UINT D3DKMT_HANDLE;
typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_GRAPHICS_PRESENT_OCCLUDED ((NTSTATUS)0xC01E0006L)

typedef struct _D3DKMT_OPENADAPTERFROMHDC {
  HDC hDc;
  D3DKMT_HANDLE hAdapter;
  LUID AdapterLuid;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_CLOSEADAPTER {
  D3DKMT_HANDLE hAdapter;
} D3DKMT_CLOSEADAPTER;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
  D3DKMT_HANDLE hAdapter;
  D3DKMT_HANDLE hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef NTSTATUS(APIENTRY* PFND3DKMTOPENADAPTERFROMHDC)(
    D3DKMT_OPENADAPTERFROMHDC*);
typedef NTSTATUS(APIENTRY* PFND3DKMTCLOSEADAPTER)(D3DKMT_CLOSEADAPTER*);
typedef NTSTATUS(APIENTRY* PFND3DKMTWAITFORVERTICALBLANKEVENT)(
    D3DKMT_WAITFORVERTICALBLANKEVENT*);
}  // namespace

// The actual implementation of background tasks plus any state that might be
// needed on the worker thread.
class GpuVSyncWorker : public base::Thread,
                       public base::RefCountedThreadSafe<GpuVSyncWorker> {
 public:
  GpuVSyncWorker(const gfx::VSyncProvider::UpdateVSyncCallback& callback,
                 SurfaceHandle surface_handle);

  void CleanupAndStop();
  void Enable(bool enabled);
  void StartRunningVSyncOnThread();
  void WaitForVSyncOnThread();
  bool BelongsToWorkerThread();

 private:
  friend class base::RefCountedThreadSafe<GpuVSyncWorker>;
  ~GpuVSyncWorker() override;

  // base::Thread overrides
  void Init() override;
  void CleanUp() override;

  // These error codes are specified for diagnostic purposes when falling back
  // to delay based v-sync.
  enum class WaitForVBlankErrorCode {
    kSuccess = 0,
    kGetMonitorInfo = 1,
    kOpenAdapter = 2,
    kWaitForVBlankEvent = 3,
    kMaxValue = kWaitForVBlankEvent,
  };

  void Reschedule();
  bool OpenAdapter(const wchar_t* device_name);
  void CloseAdapter();
  NTSTATUS WaitForVBlankEvent();

  void AddTimestamp(base::TimeTicks timestamp);
  void AddInterval(base::TimeDelta interval);
  base::TimeDelta GetAverageInterval() const;
  void ClearHistory();

  bool GetDisplayFrequency(const wchar_t* device_name, DWORD* frequency);
  void UpdateCurrentDisplayFrequency();
  bool GetDwmVBlankTimestamp(base::TimeTicks* timestamp);

  void SendGpuVSyncUpdate(base::TimeTicks now, bool use_dwm);

  void InvokeCallbackAndReschedule(base::TimeTicks timestamp,
                                   base::TimeDelta interval);
  void UseDelayBasedVSyncOnError(WaitForVBlankErrorCode error_code);
  void ReportErrorCode(WaitForVBlankErrorCode error_code);

  // Specifies whether worker tasks are running.
  // This can be set on the worker thread only.
  bool running_ = false;

  // Specified whether the worker is enabled.  This is accessed from I/O
  // and v-sync threads and can be changed on main and I/O threads.
  base::subtle::AtomicWord enabled_ = false;

  // This is used to prevent a race condition when SetNeedsVsync
  // is called at or after v-sync thread shutdown.
  base::Lock shutdown_lock_;

  // Task runner for the worker thread, initialized in Init() and cleared in
  // CleanUp(). Can be used from any thread.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  const gfx::VSyncProvider::UpdateVSyncCallback callback_;
  const SurfaceHandle surface_handle_;

  PFND3DKMTOPENADAPTERFROMHDC open_adapter_from_hdc_ptr_;
  PFND3DKMTCLOSEADAPTER close_adapter_ptr_;
  PFND3DKMTWAITFORVERTICALBLANKEVENT wait_for_vertical_blank_event_ptr_;

  std::wstring current_device_name_;
  D3DKMT_HANDLE current_adapter_handle_ = 0;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID current_source_id_ = 0;

  // Last known v-sync timestamp.
  base::TimeTicks last_timestamp_;

  // Current display refresh frequency in Hz which is used to detect
  // when the frequency changes and and to update the accepted interval
  // range below.
  DWORD current_display_frequency_ = 0;

  // Range of intervals accepted for the average calculation which is
  // +/-20% from the interval corresponding to the display frequency above.
  // This is used to filter out outliers.
  base::TimeDelta min_accepted_interval_;
  base::TimeDelta max_accepted_interval_;

  // A simple circular buffer for storing a number of recent v-sync intervals
  // or DWM adjustment deltas and finding an average of them.
  class TimeDeltaRingBuffer {
   public:
    TimeDeltaRingBuffer() = default;
    ~TimeDeltaRingBuffer() = default;

    void Add(base::TimeDelta value) {
      if (size_ == kMaxSize) {
        rolling_sum_ -= values_[next_index_];
      } else {
        size_++;
      }

      values_[next_index_] = value;
      rolling_sum_ += value;
      next_index_ = (next_index_ + 1) % kMaxSize;
    }

    void Clear() {
      rolling_sum_ = base::TimeDelta();
      next_index_ = 0;
      size_ = 0;
    }

    base::TimeDelta GetAverage() const {
      if (size_ == 0)
        return base::TimeDelta();

      return rolling_sum_ / size_;
    }

   private:
    enum { kMaxSize = 60 };

    base::TimeDelta values_[kMaxSize];
    size_t next_index_ = 0;
    size_t size_ = 0;

    // Rolling sum of TimeDelta values in the circular buffer above.
    base::TimeDelta rolling_sum_;

    DISALLOW_COPY_AND_ASSIGN(TimeDeltaRingBuffer);
  };

  // History of recent deltas between timestamps used to calculate the average
  // v-sync interval.
  TimeDeltaRingBuffer recent_intervals_;

  // History of recent DWM adjustments used to calculate the average adjustment.
  TimeDeltaRingBuffer recent_adjustments_;

  DISALLOW_COPY_AND_ASSIGN(GpuVSyncWorker);
};

GpuVSyncWorker::GpuVSyncWorker(
    const gfx::VSyncProvider::UpdateVSyncCallback& callback,
    SurfaceHandle surface_handle)
    : base::Thread(base::StringPrintf("VSync-%p", surface_handle)),
      callback_(callback),
      surface_handle_(surface_handle) {
  HMODULE gdi32 = GetModuleHandle(L"gdi32");
  if (!gdi32) {
    NOTREACHED() << "Can't open gdi32.dll";
    return;
  }

  open_adapter_from_hdc_ptr_ = reinterpret_cast<PFND3DKMTOPENADAPTERFROMHDC>(
      ::GetProcAddress(gdi32, "D3DKMTOpenAdapterFromHdc"));
  if (!open_adapter_from_hdc_ptr_) {
    NOTREACHED() << "Can't find D3DKMTOpenAdapterFromHdc in gdi32.dll";
    return;
  }

  close_adapter_ptr_ = reinterpret_cast<PFND3DKMTCLOSEADAPTER>(
      ::GetProcAddress(gdi32, "D3DKMTCloseAdapter"));
  if (!close_adapter_ptr_) {
    NOTREACHED() << "Can't find D3DKMTCloseAdapter in gdi32.dll";
    return;
  }

  wait_for_vertical_blank_event_ptr_ =
      reinterpret_cast<PFND3DKMTWAITFORVERTICALBLANKEVENT>(
          ::GetProcAddress(gdi32, "D3DKMTWaitForVerticalBlankEvent"));
  if (!wait_for_vertical_blank_event_ptr_) {
    NOTREACHED() << "Can't find D3DKMTWaitForVerticalBlankEvent in gdi32.dll";
    return;
  }
}

GpuVSyncWorker::~GpuVSyncWorker() = default;

void GpuVSyncWorker::Init() {
  task_runner_ = task_runner();
}

void GpuVSyncWorker::CleanUp() {
  task_runner_ = nullptr;
}

void GpuVSyncWorker::CleanupAndStop() {
  base::AutoLock lock(shutdown_lock_);

  base::subtle::NoBarrier_Store(&enabled_, false);

  // Thread::Close() call below will block until this task has finished running
  // so it is safe to post it here and pass unretained pointer.
  task_runner_->PostTask(FROM_HERE, base::Bind(&GpuVSyncWorker::CloseAdapter,
                                               base::Unretained(this)));
  Stop();

  DCHECK_EQ(0u, current_adapter_handle_);
  DCHECK(current_device_name_.empty());
}

void GpuVSyncWorker::Enable(bool enabled) {
  base::AutoLock lock(shutdown_lock_);
  if (!task_runner_)
    return;

  auto was_enabled = base::subtle::NoBarrier_AtomicExchange(&enabled_, enabled);

  if (enabled && !was_enabled)
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&GpuVSyncWorker::StartRunningVSyncOnThread,
                              base::Unretained(this)));
}

bool GpuVSyncWorker::BelongsToWorkerThread() {
  return base::PlatformThread::CurrentId() == GetThreadId();
}

void GpuVSyncWorker::StartRunningVSyncOnThread() {
  DCHECK(BelongsToWorkerThread());

  if (!running_) {
    running_ = true;
    WaitForVSyncOnThread();
  }
}

void GpuVSyncWorker::WaitForVSyncOnThread() {
  DCHECK(BelongsToWorkerThread());

  TRACE_EVENT0("gpu", "GpuVSyncWorker::WaitForVSyncOnThread");

  HMONITOR monitor =
      MonitorFromWindow(surface_handle_, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info = {};
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  BOOL success = GetMonitorInfo(monitor, &monitor_info);
  if (!success) {
    // This is possible when a monitor is switched off or disconnected.
    CloseAdapter();
    UseDelayBasedVSyncOnError(WaitForVBlankErrorCode::kGetMonitorInfo);
    return;
  }

  if (current_device_name_.compare(monitor_info.szDevice) != 0) {
    // Monitor changed. Close the current adapter handle and open a new one.
    CloseAdapter();
    if (!OpenAdapter(monitor_info.szDevice)) {
      UseDelayBasedVSyncOnError(WaitForVBlankErrorCode::kOpenAdapter);
      return;
    }
  }

  UpdateCurrentDisplayFrequency();

  // Use DWM timing only when running on the primary monitor which DWM
  // is synchronized with and only if we can get accurate high resulution
  // timestamps.
  bool use_dwm = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0 &&
                 base::TimeTicks::IsHighResolution();

  NTSTATUS wait_result = WaitForVBlankEvent();
  if (wait_result != STATUS_SUCCESS) {
    if (wait_result == STATUS_GRAPHICS_PRESENT_OCCLUDED ||
        wait_result == WAIT_TIMEOUT) {
      // This may be triggered by the monitor going into sleep.
      UseDelayBasedVSyncOnError(WaitForVBlankErrorCode::kWaitForVBlankEvent);
      return;
    } else {
      base::debug::Alias(&wait_result);
      CHECK(false);
    }
  }

  SendGpuVSyncUpdate(base::TimeTicks::Now(), use_dwm);
  ReportErrorCode(WaitForVBlankErrorCode::kSuccess);
}

void GpuVSyncWorker::ReportErrorCode(WaitForVBlankErrorCode error_code) {
  UMA_HISTOGRAM_ENUMERATION("GPU.WaitForVBlankErrorCode", error_code);
}

void GpuVSyncWorker::AddTimestamp(base::TimeTicks timestamp) {
  if (!last_timestamp_.is_null()) {
    AddInterval(timestamp - last_timestamp_);
  }

  last_timestamp_ = timestamp;
}

void GpuVSyncWorker::AddInterval(base::TimeDelta interval) {
  if (interval < min_accepted_interval_ || interval > max_accepted_interval_)
    return;

  recent_intervals_.Add(interval);
}

void GpuVSyncWorker::ClearHistory() {
  last_timestamp_ = base::TimeTicks();
  recent_intervals_.Clear();
  recent_adjustments_.Clear();
}

base::TimeDelta GpuVSyncWorker::GetAverageInterval() const {
  base::TimeDelta average_interval = recent_intervals_.GetAverage();
  return !average_interval.is_zero()
             ? average_interval
             : base::TimeDelta::FromMicroseconds(kDefaultInterval);
}

bool GpuVSyncWorker::GetDisplayFrequency(const wchar_t* device_name,
                                         DWORD* frequency) {
  DEVMODE display_info;
  display_info.dmSize = sizeof(DEVMODE);
  display_info.dmDriverExtra = 0;

  BOOL result =
      EnumDisplaySettings(device_name, ENUM_CURRENT_SETTINGS, &display_info);
  if (result && display_info.dmDisplayFrequency > 1) {
    *frequency = display_info.dmDisplayFrequency;
    return true;
  }

  return false;
}

void GpuVSyncWorker::UpdateCurrentDisplayFrequency() {
  DWORD frequency;
  DCHECK(!current_device_name_.empty());
  if (!GetDisplayFrequency(current_device_name_.c_str(), &frequency)) {
    current_display_frequency_ = 0;
    return;
  }

  if (frequency != current_display_frequency_) {
    current_display_frequency_ = frequency;
    base::TimeDelta interval = base::TimeDelta::FromMicroseconds(
        base::Time::kMicrosecondsPerSecond / static_cast<double>(frequency));
    ClearHistory();

    min_accepted_interval_ = interval * 0.8;
    max_accepted_interval_ = interval * 1.2;
  }
}

bool GpuVSyncWorker::GetDwmVBlankTimestamp(base::TimeTicks* timestamp) {
  DWM_TIMING_INFO timing_info;
  timing_info.cbSize = sizeof(timing_info);
  HRESULT result = DwmGetCompositionTimingInfo(nullptr, &timing_info);
  if (result != S_OK)
    return false;

  *timestamp = base::TimeTicks::FromQPCValue(
      static_cast<LONGLONG>(timing_info.qpcVBlank));
  return true;
}

void GpuVSyncWorker::SendGpuVSyncUpdate(base::TimeTicks now, bool use_dwm) {
  base::TimeTicks timestamp;
  base::TimeDelta adjustment;

  if (use_dwm && GetDwmVBlankTimestamp(&timestamp)) {
    base::TimeDelta interval = GetAverageInterval();
    if (now - timestamp > interval * kMissingDwmTimestampsThreshold) {
      // DWM timestamp is too far in the past. Ignore it and use average
      // historical adjustment to be applied to |now| time to estimate
      // the v-sync timestamp.
      adjustment = recent_adjustments_.GetAverage();
    } else {
      // Timestamp comes from DwmGetCompositionTimingInfo and apparently it
      // might be a few v-sync cycles in the past or in the future.
      // The adjustment formula was suggested here:
      // http://www.vsynctester.com/firefoxisbroken.html
      adjustment =
          ((now - timestamp + interval / 8) % interval + interval) % interval -
          interval / 8;
      recent_adjustments_.Add(adjustment);
    }
    timestamp = now - adjustment;
  } else {
    // Not using DWM.
    timestamp = now;
  }

  AddTimestamp(timestamp);

  base::TimeDelta average_interval = GetAverageInterval();
  TRACE_EVENT2("gpu", "GpuVSyncWorker::SendGpuVSyncUpdate", "adjustment",
               adjustment.InMicroseconds(), "interval",
               average_interval.InMicroseconds());

  DCHECK_GT(average_interval.InMillisecondsF(), 0);
  InvokeCallbackAndReschedule(timestamp, average_interval);
}

void GpuVSyncWorker::InvokeCallbackAndReschedule(base::TimeTicks timestamp,
                                                 base::TimeDelta interval) {
  // Send update and restart the task if still enabled.
  if (base::subtle::NoBarrier_Load(&enabled_)) {
    callback_.Run(timestamp, interval);
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&GpuVSyncWorker::WaitForVSyncOnThread,
                                      base::Unretained(this)));
  } else {
    running_ = false;
    // Clear last_timestamp_ to avoid a long interval when the worker restarts.
    last_timestamp_ = base::TimeTicks();
  }
}

void GpuVSyncWorker::UseDelayBasedVSyncOnError(
    WaitForVBlankErrorCode error_code) {
  // This is called in a case of an error.
  // Use timer based mechanism as a backup for one v-sync cycle, start with
  // getting VSync parameters to determine timebase and interval.
  // TODO(stanisc): Consider a slower v-sync rate in this particular case.
  base::TimeTicks timebase;
  GetDwmVBlankTimestamp(&timebase);

  base::TimeDelta interval = GetAverageInterval();
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeTicks next_vsync = now.SnappedToNextTick(timebase, interval);

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&GpuVSyncWorker::InvokeCallbackAndReschedule,
                 base::Unretained(this), next_vsync, interval),
      next_vsync - now);

  ReportErrorCode(error_code);
}

bool GpuVSyncWorker::OpenAdapter(const wchar_t* device_name) {
  DCHECK_EQ(0u, current_adapter_handle_);

  HDC hdc = CreateDC(NULL, device_name, NULL, NULL);

  D3DKMT_OPENADAPTERFROMHDC open_adapter_data;
  open_adapter_data.hDc = hdc;

  NTSTATUS result = open_adapter_from_hdc_ptr_(&open_adapter_data);
  DeleteDC(hdc);

  if (result != STATUS_SUCCESS) {
    // The most likely reason for this is a result of race condition between
    // this code and the monitor being disconnected, going to sleep, or being
    // locked out.
    return false;
  }

  current_device_name_ = device_name;
  current_adapter_handle_ = open_adapter_data.hAdapter;
  current_source_id_ = open_adapter_data.VidPnSourceId;
  return true;
}

void GpuVSyncWorker::CloseAdapter() {
  if (current_adapter_handle_ != 0) {
    D3DKMT_CLOSEADAPTER close_adapter_data;
    close_adapter_data.hAdapter = current_adapter_handle_;

    NTSTATUS result = close_adapter_ptr_(&close_adapter_data);
    CHECK(result == STATUS_SUCCESS);

    current_adapter_handle_ = 0;
    current_device_name_.clear();

    ClearHistory();
  }
}

NTSTATUS GpuVSyncWorker::WaitForVBlankEvent() {
  D3DKMT_WAITFORVERTICALBLANKEVENT wait_for_vertical_blank_event_data;
  wait_for_vertical_blank_event_data.hAdapter = current_adapter_handle_;
  wait_for_vertical_blank_event_data.hDevice = 0;
  wait_for_vertical_blank_event_data.VidPnSourceId = current_source_id_;

  return wait_for_vertical_blank_event_ptr_(
      &wait_for_vertical_blank_event_data);
}

// MessageFilter class for sending and receiving IPC messages
// directly, avoiding routing them through the main GPU thread.
class GpuVSyncMessageFilter : public IPC::MessageFilter {
 public:
  explicit GpuVSyncMessageFilter(
      const scoped_refptr<GpuVSyncWorker>& vsync_worker,
      int32_t route_id)
      : vsync_worker_(vsync_worker), route_id_(route_id) {}

  // IPC::MessageFilter overrides.
  void OnChannelError() override { Reset(); }
  void OnChannelClosing() override { Reset(); }
  void OnFilterAdded(IPC::Channel* channel) override;
  void OnFilterRemoved() override { Reset(); }
  bool OnMessageReceived(const IPC::Message& msg) override;

  // Send can be called from GpuVSyncWorker thread.
  void Send(std::unique_ptr<IPC::Message> message);

  int32_t route_id() const { return route_id_; }

 private:
  ~GpuVSyncMessageFilter() override = default;
  void SendOnIOThread(std::unique_ptr<IPC::Message> message);
  void Reset();

  scoped_refptr<GpuVSyncWorker> vsync_worker_;
  // The sender to which this filter was added.
  IPC::Sender* sender_ = nullptr;
  // The sender must be invoked on IO thread.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  const int32_t route_id_;
};

void GpuVSyncMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  io_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  sender_ = channel;
}

void GpuVSyncMessageFilter::Reset() {
  sender_ = nullptr;
  vsync_worker_->Enable(false);
}

bool GpuVSyncMessageFilter::OnMessageReceived(const IPC::Message& msg) {
  if (msg.routing_id() != route_id_)
    return false;

  IPC_BEGIN_MESSAGE_MAP(GpuVSyncMessageFilter, msg)
    IPC_MESSAGE_FORWARD(GpuCommandBufferMsg_SetNeedsVSync, vsync_worker_.get(),
                        GpuVSyncWorker::Enable);
    IPC_MESSAGE_UNHANDLED(return false)
  IPC_END_MESSAGE_MAP()
  return true;
}

void GpuVSyncMessageFilter::Send(std::unique_ptr<IPC::Message> message) {
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&GpuVSyncMessageFilter::SendOnIOThread, this,
                            base::Passed(&message)));
}

void GpuVSyncMessageFilter::SendOnIOThread(
    std::unique_ptr<IPC::Message> message) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(!message->is_sync());
  if (!sender_)
    return;

  sender_->Send(message.release());
}

GpuVSyncProviderWin::GpuVSyncProviderWin(
    base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
    SurfaceHandle surface_handle) {
  vsync_worker_ = new GpuVSyncWorker(
      base::Bind(&GpuVSyncProviderWin::OnVSync, base::Unretained(this)),
      surface_handle);

  // Start the thread.
  base::Thread::Options options;
  // Realtime priority is needed to ensure the minimal possible wakeup latency
  // and to ensure that the thread isn't pre-empted when it handles the v-blank
  // wake-up.  The thread sleeps most of the time and does a tiny amount of
  // actual work on each cycle. So the increased priority is mostly for the best
  // possible latency.
  options.priority = base::ThreadPriority::REALTIME_AUDIO;
  vsync_worker_->StartWithOptions(options);

  // Add IPC message filter.
  message_filter_ =
      new GpuVSyncMessageFilter(vsync_worker_, delegate->GetRouteID());
  delegate->AddFilter(message_filter_.get());
}

GpuVSyncProviderWin::~GpuVSyncProviderWin() {
  vsync_worker_->CleanupAndStop();
}

void GpuVSyncProviderWin::GetVSyncParameters(
    const UpdateVSyncCallback& callback) {
  // This is ignored and the |callback| is never called back.  The timestamp
  // and interval are posted directly via
  // GpuCommandBufferMsg_UpdateVSyncParameters message sent from the worker
  // thread.
}

bool GpuVSyncProviderWin::GetVSyncParametersIfAvailable(
    base::TimeTicks* timebase,
    base::TimeDelta* interval) {
  return false;
}

bool GpuVSyncProviderWin::SupportGetVSyncParametersIfAvailable() const {
  return false;
}

bool GpuVSyncProviderWin::IsHWClock() const {
  return false;
}

void GpuVSyncProviderWin::OnVSync(base::TimeTicks timestamp,
                                  base::TimeDelta interval) {
  DCHECK(vsync_worker_->BelongsToWorkerThread());

  message_filter_->Send(
      std::make_unique<GpuCommandBufferMsg_UpdateVSyncParameters>(
          message_filter_->route_id(), timestamp, interval));
}

}  // namespace gpu
