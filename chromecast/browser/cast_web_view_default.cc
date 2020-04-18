// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_view_default.h"

#include <utility>

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/base/cast_features.h"
#include "chromecast/base/chromecast_switches.h"
#include "chromecast/base/metrics/cast_metrics_helper.h"
#include "chromecast/browser/cast_browser_process.h"
#include "chromecast/browser/cast_web_contents_manager.h"
#include "chromecast/browser/devtools/remote_debugging_server.h"
#include "chromecast/chromecast_buildflags.h"
#include "chromecast/public/cast_media_shlib.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ipc/ipc_message.h"
#include "net/base/net_errors.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "chromecast/browser/android/cast_web_contents_surface_helper.h"
#endif  // defined(OS_ANDROID)

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace chromecast {

namespace {

std::unique_ptr<content::WebContents> CreateWebContents(
    content::BrowserContext* browser_context,
    scoped_refptr<content::SiteInstance> site_instance) {
  CHECK(display::Screen::GetScreen());
  gfx::Size display_size =
      display::Screen::GetScreen()->GetPrimaryDisplay().size();

  content::WebContents::CreateParams create_params(browser_context, NULL);
  create_params.routing_id = MSG_ROUTING_NONE;
  create_params.initial_size = display_size;
  create_params.site_instance = site_instance;
  return content::WebContents::Create(create_params);
}

}  // namespace

CastWebViewDefault::CastWebViewDefault(
    const CreateParams& params,
    CastWebContentsManager* web_contents_manager,
    content::BrowserContext* browser_context,
    scoped_refptr<content::SiteInstance> site_instance)
    : web_contents_manager_(web_contents_manager),
      browser_context_(browser_context),
      remote_debugging_server_(
          shell::CastBrowserProcess::GetInstance()->remote_debugging_server()),
      site_instance_(std::move(site_instance)),
      delegate_(params.delegate),
      transparent_(params.transparent),
      allow_media_access_(params.allow_media_access),
      enabled_for_dev_(params.enabled_for_dev),
      web_contents_(CreateWebContents(browser_context_, site_instance_)),
      window_(shell::CastContentWindow::Create(params.delegate,
                                               params.is_headless,
                                               params.enable_touch_input)),
      did_start_navigation_(false) {
  DCHECK(delegate_);
  DCHECK(web_contents_manager_);
  DCHECK(browser_context_);
  DCHECK(window_);
  content::WebContentsObserver::Observe(web_contents_.get());
  web_contents_->SetDelegate(this);

#if BUILDFLAG(IS_ANDROID_THINGS)
// Configure the ducking multiplier for AThings speakers. When CMA backend is
// used we don't want the Chromium MediaSession to duck since we are doing
// our own ducking. When no CMA backend is used we rely on the MediaSession
// for ducking. In that case set it to a proper value to match the ducking
// done in CMA backend.
#if BUILDFLAG(IS_CAST_USING_CMA_BACKEND)
  // passthrough, i.e., disable ducking
  constexpr double kDuckingMultiplier = 1.0;
#else
  // duck by -30dB
  constexpr double kDuckingMultiplier = 0.03;
#endif
  content::MediaSession::Get(web_contents_.get())
      ->SetDuckingVolumeMultiplier(kDuckingMultiplier);
#endif

  // If this CastWebView is enabled for development, start the remote debugger.
  if (enabled_for_dev_) {
    LOG(INFO) << "Enabling dev console for " << web_contents_->GetVisibleURL();
    remote_debugging_server_->EnableWebContentsForDebugging(
        web_contents_.get());
  }
}

CastWebViewDefault::~CastWebViewDefault() {}

shell::CastContentWindow* CastWebViewDefault::window() const {
  return window_.get();
}

content::WebContents* CastWebViewDefault::web_contents() const {
  return web_contents_.get();
}

void CastWebViewDefault::LoadUrl(GURL url) {
  web_contents_->GetController().LoadURL(url, content::Referrer(),
                                         ui::PAGE_TRANSITION_TYPED, "");
}

void CastWebViewDefault::ClosePage(const base::TimeDelta& shutdown_delay) {
  shutdown_delay_ = shutdown_delay;
  content::WebContentsObserver::Observe(nullptr);
  web_contents_->DispatchBeforeUnload();
  web_contents_->ClosePage();
}

void CastWebViewDefault::CloseContents(content::WebContents* source) {
  DCHECK_EQ(source, web_contents_.get());
  window_.reset();  // Window destructor requires live web_contents on Android.
  // We need to delay the deletion of web_contents_ to give (and guarantee) the
  // renderer enough time to finish 'onunload' handler (but we don't want to
  // wait any longer than that to delay the starting of next app).
  web_contents_manager_->DelayWebContentsDeletion(std::move(web_contents_),
                                                  shutdown_delay_);
  delegate_->OnPageStopped(net::OK);
}

void CastWebViewDefault::InitializeWindow(CastWindowManager* window_manager,
                                          bool is_visible,
                                          CastWindowManager::WindowId z_order,
                                          VisibilityPriority initial_priority) {
  if (media::CastMediaShlib::ClearVideoPlaneImage) {
    media::CastMediaShlib::ClearVideoPlaneImage();
  }

  DCHECK(window_manager);
  window_->CreateWindowForWebContents(web_contents_.get(), window_manager,
                                      is_visible, z_order, initial_priority);
  web_contents_->Focus();
}

content::WebContents* CastWebViewDefault::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  LOG(INFO) << "Change url: " << params.url;
  // If source is NULL which means current tab, use web_contents_ of this class.
  if (!source)
    source = web_contents_.get();
  DCHECK_EQ(source, web_contents_.get());
  // We don't want to create another web_contents. Load url only when source is
  // specified.
  source->GetController().LoadURL(params.url, params.referrer,
                                  params.transition, params.extra_headers);
  return source;
}

