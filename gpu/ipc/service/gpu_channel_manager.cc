// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_channel_manager.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/command_buffer/service/gpu_tracer.h"
#include "gpu/command_buffer/service/mailbox_manager_factory.h"
#include "gpu/command_buffer/service/memory_program_cache.h"
#include "gpu/command_buffer/service/passthrough_program_cache.h"
#include "gpu/command_buffer/service/scheduler.h"
#include "gpu/command_buffer/service/service_utils.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "gpu/ipc/service/gpu_channel_manager_delegate.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory.h"
#include "gpu/ipc/service/gpu_memory_manager.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/init/gl_factory.h"

namespace gpu {

namespace {
#if defined(OS_ANDROID)
// Amount of time we expect the GPU to stay powered up without being used.
const int kMaxGpuIdleTimeMs = 40;
// Maximum amount of time we keep pinging the GPU waiting for the client to
// draw.
const int kMaxKeepAliveTimeMs = 200;
#endif
}

GpuChannelManager::GpuChannelManager(
    const GpuPreferences& gpu_preferences,
    GpuChannelManagerDelegate* delegate,
    GpuWatchdogThread* watchdog,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    Scheduler* scheduler,
    SyncPointManager* sync_point_manager,
    GpuMemoryBufferFactory* gpu_memory_buffer_factory,
    const GpuFeatureInfo& gpu_feature_info,
    GpuProcessActivityFlags activity_flags)
    : task_runner_(task_runner),
      io_task_runner_(io_task_runner),
      gpu_preferences_(gpu_preferences),
      gpu_driver_bug_workarounds_(
          gpu_feature_info.enabled_gpu_driver_bug_workarounds),
      delegate_(delegate),
      watchdog_(watchdog),
      share_group_(new gl::GLShareGroup()),
      mailbox_manager_(gles2::CreateMailboxManager(gpu_preferences)),
      gpu_memory_manager_(this),
      scheduler_(scheduler),
      sync_point_manager_(sync_point_manager),
      shader_translator_cache_(gpu_preferences_),
      gpu_memory_buffer_factory_(gpu_memory_buffer_factory),
      gpu_feature_info_(gpu_feature_info),
      exiting_for_lost_context_(false),
      activity_flags_(std::move(activity_flags)),
      memory_pressure_listener_(
          base::Bind(&GpuChannelManager::HandleMemoryPressure,
                     base::Unretained(this))),
      weak_factory_(this) {
  DCHECK(task_runner->BelongsToCurrentThread());
  DCHECK(io_task_runner);
  DCHECK(scheduler);
}

GpuChannelManager::~GpuChannelManager() {
  // Destroy channels before anything else because of dependencies.
  gpu_channels_.clear();
  if (default_offscreen_surface_.get()) {
    default_offscreen_surface_->Destroy();
    default_offscreen_surface_ = NULL;
  }
}

gles2::Outputter* GpuChannelManager::outputter() {
  if (!outputter_)
    outputter_.reset(new gles2::TraceOutputter("GpuChannelManager Trace"));
  return outputter_.get();
}

gles2::ProgramCache* GpuChannelManager::program_cache() {
  if (!program_cache_.get()) {
    const GpuDriverBugWorkarounds& workarounds = gpu_driver_bug_workarounds_;
    bool disable_disk_cache =
        gpu_preferences_.disable_gpu_shader_disk_cache ||
        workarounds.disable_program_disk_cache;

    // Use the EGL cache control extension for the passthrough decoder.
    if (gpu_preferences_.use_passthrough_cmd_decoder &&
        gles2::PassthroughCommandDecoderSupported()) {
      program_cache_.reset(new gles2::PassthroughProgramCache(
          gpu_preferences_.gpu_program_cache_size, disable_disk_cache));
    } else {
      program_cache_.reset(new gles2::MemoryProgramCache(
          gpu_preferences_.gpu_program_cache_size, disable_disk_cache,
          workarounds.disable_program_caching_for_transform_feedback,
          &activity_flags_));
    }
  }
  return program_cache_.get();
}

void GpuChannelManager::RemoveChannel(int client_id) {
  delegate_->DidDestroyChannel(client_id);
  gpu_channels_.erase(client_id);
}

GpuChannel* GpuChannelManager::LookupChannel(int32_t client_id) const {
  const auto& it = gpu_channels_.find(client_id);
  return it != gpu_channels_.end() ? it->second.get() : nullptr;
}

GpuChannel* GpuChannelManager::EstablishChannel(int client_id,
                                                uint64_t client_tracing_id,
                                                bool is_gpu_host) {
  std::unique_ptr<GpuChannel> gpu_channel = std::make_unique<GpuChannel>(
      this, scheduler_, sync_point_manager_, share_group_, task_runner_,
      io_task_runner_, client_id, client_tracing_id, is_gpu_host);

  GpuChannel* gpu_channel_ptr = gpu_channel.get();
  gpu_channels_[client_id] = std::move(gpu_channel);
  return gpu_channel_ptr;
}

void GpuChannelManager::InternalDestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&GpuChannelManager::InternalDestroyGpuMemoryBufferOnIO,
                 base::Unretained(this), id, client_id));
}

