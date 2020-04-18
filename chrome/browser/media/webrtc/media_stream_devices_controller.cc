// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/media_stream_devices_controller.h"

#include <algorithm>
#include <utility>

#include "base/callback_helpers.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/media/webrtc/media_stream_device_permissions.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/permissions/permission_result.h"
#include "chrome/browser/permissions/permission_uma_util.h"
#include "chrome/browser/permissions/permission_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_features.h"
#include "content/public/common/media_stream_request.h"
#include "content/public/common/origin_util.h"
#include "extensions/common/constants.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"

#if defined(OS_ANDROID)
#include <vector>

#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/android/preferences/pref_service_bridge.h"
#include "chrome/browser/permissions/permission_dialog_delegate.h"
#include "chrome/browser/permissions/permission_update_infobar_delegate_android.h"
#include "ui/android/window_android.h"
#else  // !defined(OS_ANDROID)
#include "components/vector_icons/vector_icons.h"
#endif

using content::BrowserThread;

namespace {

const char kPepperMediaFeaturePolicyDeprecationMessage[] =
    "Microphone and camera usage in cross-origin iframes is deprecated and "
    "will be disabled in M64 (around January 2018). To continue to use this "
    "feature, it must be enabled by the embedding document using Feature "
    "Policy, e.g. <iframe allow=\"microphone; camera;\" ...>. See "
    "https://goo.gl/EuHzyv for more details.";

// Returns true if the given ContentSettingsType is being requested in
// |request|.
bool ContentTypeIsRequested(ContentSettingsType type,
                            const content::MediaStreamRequest& request) {
  if (request.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY)
    return true;

  if (type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC)
    return request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE;

  if (type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA)
    return request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE;

  return false;
}

bool HasAvailableDevices(ContentSettingsType content_type,
                         const std::string& device_id) {
  const content::MediaStreamDevices* devices = nullptr;
  if (content_type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC) {
    devices =
        &MediaCaptureDevicesDispatcher::GetInstance()->GetAudioCaptureDevices();
  } else if (content_type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA) {
    devices =
        &MediaCaptureDevicesDispatcher::GetInstance()->GetVideoCaptureDevices();
  } else {
    NOTREACHED();
  }

  // TODO(tommi): It's kind of strange to have this here since if we fail this
  // test, there'll be a UI shown that indicates to the user that access to
  // non-existing audio/video devices has been denied.  The user won't have
  // any way to change that but there will be a UI shown which indicates that
  // access is blocked.
  if (devices->empty())
    return false;

  // Note: we check device_id before dereferencing devices. If the requested
  // device id is non-empty, then the corresponding device list must not be
  // NULL.
  if (!device_id.empty()) {
    auto it =
        std::find_if(devices->begin(), devices->end(),
                     [device_id](const content::MediaStreamDevice& device) {
                       return device.id == device_id;
                     });
    if (it == devices->end())
      return false;
  }

  return true;
}

}  // namespace

// static
void MediaStreamDevicesController::RequestPermissions(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request.render_process_id, request.render_frame_id);
  // The RFH may have been destroyed by the time the request is processed.
  if (!rfh) {
    callback.Run(content::MediaStreamDevices(),
                 content::MEDIA_DEVICE_FAILED_DUE_TO_SHUTDOWN,
                 std::unique_ptr<content::MediaStreamUI>());
    return;
  }
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  std::unique_ptr<MediaStreamDevicesController> controller(
      new MediaStreamDevicesController(web_contents, request, callback));

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  std::vector<ContentSettingsType> content_settings_types;

  PermissionManager* permission_manager = PermissionManager::Get(profile);
  bool will_prompt_for_audio = false;
  bool will_prompt_for_video = false;

  if (controller->ShouldRequestAudio()) {
    content_settings_types.push_back(CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC);
    will_prompt_for_audio =
        permission_manager->GetPermissionStatusForFrame(
                                CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
                                rfh,
                                request.security_origin).content_setting ==
        CONTENT_SETTING_ASK;
  }
  if (controller->ShouldRequestVideo()) {
    content_settings_types.push_back(CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
    will_prompt_for_video =
        permission_manager->GetPermissionStatusForFrame(
                                CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                                rfh,
                                request.security_origin).content_setting ==
        CONTENT_SETTING_ASK;
  }

  permission_manager->RequestPermissions(
      content_settings_types, rfh, request.security_origin,
      request.user_gesture,
      base::Bind(
          &MediaStreamDevicesController::RequestAndroidPermissionsIfNeeded,
          web_contents, base::Passed(&controller), will_prompt_for_audio,
          will_prompt_for_video));
}

