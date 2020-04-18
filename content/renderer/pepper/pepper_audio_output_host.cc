// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_audio_output_host.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "content/common/pepper_file_util.h"
#include "content/renderer/pepper/pepper_audio_controller.h"
#include "content/renderer/pepper/pepper_media_device_manager.h"
#include "content/renderer/pepper/pepper_platform_audio_output_dev.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/plugin_instance_throttler_impl.h"
#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_structs.h"

namespace content {

PepperAudioOutputHost::PepperAudioOutputHost(RendererPpapiHostImpl* host,
                                             PP_Instance instance,
                                             PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      audio_output_(nullptr),
      playback_throttled_(false),
      enumeration_helper_(this,
                          PepperMediaDeviceManager::GetForRenderFrame(
                              host->GetRenderFrameForInstance(pp_instance())),
                          PP_DEVICETYPE_DEV_AUDIOOUTPUT,
                          host->GetDocumentURL(instance)) {
  PepperPluginInstanceImpl* plugin_instance =
      static_cast<PepperPluginInstanceImpl*>(
          PepperPluginInstance::Get(pp_instance()));
  if (plugin_instance && plugin_instance->throttler())
    plugin_instance->throttler()->AddObserver(this);
}

PepperAudioOutputHost::~PepperAudioOutputHost() {
  PepperPluginInstanceImpl* instance = static_cast<PepperPluginInstanceImpl*>(
      PepperPluginInstance::Get(pp_instance()));
  if (instance) {
    if (instance->throttler()) {
      instance->throttler()->RemoveObserver(this);
    }
    instance->audio_controller().RemoveInstance(this);
  }
  Close();
}

int32_t PepperAudioOutputHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  int32_t result = PP_ERROR_FAILED;
  if (enumeration_helper_.HandleResourceMessage(msg, context, &result))
    return result;

  PPAPI_BEGIN_MESSAGE_MAP(PepperAudioOutputHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_AudioOutput_Open, OnOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_AudioOutput_StartOrStop,
                                      OnStartOrStop)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_AudioOutput_Close, OnClose)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

void PepperAudioOutputHost::StreamCreated(
    base::SharedMemoryHandle shared_memory_handle,
    size_t shared_memory_size,
    base::SyncSocket::Handle socket) {
  OnOpenComplete(PP_OK, shared_memory_handle, shared_memory_size, socket);
}

void PepperAudioOutputHost::StreamCreationFailed() {
  OnOpenComplete(PP_ERROR_FAILED, base::SharedMemoryHandle(), 0,
                 base::SyncSocket::kInvalidHandle);
}

void PepperAudioOutputHost::SetVolume(double volume) {
  if (audio_output_)
    audio_output_->SetVolume(volume);
}

int32_t PepperAudioOutputHost::OnOpen(ppapi::host::HostMessageContext* context,
                                      const std::string& device_id,
                                      PP_AudioSampleRate sample_rate,
                                      uint32_t sample_frame_count) {
  if (open_context_.is_valid())
    return PP_ERROR_INPROGRESS;

  if (audio_output_)
    return PP_ERROR_FAILED;

  // When it is done, we'll get called back on StreamCreated() or
  // StreamCreationFailed().
  audio_output_ = PepperPlatformAudioOutputDev::Create(
      renderer_ppapi_host_->GetRenderFrameForInstance(pp_instance())
          ->GetRoutingID(),
      device_id, static_cast<int>(sample_rate),
      static_cast<int>(sample_frame_count), this);
  if (audio_output_) {
    open_context_ = context->MakeReplyMessageContext();
    return PP_OK_COMPLETIONPENDING;
  } else {
    return PP_ERROR_FAILED;
  }
}

int32_t PepperAudioOutputHost::OnStartOrStop(
    ppapi::host::HostMessageContext* /* context */,
    bool playback) {
  if (!audio_output_)
    return PP_ERROR_FAILED;

  PepperPluginInstanceImpl* instance = static_cast<PepperPluginInstanceImpl*>(
      PepperPluginInstance::Get(pp_instance()));

  if (playback) {
    // If plugin is in power saver mode, defer audio IPC communication.
    if (instance && instance->throttler() &&
        instance->throttler()->power_saver_enabled()) {
      instance->throttler()->NotifyAudioThrottled();
      playback_throttled_ = true;
      return PP_TRUE;
    }
    if (instance)
      instance->audio_controller().AddInstance(this);

    audio_output_->StartPlayback();
  } else {
    if (instance)
      instance->audio_controller().RemoveInstance(this);

    audio_output_->StopPlayback();
  }
  return PP_OK;
}

int32_t PepperAudioOutputHost::OnClose(
    ppapi::host::HostMessageContext* /* context */) {
  Close();
  return PP_OK;
}

void PepperAudioOutputHost::OnOpenComplete(
    int32_t result,
    base::SharedMemoryHandle shared_memory_handle,
    size_t shared_memory_size,
    base::SyncSocket::Handle socket_handle) {
  // Make sure the handles are cleaned up.
  base::SyncSocket scoped_socket(socket_handle);
  base::SharedMemory scoped_shared_memory(shared_memory_handle, false);

  if (!open_context_.is_valid()) {
    NOTREACHED();
    return;
  }

  ppapi::proxy::SerializedHandle serialized_socket_handle(
      ppapi::proxy::SerializedHandle::SOCKET);
  ppapi::proxy::SerializedHandle serialized_shared_memory_handle(
      ppapi::proxy::SerializedHandle::SHARED_MEMORY);

  if (result == PP_OK) {
    IPC::PlatformFileForTransit temp_socket =
        IPC::InvalidPlatformFileForTransit();
    base::SharedMemoryHandle temp_shmem;
    result = GetRemoteHandles(scoped_socket, scoped_shared_memory, &temp_socket,
                              &temp_shmem);

    serialized_socket_handle.set_socket(temp_socket);
    serialized_shared_memory_handle.set_shmem(temp_shmem, shared_memory_size);
  }

  // Send all the values, even on error. This simplifies some of our cleanup
  // code since the handles will be in the other process and could be
  // inconvenient to clean up. Our IPC code will automatically handle this for
  // us, as long as the remote side always closes the handles it receives, even
  // in the failure case.
  open_context_.params.AppendHandle(serialized_socket_handle);
  open_context_.params.AppendHandle(serialized_shared_memory_handle);
  SendOpenReply(result);
}

int32_t PepperAudioOutputHost::GetRemoteHandles(
    const base::SyncSocket& socket,
    const base::SharedMemory& shared_memory,
    IPC::PlatformFileForTransit* remote_socket_handle,
    base::SharedMemoryHandle* remote_shared_memory_handle) {
  *remote_socket_handle =
      renderer_ppapi_host_->ShareHandleWithRemote(socket.handle(), false);
  if (*remote_socket_handle == IPC::InvalidPlatformFileForTransit())
    return PP_ERROR_FAILED;

  *remote_shared_memory_handle =
      renderer_ppapi_host_->ShareSharedMemoryHandleWithRemote(
          shared_memory.handle());
  if (!base::SharedMemory::IsHandleValid(*remote_shared_memory_handle))
    return PP_ERROR_FAILED;

  return PP_OK;
}

void PepperAudioOutputHost::Close() {
  if (!audio_output_)
    return;

  audio_output_->ShutDown();
  audio_output_ = nullptr;

  if (open_context_.is_valid())
    SendOpenReply(PP_ERROR_ABORTED);
}

void PepperAudioOutputHost::SendOpenReply(int32_t result) {
  open_context_.params.set_result(result);
  host()->SendReply(open_context_, PpapiPluginMsg_AudioOutput_OpenReply());
  open_context_ = ppapi::host::ReplyMessageContext();
}

void PepperAudioOutputHost::OnThrottleStateChange() {
  PepperPluginInstanceImpl* instance = static_cast<PepperPluginInstanceImpl*>(
      PepperPluginInstance::Get(pp_instance()));
  if (playback_throttled_ && instance && instance->throttler() &&
      !instance->throttler()->power_saver_enabled()) {
    // If we have become unthrottled, and we have a pending playback,
    // start it.
    StartDeferredPlayback();
  }
}

void PepperAudioOutputHost::StartDeferredPlayback() {
  if (!audio_output_)
    return;

  DCHECK(playback_throttled_);
  playback_throttled_ = false;

  PepperPluginInstanceImpl* instance = static_cast<PepperPluginInstanceImpl*>(
      PepperPluginInstance::Get(pp_instance()));
  if (instance)
    instance->audio_controller().AddInstance(this);

  audio_output_->StartPlayback();
}

}  // namespace content
