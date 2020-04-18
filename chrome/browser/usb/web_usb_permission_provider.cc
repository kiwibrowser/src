// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/web_usb_permission_provider.h"

#include <stddef.h>
#include <utility>

#include "base/stl_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/usb/usb_blocklist.h"
#include "chrome/browser/usb/usb_chooser_context.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/browser/usb/usb_tab_helper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/usb/usb_device.h"

using content::RenderFrameHost;
using content::WebContents;

// static
bool WebUSBPermissionProvider::HasDevicePermission(
    UsbChooserContext* chooser_context,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    scoped_refptr<const device::UsbDevice> device) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (UsbBlocklist::Get().IsExcluded(device))
    return false;

  return chooser_context->HasDevicePermission(requesting_origin,
                                              embedding_origin, device);
}

WebUSBPermissionProvider::WebUSBPermissionProvider(
    RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(render_frame_host_);
}

WebUSBPermissionProvider::~WebUSBPermissionProvider() {}

base::WeakPtr<device::usb::PermissionProvider>
WebUSBPermissionProvider::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool WebUSBPermissionProvider::HasDevicePermission(
    scoped_refptr<const device::UsbDevice> device) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host_);
  RenderFrameHost* main_frame = web_contents->GetMainFrame();
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  return HasDevicePermission(
      UsbChooserContextFactory::GetForProfile(profile),
      render_frame_host_->GetLastCommittedURL().GetOrigin(),
      main_frame->GetLastCommittedURL().GetOrigin(), device);
}

void WebUSBPermissionProvider::IncrementConnectionCount() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host_);
  UsbTabHelper* tab_helper = UsbTabHelper::FromWebContents(web_contents);
  tab_helper->IncrementConnectionCount(render_frame_host_);
}

void WebUSBPermissionProvider::DecrementConnectionCount() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host_);
  UsbTabHelper* tab_helper = UsbTabHelper::FromWebContents(web_contents);
  tab_helper->DecrementConnectionCount(render_frame_host_);
}
