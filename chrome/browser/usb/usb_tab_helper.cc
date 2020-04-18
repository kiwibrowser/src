// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/usb_tab_helper.h"

#include <memory>
#include <utility>

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/usb/web_usb_permission_provider.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_features.h"
#include "device/usb/mojo/device_manager_impl.h"
#include "mojo/public/cpp/bindings/message.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/usb/web_usb_chooser_service_android.h"
#else
#include "chrome/browser/usb/web_usb_chooser_service_desktop.h"
#endif  // defined(OS_ANDROID)

using content::RenderFrameHost;
using content::WebContents;

namespace {

// The renderer performs its own feature policy checks so a request that gets
// to the browser process indicates malicous code.
const char kFeaturePolicyViolation[] =
    "Feature policy blocks access to WebUSB.";

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(UsbTabHelper);

struct FrameUsbServices {
  std::unique_ptr<WebUSBPermissionProvider> permission_provider;
#if defined(OS_ANDROID)
  std::unique_ptr<WebUsbChooserServiceAndroid> chooser_service;
#else
  std::unique_ptr<WebUsbChooserServiceDesktop> chooser_service;
#endif  // defined(OS_ANDROID)
  int device_connection_count_ = 0;
};

// static
UsbTabHelper* UsbTabHelper::GetOrCreateForWebContents(
    WebContents* web_contents) {
  UsbTabHelper* tab_helper = FromWebContents(web_contents);
  if (!tab_helper) {
    CreateForWebContents(web_contents);
    tab_helper = FromWebContents(web_contents);
  }
  return tab_helper;
}

UsbTabHelper::~UsbTabHelper() {}

void UsbTabHelper::CreateDeviceManager(
    RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::mojom::UsbDeviceManager> request) {
  if (!AllowedByFeaturePolicy(render_frame_host)) {
    mojo::ReportBadMessage(kFeaturePolicyViolation);
    return;
  }
  device::usb::DeviceManagerImpl::Create(
      GetPermissionProvider(render_frame_host), std::move(request));
}

void UsbTabHelper::CreateChooserService(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::mojom::UsbChooserService> request) {
  if (!AllowedByFeaturePolicy(render_frame_host)) {
    mojo::ReportBadMessage(kFeaturePolicyViolation);
    return;
  }
  GetChooserService(render_frame_host, std::move(request));
}

void UsbTabHelper::IncrementConnectionCount(
    RenderFrameHost* render_frame_host) {
  auto it = frame_usb_services_.find(render_frame_host);
  DCHECK(it != frame_usb_services_.end());
  it->second->device_connection_count_++;
  NotifyTabStateChanged();
}

void UsbTabHelper::DecrementConnectionCount(
    RenderFrameHost* render_frame_host) {
  auto it = frame_usb_services_.find(render_frame_host);
  DCHECK(it != frame_usb_services_.end());
  DCHECK_GT(it->second->device_connection_count_, 0);
  it->second->device_connection_count_--;
  NotifyTabStateChanged();
}

bool UsbTabHelper::IsDeviceConnected() const {
  for (const auto& map_entry : frame_usb_services_) {
    if (map_entry.second->device_connection_count_ > 0)
      return true;
  }
  return false;
}

UsbTabHelper::UsbTabHelper(WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void UsbTabHelper::RenderFrameDeleted(RenderFrameHost* render_frame_host) {
  frame_usb_services_.erase(render_frame_host);
  NotifyTabStateChanged();
}

FrameUsbServices* UsbTabHelper::GetFrameUsbService(
    content::RenderFrameHost* render_frame_host) {
  FrameUsbServicesMap::const_iterator it =
      frame_usb_services_.find(render_frame_host);
  if (it == frame_usb_services_.end()) {
    std::unique_ptr<FrameUsbServices> frame_usb_services(
        new FrameUsbServices());
    it = (frame_usb_services_.insert(
              std::make_pair(render_frame_host, std::move(frame_usb_services))))
             .first;
  }
  return it->second.get();
}

base::WeakPtr<device::usb::PermissionProvider>
UsbTabHelper::GetPermissionProvider(RenderFrameHost* render_frame_host) {
  FrameUsbServices* frame_usb_services = GetFrameUsbService(render_frame_host);
  if (!frame_usb_services->permission_provider) {
    frame_usb_services->permission_provider.reset(
        new WebUSBPermissionProvider(render_frame_host));
  }
  return frame_usb_services->permission_provider->GetWeakPtr();
}

void UsbTabHelper::GetChooserService(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<device::mojom::UsbChooserService> request) {
  FrameUsbServices* frame_usb_services = GetFrameUsbService(render_frame_host);
  if (!frame_usb_services->chooser_service) {
    frame_usb_services->chooser_service.reset(
#if defined(OS_ANDROID)
        new WebUsbChooserServiceAndroid(render_frame_host));
#else
        new WebUsbChooserServiceDesktop(render_frame_host));
#endif  // defined(OS_ANDROID)
  }
  frame_usb_services->chooser_service->Bind(std::move(request));
}

void UsbTabHelper::NotifyTabStateChanged() const {
  // TODO(https://crbug.com/601627): Implement tab indicator for Android.
#if !defined(OS_ANDROID)
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  if (browser) {
    TabStripModel* tab_strip_model = browser->tab_strip_model();
    tab_strip_model->UpdateWebContentsStateAt(
        tab_strip_model->GetIndexOfWebContents(web_contents()),
        TabChangeType::kAll);
  }
#endif
}

bool UsbTabHelper::AllowedByFeaturePolicy(
    RenderFrameHost* render_frame_host) const {
  DCHECK(WebContents::FromRenderFrameHost(render_frame_host) == web_contents());
  return render_frame_host->IsFeatureEnabled(
      blink::mojom::FeaturePolicyFeature::kUsb);
}