void CastWebViewDefault::LoadingStateChanged(content::WebContents* source,
                                             bool to_different_document) {
  delegate_->OnLoadingStateChanged(source->IsLoading());
}

void CastWebViewDefault::ActivateContents(content::WebContents* contents) {
  DCHECK_EQ(contents, web_contents_.get());
  contents->GetRenderViewHost()->GetWidget()->Focus();
}

bool CastWebViewDefault::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type) {
  if (!chromecast::IsFeatureEnabled(kAllowUserMediaAccess) &&
      !allow_media_access_) {
    LOG(WARNING) << __func__ << ": media access is disabled.";
    return false;
  }
  return true;
}

bool CastWebViewDefault::DidAddMessageToConsole(
    content::WebContents* source,
    int32_t level,
    const base::string16& message,
    int32_t line_no,
    const base::string16& source_id) {
  return delegate_->OnAddMessageToConsoleReceived(source, level, message,
                                                  line_no, source_id);
}

const content::MediaStreamDevice* GetRequestedDeviceOrDefault(
    const content::MediaStreamDevices& devices,
    const std::string& requested_device_id) {
  if (!requested_device_id.empty()) {
    auto it = std::find_if(
        devices.begin(), devices.end(),
        [requested_device_id](const content::MediaStreamDevice& device) {
          return device.id == requested_device_id;
        });
    return it != devices.end() ? &(*it) : nullptr;
  }

  if (!devices.empty())
    return &devices[0];

  return nullptr;
}

void CastWebViewDefault::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  if (!chromecast::IsFeatureEnabled(kAllowUserMediaAccess) &&
      !allow_media_access_) {
    LOG(WARNING) << __func__ << ": media access is disabled.";
    callback.Run(content::MediaStreamDevices(),
                 content::MEDIA_DEVICE_NOT_SUPPORTED,
                 std::unique_ptr<content::MediaStreamUI>());
    return;
  }

  auto audio_devices =
      content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
  auto video_devices =
      content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
  VLOG(2) << __func__ << " audio_devices=" << audio_devices.size()
          << " video_devices=" << video_devices.size();

  content::MediaStreamDevices devices;
  if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
    const content::MediaStreamDevice* device = GetRequestedDeviceOrDefault(
        audio_devices, request.requested_audio_device_id);
    if (device) {
      VLOG(1) << __func__ << "Using audio device: id=" << device->id
              << " name=" << device->name;
      devices.push_back(*device);
    }
  }

  if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
    const content::MediaStreamDevice* device = GetRequestedDeviceOrDefault(
        video_devices, request.requested_video_device_id);
    if (device) {
      VLOG(1) << __func__ << "Using video device: id=" << device->id
              << " name=" << device->name;
      devices.push_back(*device);
    }
  }

  callback.Run(devices, content::MEDIA_DEVICE_OK,
               std::unique_ptr<content::MediaStreamUI>());
}

