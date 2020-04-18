// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_USER_MEDIA_PROCESSOR_H_
#define CONTENT_RENDERER_MEDIA_STREAM_USER_MEDIA_PROCESSOR_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/media_stream.mojom.h"
#include "content/renderer/media/stream/media_stream_dispatcher_eventhandler.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "third_party/blink/public/platform/modules/mediastream/media_devices.mojom.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_user_media_request.h"

namespace gfx {
class Size;
}

namespace blink {
class WebMediaStream;
class WebMediaStreamSource;
class WebString;
}  // namespace blink

namespace content {

class AudioCaptureSettings;
class AudioDeviceCaptureCapability;
class MediaStreamAudioSource;
class MediaStreamDeviceObserver;
class MediaStreamVideoSource;
class PeerConnectionDependencyFactory;
class VideoCaptureSettings;
class RenderFrameImpl;

// TODO(guidou): Add |request_id| and |is_processing_user_gesture| to
// blink::WebUserMediaRequest and remove this struct.
struct UserMediaRequest {
  UserMediaRequest(int request_id,
                   const blink::WebUserMediaRequest& web_request,
                   bool is_processing_user_gesture);
  const int request_id;
  const blink::WebUserMediaRequest web_request;
  const bool is_processing_user_gesture;
};

// UserMediaProcessor is responsible for processing getUserMedia() requests.
// It also keeps tracks of all sources used by streams created with
// getUserMedia().
// It communicates with the browser via MediaStreamDispatcherHost.
// Only one MediaStream at a time can be in the process of being created.
// UserMediaProcessor must be created, called and destroyed on the main render
// thread. There should be only one UserMediaProcessor per frame.
class CONTENT_EXPORT UserMediaProcessor
    : public MediaStreamDispatcherEventHandler {
 public:
  using MediaDevicesDispatcherCallback = base::RepeatingCallback<
      const blink::mojom::MediaDevicesDispatcherHostPtr&()>;
  // |render_frame| and |dependency_factory| must outlive this instance.
  UserMediaProcessor(
      RenderFrameImpl* render_frame,
      PeerConnectionDependencyFactory* dependency_factory,
      std::unique_ptr<MediaStreamDeviceObserver> media_stream_device_observer,
      MediaDevicesDispatcherCallback media_devices_dispatcher_cb);
  ~UserMediaProcessor() override;

  // It can be assumed that the output of CurrentRequest() remains the same
  // during the execution of a task on the main thread unless ProcessRequest or
  // DeleteWebRequest are invoked.
  // TODO(guidou): Remove this method. http://crbug.com/764293
  UserMediaRequest* CurrentRequest();

  // Starts processing |request| in order to create a new MediaStream. When
  // processing of |request| is complete, it notifies by invoking |callback|.
  // This method must be called only if there is no request currently being
  // processed.
  void ProcessRequest(std::unique_ptr<UserMediaRequest> request,
                      base::OnceClosure callback);

  // If |web_request| is the request currently being processed, stops processing
  // the request and returns true. Otherwise, performs no action and returns
  // false.
  // TODO(guidou): Make this method private and replace with a public
  // CancelRequest() method that deletes the request only if it has not been
  // generated yet. http://crbug.com/764293
  bool DeleteWebRequest(const blink::WebUserMediaRequest& web_request);

  // Stops processing the current request, if any, and stops all sources
  // currently being tracked, effectively stopping all tracks associated with
  // those sources.
  void StopAllProcessing();

  MediaStreamDeviceObserver* media_stream_device_observer() const {
    return media_stream_device_observer_.get();
  }

  // MediaStreamDispatcherEventHandler implementation.
  void OnDeviceStopped(const MediaStreamDevice& device) override;

  void set_media_stream_dispatcher_host_for_testing(
      mojom::MediaStreamDispatcherHostPtr dispatcher_host) {
    dispatcher_host_ = std::move(dispatcher_host);
  }

 protected:
  // These methods are virtual for test purposes. A test can override them to
  // test requesting local media streams. The function notifies WebKit that the
  // |request| have completed.
  virtual void GetUserMediaRequestSucceeded(
      const blink::WebMediaStream& stream,
      blink::WebUserMediaRequest web_request);
  virtual void GetUserMediaRequestFailed(
      MediaStreamRequestResult result,
      const blink::WebString& constraint_name = blink::WebString());

  // Creates a MediaStreamAudioSource/MediaStreamVideoSource objects.
  // These are virtual for test purposes.
  // The caller takes ownership of the returned pointers.
  // TODO(guidou): return std::unique_ptr to make ownership clearer.
  // http://crbug.com/764293
  virtual MediaStreamAudioSource* CreateAudioSource(
      const MediaStreamDevice& device,
      const MediaStreamSource::ConstraintsCallback& source_ready);
  virtual MediaStreamVideoSource* CreateVideoSource(
      const MediaStreamDevice& device,
      const MediaStreamSource::SourceStoppedCallback& stop_callback);

  // Intended to be used only for testing.
  const AudioCaptureSettings& AudioCaptureSettingsForTesting() const;
  const VideoCaptureSettings& VideoCaptureSettingsForTesting() const;

 private:
  class RequestInfo;
  typedef std::vector<blink::WebMediaStreamSource> LocalStreamSources;

  void OnStreamGenerated(int request_id,
                         MediaStreamRequestResult result,
                         const std::string& label,
                         const MediaStreamDevices& audio_devices,
                         const MediaStreamDevices& video_devices);

  void GotAllVideoInputFormatsForDevice(
      const blink::WebUserMediaRequest& web_request,
      const std::string& label,
      const std::string& device_id,
      const media::VideoCaptureFormats& formats);

  gfx::Size GetScreenSize();

  void OnStreamGenerationFailed(int request_id,
                                MediaStreamRequestResult result);

  bool IsCurrentRequestInfo(int request_id) const;
  bool IsCurrentRequestInfo(
      const blink::WebUserMediaRequest& web_request) const;
  void DelayedGetUserMediaRequestSucceeded(
      const blink::WebMediaStream& stream,
      blink::WebUserMediaRequest web_request);
  void DelayedGetUserMediaRequestFailed(
      blink::WebUserMediaRequest web_request,
      MediaStreamRequestResult result,
      const blink::WebString& constraint_name);

  // Called when |source| has been stopped from JavaScript.
  void OnLocalSourceStopped(const blink::WebMediaStreamSource& source);

  // Creates a WebKit representation of a stream source based on
  // |device| from the MediaStreamDispatcherHost.
  blink::WebMediaStreamSource InitializeVideoSourceObject(
      const MediaStreamDevice& device);

  blink::WebMediaStreamSource InitializeAudioSourceObject(
      const MediaStreamDevice& device,
      bool* is_pending);

  void StartTracks(const std::string& label);

  void CreateVideoTracks(
      const MediaStreamDevices& devices,
      blink::WebVector<blink::WebMediaStreamTrack>* webkit_tracks);

  void CreateAudioTracks(
      const MediaStreamDevices& devices,
      blink::WebVector<blink::WebMediaStreamTrack>* webkit_tracks);

  // Callback function triggered when all native versions of the
  // underlying media sources and tracks have been created and started.
  void OnCreateNativeTracksCompleted(const std::string& label,
                                     RequestInfo* request,
                                     MediaStreamRequestResult result,
                                     const blink::WebString& result_name);

  void OnStreamGeneratedForCancelledRequest(
      const MediaStreamDevices& audio_devices,
      const MediaStreamDevices& video_devices);

  static void OnAudioSourceStartedOnAudioThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::WeakPtr<UserMediaProcessor> weak_ptr,
      MediaStreamSource* source,
      MediaStreamRequestResult result,
      const blink::WebString& result_name);

