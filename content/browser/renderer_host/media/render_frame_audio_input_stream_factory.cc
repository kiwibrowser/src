// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/render_frame_audio_input_stream_factory.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/media/capture/desktop_capture_device_uma_types.h"
#include "content/browser/media/forwarding_audio_stream_factory.h"
#include "content/browser/media/media_devices_permission_checker.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_device_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "content/public/common/media_stream_request.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_input_device.h"
#include "media/base/audio_parameters.h"

namespace content {

namespace {

void LookUpDeviceAndRespondIfFound(
    scoped_refptr<AudioInputDeviceManager> audio_input_device_manager,
    int32_t session_id,
    base::OnceCallback<void(const MediaStreamDevice&)> response) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const MediaStreamDevice* device =
      audio_input_device_manager->GetOpenedDeviceById(session_id);
  if (device) {
    // Copies device.
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(response), *device));
  }
}

void EnumerateOutputDevices(MediaStreamManager* media_stream_manager,
                            MediaDevicesManager::EnumerationCallback cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  MediaDevicesManager::BoolDeviceTypes device_types;
  device_types[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  media_stream_manager->media_devices_manager()->EnumerateDevices(device_types,
                                                                  cb);
}

void TranslateDeviceId(const std::string& device_id,
                       const MediaDeviceSaltAndOrigin& salt_and_origin,
                       base::RepeatingCallback<void(const std::string&)> cb,
                       const MediaDeviceEnumeration& device_array) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (const auto& device_info : device_array[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT]) {
    if (MediaStreamManager::DoesMediaDeviceIDMatchHMAC(
            salt_and_origin.device_id_salt, salt_and_origin.origin, device_id,
            device_info.device_id)) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(std::move(cb), device_info.device_id));
      break;
    }
  }
  // If we're unable to translate the device id, |cb| will not be run.
}

}  // namespace

RenderFrameAudioInputStreamFactory::RenderFrameAudioInputStreamFactory(
    mojom::RendererAudioInputStreamFactoryRequest request,
    scoped_refptr<AudioInputDeviceManager> audio_input_device_manager,
    RenderFrameHost* render_frame_host)
    : binding_(this, std::move(request)),
      audio_input_device_manager_(std::move(audio_input_device_manager)),
      render_frame_host_(render_frame_host),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RenderFrameAudioInputStreamFactory::~RenderFrameAudioInputStreamFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RenderFrameAudioInputStreamFactory::CreateStream(
    mojom::RendererAudioInputStreamFactoryClientPtr client,
    int32_t session_id,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TRACE_EVENT_INSTANT1("audio",
                       "RenderFrameAudioInputStreamFactory::CreateStream",
                       TRACE_EVENT_SCOPE_THREAD, "session id", session_id);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &LookUpDeviceAndRespondIfFound, audio_input_device_manager_,
          session_id,
          base::BindOnce(&RenderFrameAudioInputStreamFactory::
                             CreateStreamAfterLookingUpDevice,
                         weak_ptr_factory_.GetWeakPtr(), std::move(client),
                         audio_params, automatic_gain_control,
                         shared_memory_count)));
}

void RenderFrameAudioInputStreamFactory::CreateStreamAfterLookingUpDevice(
    mojom::RendererAudioInputStreamFactoryClientPtr client,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count,
    const MediaStreamDevice& device) {
  TRACE_EVENT1(
      "audio",
      "RenderFrameAudioInputStreamFactory::CreateStreamAfterLookingUpDevice",
      "device id", device.id);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ForwardingAudioStreamFactory* factory =
      ForwardingAudioStreamFactory::ForFrame(render_frame_host_);
  if (!factory)
    return;

  WebContentsMediaCaptureId capture_id;
  if (WebContentsMediaCaptureId::Parse(device.id, &capture_id)) {
    // For MEDIA_DESKTOP_AUDIO_CAPTURE, the source is selected from picker
    // window, we do not mute the source audio.
    // For MEDIA_TAB_AUDIO_CAPTURE, the probable use case is Cast, we mute
    // the source audio.
    // TODO(qiangchen): Analyze audio constraints to make a duplicating or
    // diverting decision. It would give web developer more flexibility.

    RenderFrameHost* source_host = RenderFrameHost::FromID(
        capture_id.render_process_id, capture_id.main_render_frame_id);
    if (!source_host) {
      // The source of the capture has already been destroyed, so fail early.
      return;
    }

    factory->CreateLoopbackStream(
        render_frame_host_, source_host, audio_params, shared_memory_count,
        capture_id.disable_local_echo, std::move(client));

    if (device.type == MEDIA_DESKTOP_AUDIO_CAPTURE)
      IncrementDesktopCaptureCounter(SYSTEM_LOOPBACK_AUDIO_CAPTURER_CREATED);
  } else {
    factory->CreateInputStream(render_frame_host_, device.id, audio_params,
                               shared_memory_count, automatic_gain_control,
                               std::move(client));

    // Only count for captures from desktop media picker dialog and system loop
    // back audio.
    if (device.type == MEDIA_DESKTOP_AUDIO_CAPTURE &&
        (media::AudioDeviceDescription::IsLoopbackDevice(device.id))) {
      IncrementDesktopCaptureCounter(SYSTEM_LOOPBACK_AUDIO_CAPTURER_CREATED);
    }
  }
}

void RenderFrameAudioInputStreamFactory::AssociateInputAndOutputForAec(
    const base::UnguessableToken& input_stream_id,
    const std::string& output_device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsValidDeviceId(output_device_id))
    return;

  ForwardingAudioStreamFactory* factory =
      ForwardingAudioStreamFactory::ForFrame(render_frame_host_);
  if (!factory)
    return;

  const int process_id = render_frame_host_->GetProcess()->GetID();
  const int frame_id = render_frame_host_->GetRoutingID();
  auto salt_and_origin = GetMediaDeviceSaltAndOrigin(process_id, frame_id);

  // Check permissions for everything but the default device
  if (!media::AudioDeviceDescription::IsDefaultDevice(output_device_id) &&
      !MediaDevicesPermissionChecker().CheckPermissionOnUIThread(
          MEDIA_DEVICE_TYPE_AUDIO_OUTPUT, process_id, frame_id)) {
    return;
  }

  if (media::AudioDeviceDescription::IsDefaultDevice(output_device_id) ||
      media::AudioDeviceDescription::IsCommunicationsDevice(output_device_id)) {
    factory->AssociateInputAndOutputForAec(input_stream_id, output_device_id);
  } else {
    auto* media_stream_manager =
        BrowserMainLoop::GetInstance()->media_stream_manager();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            EnumerateOutputDevices, media_stream_manager,
            base::BindRepeating(
                &TranslateDeviceId, output_device_id, salt_and_origin,
                base::BindRepeating(&RenderFrameAudioInputStreamFactory::
                                        AssociateTranslatedOutputDeviceForAec,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    input_stream_id))));
  }
}

void RenderFrameAudioInputStreamFactory::AssociateTranslatedOutputDeviceForAec(
    const base::UnguessableToken& input_stream_id,
    const std::string& raw_output_device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ForwardingAudioStreamFactory* factory =
      ForwardingAudioStreamFactory::ForFrame(render_frame_host_);
  if (factory)
    factory->AssociateInputAndOutputForAec(input_stream_id,
                                           raw_output_device_id);
}

}  // namespace content