std::unique_ptr<content::BluetoothChooser>
CastWebViewDefault::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  auto chooser = delegate_->RunBluetoothChooser(frame, event_handler);
  return chooser
             ? std::move(chooser)
             : WebContentsDelegate::RunBluetoothChooser(frame, event_handler);
}

#if defined(OS_ANDROID)
base::android::ScopedJavaLocalRef<jobject>
CastWebViewDefault::GetContentVideoViewEmbedder() {
  DCHECK(web_contents_);
  auto* helper = shell::CastWebContentsSurfaceHelper::Get(web_contents_.get());
  return helper->GetContentVideoViewEmbedder();
}
#endif  // defined(OS_ANDROID)

void CastWebViewDefault::RenderProcessGone(base::TerminationStatus status) {
  LOG(INFO) << "APP_ERROR_CHILD_PROCESS_CRASHED";
  delegate_->OnPageStopped(net::ERR_UNEXPECTED);
}

void CastWebViewDefault::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  content::RenderWidgetHostView* view =
      render_view_host->GetWidget()->GetView();
  if (view) {
    view->SetBackgroundColor(
        transparent_ ? SK_ColorTRANSPARENT
                     : chromecast::GetSwitchValueColor(
                           switches::kCastAppBackgroundColor, SK_ColorBLACK));
  }
}

void CastWebViewDefault::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // If the navigation was not committed, it means either the page was a
  // download or error 204/205, or the navigation never left the previous
  // URL. Ignore these navigations.
  if (!navigation_handle->HasCommitted()) {
    LOG(WARNING) << "Navigation did not commit: url="
                 << navigation_handle->GetURL();
    return;
  }

  net::Error error_code = navigation_handle->GetNetErrorCode();
  if (!navigation_handle->IsErrorPage())
    return;

  // If we abort errors in an iframe, it can create a really confusing
  // and fragile user experience.  Rather than create a list of errors
  // that are most likely to occur, we ignore all of them for now.
  if (!navigation_handle->IsInMainFrame()) {
    LOG(ERROR) << "Got error on sub-iframe: url=" << navigation_handle->GetURL()
               << ", error=" << error_code
               << ", description=" << net::ErrorToShortString(error_code);
    return;
  }

  LOG(ERROR) << "Got error on navigation: url=" << navigation_handle->GetURL()
             << ", error_code=" << error_code
             << ", description= " << net::ErrorToShortString(error_code);
  delegate_->OnPageStopped(error_code);
}

void CastWebViewDefault::DidFailLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description) {
  // Only report an error if we are the main frame.  See b/8433611.
  if (render_frame_host->GetParent()) {
    LOG(ERROR) << "Got error on sub-iframe: url=" << validated_url.spec()
               << ", error=" << error_code;
  } else if (error_code == net::ERR_ABORTED) {
    // ERR_ABORTED means download was aborted by the app, typically this happens
    // when flinging URL for direct playback, the initial URLRequest gets
    // cancelled/aborted and then the same URL is requested via the buffered
    // data source for media::Pipeline playback.
    LOG(INFO) << "Load canceled: url=" << validated_url.spec();
  } else {
    LOG(ERROR) << "Got error on load: url=" << validated_url.spec()
               << ", error_code=" << error_code;
    delegate_->OnPageStopped(error_code);
  }
}

void CastWebViewDefault::DidFirstVisuallyNonEmptyPaint() {
  metrics::CastMetricsHelper::GetInstance()->LogTimeToFirstPaint();
}

void CastWebViewDefault::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (did_start_navigation_) {
    return;
  }
  did_start_navigation_ = true;

#if defined(USE_AURA)
  // Resize window
  gfx::Size display_size =
      display::Screen::GetScreen()->GetPrimaryDisplay().size();
  aura::Window* content_window = web_contents()->GetNativeView();
  content_window->SetBounds(
      gfx::Rect(display_size.width(), display_size.height()));
#endif
}

void CastWebViewDefault::MediaStartedPlaying(const MediaPlayerInfo& media_info,
                                             const MediaPlayerId& id) {
  metrics::CastMetricsHelper::GetInstance()->LogMediaPlay();
}

void CastWebViewDefault::MediaStoppedPlaying(
    const MediaPlayerInfo& media_info,
    const MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  metrics::CastMetricsHelper::GetInstance()->LogMediaPause();
}

}  // namespace chromecast
