// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// MediaStreamManager is used to open media capture devices (video supported
// now). Call flow:
// 1. GenerateStream is called when a render process wants to use a capture
//    device.
// 2. MediaStreamManager will ask MediaStreamUIController for permission to
//    use devices and for which device to use.
// 3. MediaStreamManager will request the corresponding media device manager(s)
//    to enumerate available devices. The result will be given to
//    MediaStreamUIController.
// 4. MediaStreamUIController will, by posting the request to UI, let the
//    users to select which devices to use and send callback to
//    MediaStreamManager with the result.
// 5. MediaStreamManager will call the proper media device manager to open the
//    device and run the corresponding callback with result.

// If either user or test harness selects --use-fake-device-for-media-stream,
// a fake video device or devices are used instead of real ones.

// When enumeration and open are done in separate operations,
// MediaStreamUIController is not involved as in steps.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_MANAGER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_MANAGER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop_current.h"
#include "base/optional.h"
#include "base/power_monitor/power_observer.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "content/browser/media/media_devices_util.h"
#include "content/browser/renderer_host/media/media_devices_manager.h"
#include "content/browser/renderer_host/media/media_stream_provider.h"
#include "content/common/content_export.h"
#include "content/common/media/media_devices.h"
#include "content/common/media/media_stream_controls.h"
#include "content/public/browser/media_request_state.h"
#include "content/public/common/media_stream_request.h"
#include "media/base/video_facing.h"

namespace media {
class AudioSystem;
}

namespace url {
class Origin;
}

