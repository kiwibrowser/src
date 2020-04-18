// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_USB_TAB_HELPER_H_
#define CHROME_BROWSER_USB_USB_TAB_HELPER_H_

#include <map>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace device {
namespace mojom {
class UsbChooserService;
class UsbDeviceManager;
}

namespace usb {
class PermissionProvider;
}
}

struct FrameUsbServices;

typedef std::map<content::RenderFrameHost*, std::unique_ptr<FrameUsbServices>>
    FrameUsbServicesMap;

// Per-tab owner of USB services provided to render frames within that tab.
class UsbTabHelper : public content::WebContentsObserver,
                     public content::WebContentsUserData<UsbTabHelper> {
 public:
  static UsbTabHelper* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  ~UsbTabHelper() override;

  void CreateDeviceManager(
      content::RenderFrameHost* render_frame_host,
      mojo::InterfaceRequest<device::mojom::UsbDeviceManager> request);

  void CreateChooserService(
      content::RenderFrameHost* render_frame_host,
      mojo::InterfaceRequest<device::mojom::UsbChooserService> request);

  void IncrementConnectionCount(content::RenderFrameHost* render_frame_host);
  void DecrementConnectionCount(content::RenderFrameHost* render_frame_host);
  bool IsDeviceConnected() const;

 private:
  explicit UsbTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<UsbTabHelper>;

  // content::WebContentsObserver overrides:
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

  FrameUsbServices* GetFrameUsbService(
      content::RenderFrameHost* render_frame_host);

  base::WeakPtr<device::usb::PermissionProvider> GetPermissionProvider(
      content::RenderFrameHost* render_frame_host);

  void GetChooserService(
      content::RenderFrameHost* render_frame_host,
      mojo::InterfaceRequest<device::mojom::UsbChooserService> request);

  void NotifyTabStateChanged() const;

  bool AllowedByFeaturePolicy(
      content::RenderFrameHost* render_frame_host) const;

  FrameUsbServicesMap frame_usb_services_;

  DISALLOW_COPY_AND_ASSIGN(UsbTabHelper);
};

#endif  // CHROME_BROWSER_USB_USB_TAB_HELPER_H_