void GpuChannelManager::InternalDestroyGpuMemoryBufferOnIO(
    gfx::GpuMemoryBufferId id,
    int client_id) {
  gpu_memory_buffer_factory_->DestroyGpuMemoryBuffer(id, client_id);
}

void GpuChannelManager::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                               int client_id,
                                               const SyncToken& sync_token) {
  if (!sync_point_manager_->WaitOutOfOrder(
          sync_token,
          base::Bind(&GpuChannelManager::InternalDestroyGpuMemoryBuffer,
                     base::Unretained(this), id, client_id))) {
    // No sync token or invalid sync token, destroy immediately.
    InternalDestroyGpuMemoryBuffer(id, client_id);
  }
}

void GpuChannelManager::PopulateShaderCache(const std::string& key,
                                            const std::string& program) {
  if (program_cache())
    program_cache()->LoadProgram(key, program);
}

void GpuChannelManager::LoseAllContexts() {
  for (auto& kv : gpu_channels_) {
    kv.second->MarkAllContextsLost();
  }
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&GpuChannelManager::DestroyAllChannels,
                                    weak_factory_.GetWeakPtr()));
}

void GpuChannelManager::MaybeExitOnContextLost() {
  if (!gpu_preferences().single_process && !gpu_preferences().in_process_gpu) {
    LOG(ERROR) << "Exiting GPU process because some drivers cannot recover"
               << " from problems.";
    // Signal the message loop to quit to shut down other threads
    // gracefully.
    base::RunLoop::QuitCurrentDeprecated();
    exiting_for_lost_context_ = true;
  }
}

void GpuChannelManager::DestroyAllChannels() {
  gpu_channels_.clear();
}

gl::GLSurface* GpuChannelManager::GetDefaultOffscreenSurface() {
  if (!default_offscreen_surface_.get()) {
    default_offscreen_surface_ =
        gl::init::CreateOffscreenGLSurface(gfx::Size());
  }
  return default_offscreen_surface_.get();
}

#if defined(OS_ANDROID)
void GpuChannelManager::DidAccessGpu() {
  last_gpu_access_time_ = base::TimeTicks::Now();
}

void GpuChannelManager::WakeUpGpu() {
  begin_wake_up_time_ = base::TimeTicks::Now();
  ScheduleWakeUpGpu();
}

void GpuChannelManager::ScheduleWakeUpGpu() {
  base::TimeTicks now = base::TimeTicks::Now();
  TRACE_EVENT2("gpu", "GpuChannelManager::ScheduleWakeUp", "idle_time",
               (now - last_gpu_access_time_).InMilliseconds(),
               "keep_awake_time", (now - begin_wake_up_time_).InMilliseconds());
  if (now - last_gpu_access_time_ <
      base::TimeDelta::FromMilliseconds(kMaxGpuIdleTimeMs))
    return;
  if (now - begin_wake_up_time_ >
      base::TimeDelta::FromMilliseconds(kMaxKeepAliveTimeMs))
    return;

  DoWakeUpGpu();

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&GpuChannelManager::ScheduleWakeUpGpu,
                            weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kMaxGpuIdleTimeMs));
}

void GpuChannelManager::DoWakeUpGpu() {
  const CommandBufferStub* stub = nullptr;
  for (const auto& kv : gpu_channels_) {
    const GpuChannel* channel = kv.second.get();
    stub = channel->GetOneStub();
    if (stub) {
      DCHECK(stub->decoder_context());
      break;
    }
  }
  if (!stub || !stub->decoder_context()->MakeCurrent())
    return;
  glFinish();
  DidAccessGpu();
}

void GpuChannelManager::OnApplicationBackgrounded() {
  // Delete all the GL contexts when the channel does not use WebGL and Chrome
  // goes to background on low-end devices.
  std::vector<int> channels_to_clear;
  for (auto& kv : gpu_channels_) {
    // TODO(ssid): WebGL context loss event notification must be sent before
    // clearing WebGL contexts crbug.com/725306.
    if (kv.second->HasActiveWebGLContext())
      continue;
    channels_to_clear.push_back(kv.first);
    kv.second->MarkAllContextsLost();
  }
  for (int channel : channels_to_clear)
    RemoveChannel(channel);

  if (program_cache_)
    program_cache_->Trim(0u);
}
#endif

void GpuChannelManager::HandleMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  if (program_cache_)
    program_cache_->HandleMemoryPressure(memory_pressure_level);
  discardable_manager_.HandleMemoryPressure(memory_pressure_level);
}

}  // namespace gpu