  void OnAudioSourceStarted(MediaStreamSource* source,
                            MediaStreamRequestResult result,
                            const blink::WebString& result_name);

  void NotifyCurrentRequestInfoOfAudioSourceStarted(
      MediaStreamSource* source,
      MediaStreamRequestResult result,
      const blink::WebString& result_name);

  void DeleteAllUserMediaRequests();

  // Returns the source that use a device with |device.session_id|
  // and |device.device.id|. NULL if such source doesn't exist.
  const blink::WebMediaStreamSource* FindLocalSource(
      const MediaStreamDevice& device) const {
    return FindLocalSource(local_sources_, device);
  }
  const blink::WebMediaStreamSource* FindPendingLocalSource(
      const MediaStreamDevice& device) const {
    return FindLocalSource(pending_local_sources_, device);
  }
  const blink::WebMediaStreamSource* FindLocalSource(
      const LocalStreamSources& sources,
      const MediaStreamDevice& device) const;

  // Looks up a local source and returns it if found. If not found, prepares
  // a new WebMediaStreamSource with a NULL extraData pointer.
  blink::WebMediaStreamSource FindOrInitializeSourceObject(
      const MediaStreamDevice& device);

  // Returns true if we do find and remove the |source|.
  // Otherwise returns false.
  bool RemoveLocalSource(const blink::WebMediaStreamSource& source);