void MediaStreamDevicesController::RequestAndroidPermissionsIfNeeded(
    content::WebContents* web_contents,
    std::unique_ptr<MediaStreamDevicesController> controller,
    bool did_prompt_for_audio,
    bool did_prompt_for_video,
    const std::vector<ContentSetting>& responses) {
#if defined(OS_ANDROID)
  // If either audio or video was previously allowed and Chrome no longer has
  // the necessary permissions, show a infobar to attempt to address this
  // mismatch.
  std::vector<ContentSettingsType> content_settings_types;
  // The audio setting will always be the first one in the vector, if it was
  // requested.
  // If the user was already prompted for mic (|did_prompt_for_audio| flag), we
  // would have requested Android permission at that point.
  if (!did_prompt_for_audio &&
      controller->ShouldRequestAudio() &&
      responses.front() == CONTENT_SETTING_ALLOW) {
    content_settings_types.push_back(CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC);
  }

  // If the user was already prompted for camera (|did_prompt_for_video| flag),
  // we would have requested Android permission at that point.
  if (!did_prompt_for_video &&
      controller->ShouldRequestVideo() &&
      responses.back() == CONTENT_SETTING_ALLOW) {
    content_settings_types.push_back(CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
  }
  if (content_settings_types.empty()) {
    controller->PromptAnsweredGroupedRequest(responses);
    return;
  }

  if (PermissionUpdateInfoBarDelegate::ShouldShowPermissionInfobar(
          web_contents, content_settings_types)) {
    PermissionUpdateInfoBarDelegate::Create(
        web_contents, content_settings_types,
        base::Bind(&MediaStreamDevicesController::AndroidOSPromptAnswered,
                   base::Passed(&controller), responses));
  } else {
    // TODO(raymes): We can get here for 2 reasons: (1) android permission has
    // already been granted, and (2) we can't get a handle to WindowAndroid.
    // In case (2) this will actually result in success being reported even
    // when the Android permission isn't present. crbug.com/775372.
    controller->PromptAnsweredGroupedRequest(responses);
  }
#else
  controller->PromptAnsweredGroupedRequest(responses);
#endif
}

#if defined(OS_ANDROID)
// static
void MediaStreamDevicesController::AndroidOSPromptAnswered(
    std::unique_ptr<MediaStreamDevicesController> controller,
    std::vector<ContentSetting> responses,
    bool android_prompt_granted) {
  if (!android_prompt_granted) {
    // Only permissions that were previously ALLOW for a site will have had
    // their android permissions requested. It's only in that case that we need
    // to change the setting to BLOCK to reflect that it wasn't allowed.
    for (size_t i = 0; i < responses.size(); ++i) {
      if (responses[i] == CONTENT_SETTING_ALLOW)
        responses[i] = CONTENT_SETTING_BLOCK;
    }
  }

  controller->PromptAnsweredGroupedRequest(responses);
}
#endif  // defined(OS_ANDROID)

// static
void MediaStreamDevicesController::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* prefs) {
  prefs->RegisterBooleanPref(prefs::kVideoCaptureAllowed, true);
  prefs->RegisterBooleanPref(prefs::kAudioCaptureAllowed, true);
  prefs->RegisterListPref(prefs::kVideoCaptureAllowedUrls);
  prefs->RegisterListPref(prefs::kAudioCaptureAllowedUrls);
}

MediaStreamDevicesController::~MediaStreamDevicesController() {
  if (!callback_.is_null()) {
    callback_.Run(content::MediaStreamDevices(),
                  content::MEDIA_DEVICE_FAILED_DUE_TO_SHUTDOWN,
                  std::unique_ptr<content::MediaStreamUI>());
  }
}