namespace content {

class AudioInputDeviceManager;
class AudioServiceListener;
class FakeMediaStreamUIProxy;
class MediaStreamUIProxy;
class VideoCaptureManager;
class VideoCaptureProvider;

// MediaStreamManager is used to generate and close new media devices, not to
// start the media flow. The classes requesting new media streams are answered
// using callbacks.
class CONTENT_EXPORT MediaStreamManager
    : public MediaStreamProviderListener,
      public base::MessageLoopCurrent::DestructionObserver,
      public base::PowerObserver {
 public:
  // Callback to deliver the result of a media access request.
  using MediaAccessRequestCallback =
      base::OnceCallback<void(const MediaStreamDevices& devices,
                              std::unique_ptr<MediaStreamUIProxy> ui)>;

  using GenerateStreamCallback =
      base::OnceCallback<void(MediaStreamRequestResult result,
                              const std::string& label,
                              const MediaStreamDevices& audio_devices,
                              const MediaStreamDevices& video_devices)>;

  using OpenDeviceCallback =
      base::OnceCallback<void(bool success,
                              const std::string& label,
                              const MediaStreamDevice& device)>;

  using DeviceStoppedCallback =
      base::RepeatingCallback<void(const std::string& label,
                                   const MediaStreamDevice& device)>;

  // Callback for testing.
  using GenerateStreamTestCallback =
      base::OnceCallback<bool(const StreamControls&)>;

  // Adds |message| to native logs for outstanding device requests, for use by
  // render processes hosts whose corresponding render processes are requesting
  // logging from webrtcLoggingPrivate API. Safe to call from any thread.
  static void SendMessageToNativeLog(const std::string& message);

  // |audio_task_runner| passed to constructors is the task runner used by audio
  // system when it runs in-process; it's null if audio runs out of process.

  MediaStreamManager(
      media::AudioSystem* audio_system,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);

  // |audio_system| is required but defaults will be used if either
  // |video_capture_system| or |device_task_runner| are null.
  MediaStreamManager(
      media::AudioSystem* audio_system,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      std::unique_ptr<VideoCaptureProvider> video_capture_provider);

  ~MediaStreamManager() override;

  // Used to access VideoCaptureManager.
  VideoCaptureManager* video_capture_manager();

  // Used to access AudioInputDeviceManager.
  AudioInputDeviceManager* audio_input_device_manager();

  // Used to access AudioServiceListener, must be called on IO thread.
  AudioServiceListener* audio_service_listener();

  // Used to access MediaDevicesManager.
  MediaDevicesManager* media_devices_manager();

  // Used to access AudioSystem.
  media::AudioSystem* audio_system();

  // AddVideoCaptureObserver() and RemoveAllVideoCaptureObservers() must be
  // called after InitializeDeviceManagersOnIOThread() and before
  // WillDestroyCurrentMessageLoop(). They can be called more than once and it's
  // ok to not call at all if the client is not interested in receiving
  // media::VideoCaptureObserver callbacks.
  // The methods must be called on BrowserThread::IO threads. The callbacks of
  // media::VideoCaptureObserver also arrive on BrowserThread::IO threads.
  void AddVideoCaptureObserver(media::VideoCaptureObserver* capture_observer);
  void RemoveAllVideoCaptureObservers();

  // Creates a new media access request which is identified by a unique string
  // that's returned to the caller. This will trigger the infobar and ask users
  // for access to the device. |render_process_id| and |render_frame_id| are
  // used to determine where the infobar will appear to the user. |callback| is
  // used to send the selected device to the clients. An empty list of device
  // will be returned if the users deny the access.
  std::string MakeMediaAccessRequest(int render_process_id,
                                     int render_frame_id,
                                     int page_request_id,
                                     const StreamControls& controls,
                                     const url::Origin& security_origin,
                                     MediaAccessRequestCallback callback);

  // GenerateStream opens new media devices according to |components|.  It
  // creates a new request which is identified by a unique string that's
  // returned to the caller.  |render_process_id| and |render_frame_id| are used
  // to determine where the infobar will appear to the user. |device_stopped_cb|
  // is set to receive device stopped notifications.
  void GenerateStream(int render_process_id,
                      int render_frame_id,
                      int page_request_id,
                      const StreamControls& controls,
                      MediaDeviceSaltAndOrigin salt_and_origin,
                      bool user_gesture,
                      GenerateStreamCallback generate_stream_cb,
                      DeviceStoppedCallback device_stopped_cb);

  void CancelRequest(int render_process_id,
                     int render_frame_id,
                     int page_request_id);

  // Cancel an open request identified by |label|.
  void CancelRequest(const std::string& label);

  // Cancel all requests for the given |render_process_id| and
  // |render_frame_id|.
  void CancelAllRequests(int render_process_id, int render_frame_id);

  // Closes the stream device for a certain render frame. The stream must have
  // been opened by a call to GenerateStream.
  void StopStreamDevice(int render_process_id,
                        int render_frame_id,
                        const std::string& device_id,
                        int session_id);

  // Open a device identified by |device_id|. |type| must be either
  // MEDIA_DEVICE_AUDIO_CAPTURE or MEDIA_DEVICE_VIDEO_CAPTURE.
  // |device_stopped_cb| is set to receive device stopped notifications. The
  // request is identified using string returned to the caller.
  void OpenDevice(int render_process_id,
                  int render_frame_id,
                  int page_request_id,
                  const std::string& device_id,
                  MediaStreamType type,
                  MediaDeviceSaltAndOrigin salt_and_origin,
                  OpenDeviceCallback open_device_cb,
                  DeviceStoppedCallback device_stopped_cb);

  // Finds and returns the device id corresponding to the given
  // |source_id|. Returns true if there was a raw device id that matched the
  // given |source_id|, false if nothing matched it.
  // TODO(guidou): Update to provide a callback-based interface.
  // See http://crbug.com/648155.
  bool TranslateSourceIdToDeviceId(MediaStreamType stream_type,
                                   const std::string& salt,
                                   const url::Origin& security_origin,
                                   const std::string& source_id,
                                   std::string* device_id) const;

  // Find |device_id| in the list of |requests_|, and returns its session id,
  // or MediaStreamDevice::kNoId if not found.
  int VideoDeviceIdToSessionId(const std::string& device_id) const;

  // Called by UI to make sure the device monitor is started so that UI receive
  // notifications about device changes.
  void EnsureDeviceMonitorStarted();

  // Implements MediaStreamProviderListener.
  void Opened(MediaStreamType stream_type, int capture_session_id) override;
  void Closed(MediaStreamType stream_type, int capture_session_id) override;
  void Aborted(MediaStreamType stream_type, int capture_session_id) override;

  // Returns all devices currently opened by a request with label |label|.
  // If no request with |label| exist, an empty array is returned.
  MediaStreamDevices GetDevicesOpenedByRequest(const std::string& label) const;

  // This object gets deleted on the UI thread after the IO thread has been
  // destroyed. So we need to know when IO thread is being destroyed so that
  // we can delete VideoCaptureManager and AudioInputDeviceManager.
  // Note: In tests it is sometimes necessary to invoke this explicitly when
  // using TestBrowserThreadBundle because the notification happens too late.
  // (see http://crbug.com/247525#c14).
  void WillDestroyCurrentMessageLoop() override;

  // Sends log messages to the render process hosts whose corresponding render
  // processes are making device requests, to be used by the
  // webrtcLoggingPrivate API if requested.
  void AddLogMessageOnIOThread(const std::string& message);

  // base::PowerObserver overrides.
  void OnSuspend() override;
  void OnResume() override;

  // Called by the tests to specify a factory for creating
  // FakeMediaStreamUIProxys to be used for generated streams.
  void UseFakeUIFactoryForTests(
      base::Callback<std::unique_ptr<FakeMediaStreamUIProxy>(void)>
          fake_ui_factory);

  // Register and unregister a new callback for receiving native log entries.
  // Called on the IO thread.
  static void RegisterNativeLogCallback(
      int renderer_host_id,
      const base::Callback<void(const std::string&)>& callback);
  static void UnregisterNativeLogCallback(int renderer_host_id);

  // Generates a hash of a device's unique ID usable by one
  // particular security origin.
  static std::string GetHMACForMediaDeviceID(const std::string& salt,
                                             const url::Origin& security_origin,
                                             const std::string& raw_unique_id);

  // Convenience method to check if |device_guid| is an HMAC of
  // |raw_device_id| for |security_origin|.
  static bool DoesMediaDeviceIDMatchHMAC(const std::string& salt,
                                         const url::Origin& security_origin,
                                         const std::string& device_guid,
                                         const std::string& raw_unique_id);

  // Returns true if the renderer process identified with |render_process_id|
  // is allowed to access |origin|.
  static bool IsOriginAllowed(int render_process_id, const url::Origin& origin);

  // Set whether the capturing is secure for the capturing session with given
  // |session_id|, |render_process_id|, and the MediaStreamType |type|.
  void SetCapturingLinkSecured(int render_process_id,
                               int session_id,
                               content::MediaStreamType type,
                               bool is_secure);

  // Helper for sending up-to-date device lists to media observer when a
  // capture device is plugged in or unplugged.
  void NotifyDevicesChanged(MediaDeviceType stream_type,
                            const MediaDeviceInfoArray& devices);

  // This method is called when an audio or video device is removed. It makes
  // sure all MediaStreams that use a removed device are stopped and that the
  // render process is notified.
  void StopRemovedDevice(MediaDeviceType type,
                         const MediaDeviceInfo& media_device_info);

  void SetGenerateStreamCallbackForTesting(
      GenerateStreamTestCallback test_callback);

  // This method is called when all tracks are started.
  void OnStreamStarted(const std::string& label);

 private:
  // Contains all data needed to keep track of requests.
  class DeviceRequest;

  // |DeviceRequests| is a list to ensure requests are processed in the order
  // they arrive. The first member of the pair is the label of the
  // |DeviceRequest|.
  using LabeledDeviceRequest = std::pair<std::string, DeviceRequest*>;
  using DeviceRequests = std::list<LabeledDeviceRequest>;

  void InitializeMaybeAsync(
      std::unique_ptr<VideoCaptureProvider> video_capture_provider);

  // |output_parameters| contains real values only if the request requires it.
  void HandleAccessRequestResponse(
      const std::string& label,
      const media::AudioParameters& output_parameters,
      const MediaStreamDevices& devices,
      content::MediaStreamRequestResult result);
  void StopMediaStreamFromBrowser(const std::string& label);

  // Helpers.
  // Checks if all devices that was requested in the request identififed by
  // |label| has been opened and set the request state accordingly.
  void HandleRequestDone(const std::string& label, DeviceRequest* request);
  // Stop the use of the device associated with |session_id| of type |type| in
  // all |requests_|. The device is removed from the request. If a request
  /// doesn't use any devices as a consequence, the request is deleted.
  void StopDevice(MediaStreamType type, int session_id);
  // Calls the correct capture manager and close the device with |session_id|.
  // All requests that uses the device are updated.
  void CloseDevice(MediaStreamType type, int session_id);
  // Returns true if a request for devices has been completed and the devices
  // has either been opened or an error has occurred.
  bool RequestDone(const DeviceRequest& request) const;
  MediaStreamProvider* GetDeviceManager(MediaStreamType stream_type);
  void StartEnumeration(DeviceRequest* request, const std::string& label);
  std::string AddRequest(DeviceRequest* request);
  DeviceRequest* FindRequest(const std::string& label) const;
  void DeleteRequest(const std::string& label);
  // Prepare the request with label |label| by starting device enumeration if
  // needed.
  void SetupRequest(const std::string& label);
  // Prepare |request| of type MEDIA_DEVICE_AUDIO_CAPTURE and/or
  // MEDIA_DEVICE_VIDEO_CAPTURE for being posted to the UI by parsing
  // StreamControls for requested device IDs.
  bool SetupDeviceCaptureRequest(DeviceRequest* request,
                                 const MediaDeviceEnumeration& enumeration);
  // Prepare |request| of type MEDIA_TAB_AUDIO_CAPTURE and/or
  // MEDIA_TAB_VIDEO_CAPTURE for being posted to the UI by parsing
  // StreamControls for requested tab capture IDs.
  bool SetupTabCaptureRequest(DeviceRequest* request);
  // Prepare |request| of type MEDIA_DESKTOP_AUDIO_CAPTURE and/or
  // MEDIA_DESKTOP_VIDEO_CAPTURE for being posted to the UI by parsing
  // StreamControls for the requested desktop ID.
  bool SetupScreenCaptureRequest(DeviceRequest* request);
  // Called when a request has been setup and devices have been enumerated if
  // needed.
  void ReadOutputParamsAndPostRequestToUI(
      const std::string& label,
      DeviceRequest* request,
      const MediaDeviceEnumeration& enumeration);
  // Called when audio output parameters have been read if needed.
  void PostRequestToUI(
      const std::string& label,
      DeviceRequest* request,
      const MediaDeviceEnumeration& enumeration,
      const base::Optional<media::AudioParameters>& output_parameters);
  // Returns true if a device with |device_id| has already been requested with
  // a render procecss_id and render_frame_id and type equal to the the values
  // in |request|. If it has been requested, |device_info| contain information
  // about the device.
  bool FindExistingRequestedDevice(
      const DeviceRequest& new_request,
      const MediaStreamDevice& new_device,
      MediaStreamDevice* existing_device,
      MediaRequestState* existing_request_state) const;

  void FinalizeGenerateStream(const std::string& label, DeviceRequest* request);
  void FinalizeRequestFailed(const std::string& label,
                             DeviceRequest* request,
                             content::MediaStreamRequestResult result);
  void FinalizeOpenDevice(const std::string& label, DeviceRequest* request);
  void FinalizeMediaAccessRequest(const std::string& label,
                                  DeviceRequest* request,
                                  const MediaStreamDevices& devices);
  void HandleCheckMediaAccessResponse(const std::string& label,
                                      bool have_access);

  // Picks a device ID from a list of required and alternate device IDs,
  // presented as part of a TrackControls structure.
  // Either the required device ID is picked (if present), or the first
  // valid alternate device ID.
  // Returns false if the required device ID is present and invalid.
  // Otherwise, if no valid device is found, device_id is unchanged.
  bool PickDeviceId(const MediaDeviceSaltAndOrigin& salt_and_origin,
                    const TrackControls& controls,
                    const MediaDeviceInfoArray& devices,
                    std::string* device_id) const;

  // Finds the requested device id from request. The requested device type
  // must be MEDIA_DEVICE_AUDIO_CAPTURE or MEDIA_DEVICE_VIDEO_CAPTURE.
  bool GetRequestedDeviceCaptureId(const DeviceRequest* request,
                                   MediaStreamType type,
                                   const MediaDeviceInfoArray& devices,
                                   std::string* device_id) const;

  void TranslateDeviceIdToSourceId(DeviceRequest* request,
                                   MediaStreamDevice* device);

  // Handles the callback from MediaStreamUIProxy to receive the UI window id,
  // used for excluding the notification window in desktop capturing.
  void OnMediaStreamUIWindowId(MediaStreamType video_type,
                               const MediaStreamDevices& devices,
                               gfx::NativeViewId window_id);

  // Runs on the IO thread and does the actual [un]registration of callbacks.
  void DoNativeLogCallbackRegistration(
      int renderer_host_id,
      const base::Callback<void(const std::string&)>& callback);
  void DoNativeLogCallbackUnregistration(int renderer_host_id);

  // Callback to handle the reply to a low-level enumeration request.
  void DevicesEnumerated(bool requested_audio_input,
                         bool requested_video_input,
                         const std::string& label,
                         const MediaDeviceEnumeration& enumeration);

  // Creates MediaStreamDevices for |devices_infos| of |stream_type|. For video
  // capture device it also uses cached content from |video_capture_manager_| to
  // set the MediaStreamDevice fields.
  MediaStreamDevices ConvertToMediaStreamDevices(
      MediaStreamType stream_type,
      const MediaDeviceInfoArray& device_infos);

  media::AudioSystem* const audio_system_;  // not owned
  scoped_refptr<AudioInputDeviceManager> audio_input_device_manager_;
  scoped_refptr<VideoCaptureManager> video_capture_manager_;

  // Not initialized on Mac (if not in tests), since the main thread is used.
  // Always initialized on Windows.
  // On other platforms, initialized when no audio task runner is provided in
  // the constructor.
  base::Optional<base::Thread> video_capture_thread_;

  std::unique_ptr<MediaDevicesManager> media_devices_manager_;

  // All non-closed request. Must be accessed on IO thread.
  DeviceRequests requests_;

  base::Callback<std::unique_ptr<FakeMediaStreamUIProxy>(void)>
      fake_ui_factory_;

  // Maps render process hosts to log callbacks. Used on the IO thread.
  std::map<int, base::Callback<void(const std::string&)>> log_callbacks_;

  std::unique_ptr<AudioServiceListener> audio_service_listener_;

  GenerateStreamTestCallback generate_stream_test_callback_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_MANAGER_H_