  void StopLocalSource(const blink::WebMediaStreamSource& source,
                       bool notify_dispatcher);

  const mojom::MediaStreamDispatcherHostPtr& GetMediaStreamDispatcherHost();
  const blink::mojom::MediaDevicesDispatcherHostPtr&
  GetMediaDevicesDispatcher();

  void SetupAudioInput();
  void SelectAudioDeviceSettings(
      const blink::WebUserMediaRequest& web_request,
      std::vector<blink::mojom::AudioInputDeviceCapabilitiesPtr>
          audio_input_capabilities);
  void SelectAudioSettings(
      const blink::WebUserMediaRequest& web_request,
      const std::vector<AudioDeviceCaptureCapability>& capabilities);

  void SetupVideoInput();
  void SelectVideoDeviceSettings(
      const blink::WebUserMediaRequest& web_request,
      std::vector<blink::mojom::VideoInputDeviceCapabilitiesPtr>
          video_input_capabilities);
  void FinalizeSelectVideoDeviceSettings(
      const blink::WebUserMediaRequest& web_request,
      const VideoCaptureSettings& settings);
  void SelectVideoContentSettings();

  void GenerateStreamForCurrentRequestInfo();

  // Weak ref to a PeerConnectionDependencyFactory, owned by the RenderThread.
  // It's valid for the lifetime of RenderThread.
  // TODO(xians): Remove this dependency once audio do not need it for local
  // audio.
  PeerConnectionDependencyFactory* const dependency_factory_;

  // UserMediaProcessor owns MediaStreamDeviceObserver instead of
  // RenderFrameImpl (or RenderFrameObserver) to ensure tear-down occurs in the
  // right order.
  const std::unique_ptr<MediaStreamDeviceObserver>
      media_stream_device_observer_;

  LocalStreamSources local_sources_;
  LocalStreamSources pending_local_sources_;

  mojom::MediaStreamDispatcherHostPtr dispatcher_host_;

  // UserMedia requests are processed sequentially. |current_request_info_|
  // contains the request currently being processed, if any, and
  // |pending_request_infos_| is a list of queued requests.
  std::unique_ptr<RequestInfo> current_request_info_;
  MediaDevicesDispatcherCallback media_devices_dispatcher_cb_;
  base::OnceClosure request_completed_cb_;

  RenderFrameImpl* const render_frame_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Note: This member must be the last to ensure all outstanding weak pointers
  // are invalidated first.
  base::WeakPtrFactory<UserMediaProcessor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UserMediaProcessor);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_USER_MEDIA_PROCESSOR_H_