void MediaStreamDevicesController::PromptAnsweredGroupedRequest(
    const std::vector<ContentSetting>& responses) {
  // The audio setting will always be the first one in the vector, if it was
  // requested.
  bool blocked_by_feature_policy = ShouldRequestAudio() || ShouldRequestVideo();
  if (ShouldRequestAudio()) {
    audio_setting_ = responses.front();
    blocked_by_feature_policy &=
        audio_setting_ == CONTENT_SETTING_BLOCK &&
        PermissionIsBlockedForReason(CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
                                     PermissionStatusSource::FEATURE_POLICY);
  }

  if (ShouldRequestVideo()) {
    video_setting_ = responses.back();
    blocked_by_feature_policy &=
        video_setting_ == CONTENT_SETTING_BLOCK &&
        PermissionIsBlockedForReason(CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                                     PermissionStatusSource::FEATURE_POLICY);
  }

  for (ContentSetting response : responses) {
    if (response == CONTENT_SETTING_BLOCK)
      denial_reason_ = content::MEDIA_DEVICE_PERMISSION_DENIED;
    else if (response == CONTENT_SETTING_ASK)
      denial_reason_ = content::MEDIA_DEVICE_PERMISSION_DISMISSED;
  }

  RunCallback(blocked_by_feature_policy);
}

MediaStreamDevicesController::MediaStreamDevicesController(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : web_contents_(web_contents), request_(request), callback_(callback) {
  DCHECK(content::IsOriginSecure(request_.security_origin) ||
         request_.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY);

  profile_ = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  content_settings_ = TabSpecificContentSettings::FromWebContents(web_contents);

  denial_reason_ = content::MEDIA_DEVICE_OK;
  audio_setting_ = GetContentSetting(CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
                                     request, &denial_reason_);
  video_setting_ = GetContentSetting(CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                                     request, &denial_reason_);

  // Log a deprecation warning for pepper requests made when a feature policy is
  // in place. Other types of requests (namely getUserMedia requests) have a
  // deprecation warning logged in blink. Only do this if
  // kUseFeaturePolicyForPermissions isn't yet enabled. When it is enabled, we
  // log an error in PermissionContextBase as a part of the request.
  if (request_.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY &&
      !base::FeatureList::IsEnabled(
          features::kUseFeaturePolicyForPermissions)) {
    DCHECK_NE(CONTENT_SETTING_DEFAULT, audio_setting_);
    DCHECK_NE(CONTENT_SETTING_DEFAULT, video_setting_);
    content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
        request.render_process_id, request.render_frame_id);
    if (!rfh->IsFeatureEnabled(
            blink::mojom::FeaturePolicyFeature::kMicrophone) ||
        !rfh->IsFeatureEnabled(blink::mojom::FeaturePolicyFeature::kCamera)) {
      rfh->AddMessageToConsole(content::CONSOLE_MESSAGE_LEVEL_WARNING,
                               kPepperMediaFeaturePolicyDeprecationMessage);
    }
  }
}

bool MediaStreamDevicesController::ShouldRequestAudio() const {
  return audio_setting_ == CONTENT_SETTING_ASK;
}

bool MediaStreamDevicesController::ShouldRequestVideo() const {
  return video_setting_ == CONTENT_SETTING_ASK;
}

content::MediaStreamDevices MediaStreamDevicesController::GetDevices(
    ContentSetting audio_setting,
    ContentSetting video_setting) {
  bool audio_allowed = audio_setting == CONTENT_SETTING_ALLOW;
  bool video_allowed = video_setting == CONTENT_SETTING_ALLOW;

  if (!audio_allowed && !video_allowed)
    return content::MediaStreamDevices();

  content::MediaStreamDevices devices;
  switch (request_.request_type) {
    case content::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
      const content::MediaStreamDevice* device = NULL;
      // For open device request, when requested device_id is empty, pick
      // the first available of the given type. If requested device_id is
      // not empty, return the desired device if it's available. Otherwise,
      // return no device.
      if (audio_allowed &&
          request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
        DCHECK_EQ(content::MEDIA_NO_SERVICE, request_.video_type);
        if (!request_.requested_audio_device_id.empty()) {
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedAudioDevice(request_.requested_audio_device_id);
        } else {
          device = MediaCaptureDevicesDispatcher::GetInstance()
                       ->GetFirstAvailableAudioDevice();
        }
      } else if (video_allowed &&
                 request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
        DCHECK_EQ(content::MEDIA_NO_SERVICE, request_.audio_type);
        // Pepper API opens only one device at a time.
        if (!request_.requested_video_device_id.empty()) {
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedVideoDevice(request_.requested_video_device_id);
        } else {
          device = MediaCaptureDevicesDispatcher::GetInstance()
                       ->GetFirstAvailableVideoDevice();
        }
      }
      if (device)
        devices.push_back(*device);
      break;
    }
    case content::MEDIA_GENERATE_STREAM: {
      bool get_default_audio_device = audio_allowed;
      bool get_default_video_device = video_allowed;

      // Get the exact audio or video device if an id is specified.
      if (audio_allowed && !request_.requested_audio_device_id.empty()) {
        const content::MediaStreamDevice* audio_device =
            MediaCaptureDevicesDispatcher::GetInstance()
                ->GetRequestedAudioDevice(request_.requested_audio_device_id);
        if (audio_device) {
          devices.push_back(*audio_device);
          get_default_audio_device = false;
        }
      }
      if (video_allowed && !request_.requested_video_device_id.empty()) {
        const content::MediaStreamDevice* video_device =
            MediaCaptureDevicesDispatcher::GetInstance()
                ->GetRequestedVideoDevice(request_.requested_video_device_id);
        if (video_device) {
          devices.push_back(*video_device);
          get_default_video_device = false;
        }
      }

      // If either or both audio and video devices were requested but not
      // specified by id, get the default devices.
      if (get_default_audio_device || get_default_video_device) {
        MediaCaptureDevicesDispatcher::GetInstance()
            ->GetDefaultDevicesForProfile(profile_, get_default_audio_device,
                                          get_default_video_device, &devices);
      }
      break;
    }
    case content::MEDIA_DEVICE_ACCESS: {
      // Get the default devices for the request.
      MediaCaptureDevicesDispatcher::GetInstance()->GetDefaultDevicesForProfile(
          profile_, audio_allowed, video_allowed, &devices);
      break;
    }
  }  // switch

  return devices;
}

void MediaStreamDevicesController::RunCallback(bool blocked_by_feature_policy) {
  CHECK(!callback_.is_null());

  // If the kill switch is, or the request was blocked because of feature
  // policy we don't update the tab context.
  if (denial_reason_ != content::MEDIA_DEVICE_KILL_SWITCH_ON &&
      !blocked_by_feature_policy) {
    UpdateTabSpecificContentSettings(audio_setting_, video_setting_);
  }

  content::MediaStreamDevices devices =
      GetDevices(audio_setting_, video_setting_);

  // If either audio or video are allowed then the callback should report
  // success, otherwise we report |denial_reason_|.
  content::MediaStreamRequestResult request_result = content::MEDIA_DEVICE_OK;
  if (audio_setting_ != CONTENT_SETTING_ALLOW &&
      video_setting_ != CONTENT_SETTING_ALLOW) {
    DCHECK_NE(content::MEDIA_DEVICE_OK, denial_reason_);
    request_result = denial_reason_;
  } else if (devices.empty()) {
    // Even if one of the content settings was allowed, if there are no devices
    // at this point we still report a failure.
    request_result = content::MEDIA_DEVICE_NO_HARDWARE;
  }

  std::unique_ptr<content::MediaStreamUI> ui;
  if (!devices.empty()) {
    ui = MediaCaptureDevicesDispatcher::GetInstance()
             ->GetMediaStreamCaptureIndicator()
             ->RegisterMediaStream(web_contents_, devices);
  }
  base::ResetAndReturn(&callback_).Run(devices, request_result, std::move(ui));
}

void MediaStreamDevicesController::UpdateTabSpecificContentSettings(
    ContentSetting audio_setting,
    ContentSetting video_setting) const {
  if (!content_settings_)
    return;

  TabSpecificContentSettings::MicrophoneCameraState microphone_camera_state =
      TabSpecificContentSettings::MICROPHONE_CAMERA_NOT_ACCESSED;
  std::string selected_audio_device;
  std::string selected_video_device;
  std::string requested_audio_device = request_.requested_audio_device_id;
  std::string requested_video_device = request_.requested_video_device_id;

  // TODO(raymes): Why do we use the defaults here for the selected devices?
  // Shouldn't we just use the devices that were actually selected?
  PrefService* prefs = Profile::FromBrowserContext(
                           web_contents_->GetBrowserContext())->GetPrefs();
  if (audio_setting != CONTENT_SETTING_DEFAULT) {
    selected_audio_device =
        requested_audio_device.empty()
            ? prefs->GetString(prefs::kDefaultAudioCaptureDevice)
            : requested_audio_device;
    microphone_camera_state |=
        TabSpecificContentSettings::MICROPHONE_ACCESSED |
        (audio_setting == CONTENT_SETTING_ALLOW
             ? 0
             : TabSpecificContentSettings::MICROPHONE_BLOCKED);
  }

  if (video_setting != CONTENT_SETTING_DEFAULT) {
    selected_video_device =
        requested_video_device.empty()
            ? prefs->GetString(prefs::kDefaultVideoCaptureDevice)
            : requested_video_device;
    microphone_camera_state |=
        TabSpecificContentSettings::CAMERA_ACCESSED |
        (video_setting == CONTENT_SETTING_ALLOW
             ? 0
             : TabSpecificContentSettings::CAMERA_BLOCKED);
  }

  content_settings_->OnMediaStreamPermissionSet(
      PermissionManager::Get(profile_)->GetCanonicalOrigin(
          request_.security_origin, web_contents_->GetLastCommittedURL()),
      microphone_camera_state, selected_audio_device, selected_video_device,
      requested_audio_device, requested_video_device);
}

ContentSetting MediaStreamDevicesController::GetContentSetting(
    ContentSettingsType content_type,
    const content::MediaStreamRequest& request,
    content::MediaStreamRequestResult* denial_reason) const {
  DCHECK(content_type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC ||
         content_type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
  DCHECK(!request_.security_origin.is_empty());
  DCHECK(content::IsOriginSecure(request_.security_origin) ||
         request_.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY);
  if (!ContentTypeIsRequested(content_type, request)) {
    // No denial reason set as it will have been previously set.
    return CONTENT_SETTING_DEFAULT;
  }

  std::string device_id;
  if (content_type == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC)
    device_id = request.requested_audio_device_id;
  else
    device_id = request.requested_video_device_id;
  if (!HasAvailableDevices(content_type, device_id)) {
    *denial_reason = content::MEDIA_DEVICE_NO_HARDWARE;
    return CONTENT_SETTING_BLOCK;
  }

  if (!IsUserAcceptAllowed(content_type)) {
    *denial_reason = content::MEDIA_DEVICE_PERMISSION_DENIED;
    return CONTENT_SETTING_BLOCK;
  }

  // Don't request if the kill switch is on.
  if (PermissionIsBlockedForReason(content_type,
                                   PermissionStatusSource::KILL_SWITCH)) {
    *denial_reason = content::MEDIA_DEVICE_KILL_SWITCH_ON;
    return CONTENT_SETTING_BLOCK;
  }

  return CONTENT_SETTING_ASK;
}

bool MediaStreamDevicesController::IsUserAcceptAllowed(
    ContentSettingsType content_type) const {
#if defined(OS_ANDROID)
  ui::WindowAndroid* window_android =
      web_contents_->GetNativeView()->GetWindowAndroid();
  if (!window_android)
    return false;

  std::vector<std::string> android_permissions;
  PrefServiceBridge::GetAndroidPermissionsForContentSetting(
      content_type, &android_permissions);
  for (const auto& android_permission : android_permissions) {
    if (!window_android->HasPermission(android_permission) &&
        !window_android->CanRequestPermission(android_permission)) {
      return false;
    }
  }

  // Don't approve device requests if the tab was hidden.
  // TODO(qinmin): Add a test for this. http://crbug.com/396869.
  // TODO(raymes): Shouldn't this apply to all permissions not just audio/video?
  return web_contents_->GetRenderWidgetHostView()->IsShowing();
#endif
  return true;
}

bool MediaStreamDevicesController::PermissionIsBlockedForReason(
    ContentSettingsType content_type,
    PermissionStatusSource reason) const {
  // TODO(raymes): This function wouldn't be needed if
  // PermissionManager::RequestPermissions returned a denial reason.
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request_.render_process_id, request_.render_frame_id);
  PermissionResult result =
      PermissionManager::Get(profile_)->GetPermissionStatusForFrame(
          content_type, rfh, request_.security_origin);
  if (result.source == reason) {
    DCHECK_EQ(CONTENT_SETTING_BLOCK, result.content_setting);
    return true;
  }
  return false;
}
