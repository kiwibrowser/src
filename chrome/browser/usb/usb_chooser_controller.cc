// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/usb_chooser_controller.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/net/referrer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/usb/usb_blocklist.h"
#include "chrome/browser/usb/usb_chooser_context.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/browser/usb/web_usb_histograms.h"
#include "chrome/browser/usb/web_usb_permission_provider.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/base/device_client.h"
#include "device/usb/mojo/type_converters.h"
#include "device/usb/public/cpp/filter_utils.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_ids.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(OS_ANDROID)
#include "device/usb/usb_ids.h"
#endif  // !defined(OS_ANDROID)

using content::RenderFrameHost;
using content::WebContents;
using device::UsbDevice;

namespace {

base::string16 FormatUsbDeviceName(scoped_refptr<device::UsbDevice> device) {
  base::string16 device_name = device->product_string();
  if (device_name.empty()) {
    uint16_t vendor_id = device->vendor_id();
    uint16_t product_id = device->product_id();
#if !defined(OS_ANDROID)
    if (const char* product_name =
            device::UsbIds::GetProductName(vendor_id, product_id)) {
      return base::UTF8ToUTF16(product_name);
    } else if (const char* vendor_name =
                   device::UsbIds::GetVendorName(vendor_id)) {
      return l10n_util::GetStringFUTF16(
          IDS_DEVICE_CHOOSER_DEVICE_NAME_UNKNOWN_DEVICE_WITH_VENDOR_NAME,
          base::UTF8ToUTF16(vendor_name));
    }
#endif  // !defined(OS_ANDROID)
    device_name = l10n_util::GetStringFUTF16(
        IDS_DEVICE_CHOOSER_DEVICE_NAME_UNKNOWN_DEVICE_WITH_VENDOR_ID_AND_PRODUCT_ID,
        base::ASCIIToUTF16(base::StringPrintf("%04x", vendor_id)),
        base::ASCIIToUTF16(base::StringPrintf("%04x", product_id)));
  }

  return device_name;
}

}  // namespace

UsbChooserController::UsbChooserController(
    RenderFrameHost* render_frame_host,
    std::vector<device::mojom::UsbDeviceFilterPtr> device_filters,
    device::mojom::UsbChooserService::GetPermissionCallback callback)
    : ChooserController(render_frame_host,
                        IDS_USB_DEVICE_CHOOSER_PROMPT_ORIGIN,
                        IDS_USB_DEVICE_CHOOSER_PROMPT_EXTENSION_NAME),
      filters_(std::move(device_filters)),
      callback_(std::move(callback)),
      web_contents_(WebContents::FromRenderFrameHost(render_frame_host)),
      usb_service_observer_(this),
      weak_factory_(this) {
  device::UsbService* usb_service =
      device::DeviceClient::Get()->GetUsbService();
  if (usb_service) {
    usb_service_observer_.Add(usb_service);
    usb_service->GetDevices(base::Bind(&UsbChooserController::GotUsbDeviceList,
                                       weak_factory_.GetWeakPtr()));
  }

  RenderFrameHost* main_frame = web_contents_->GetMainFrame();
  requesting_origin_ = render_frame_host->GetLastCommittedURL().GetOrigin();
  embedding_origin_ = main_frame->GetLastCommittedURL().GetOrigin();
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  chooser_context_ =
      UsbChooserContextFactory::GetForProfile(profile)->AsWeakPtr();
}

UsbChooserController::~UsbChooserController() {
  if (!callback_.is_null())
    std::move(callback_).Run(nullptr);
}

base::string16 UsbChooserController::GetNoOptionsText() const {
  return l10n_util::GetStringUTF16(IDS_DEVICE_CHOOSER_NO_DEVICES_FOUND_PROMPT);
}

base::string16 UsbChooserController::GetOkButtonLabel() const {
  return l10n_util::GetStringUTF16(IDS_USB_DEVICE_CHOOSER_CONNECT_BUTTON_TEXT);
}

size_t UsbChooserController::NumOptions() const {
  return devices_.size();
}

base::string16 UsbChooserController::GetOption(size_t index) const {
  DCHECK_LT(index, devices_.size());
  const base::string16& device_name = devices_[index].second;
  const auto& it = device_name_map_.find(device_name);
  DCHECK(it != device_name_map_.end());
  return it->second == 1
             ? device_name
             : l10n_util::GetStringFUTF16(
                   IDS_DEVICE_CHOOSER_DEVICE_NAME_WITH_ID, device_name,
                   devices_[index].first->serial_number());
}

bool UsbChooserController::IsPaired(size_t index) const {
  scoped_refptr<UsbDevice> device = devices_[index].first;

  if (!chooser_context_)
    return false;

  return WebUSBPermissionProvider::HasDevicePermission(
      chooser_context_.get(), requesting_origin_, embedding_origin_, device);
}

void UsbChooserController::Select(const std::vector<size_t>& indices) {
  DCHECK_EQ(1u, indices.size());
  size_t index = indices[0];
  DCHECK_LT(index, devices_.size());

  if (chooser_context_) {
    chooser_context_->GrantDevicePermission(
        requesting_origin_, embedding_origin_, devices_[index].first->guid());
  }

  std::move(callback_).Run(
      device::mojom::UsbDeviceInfo::From(*devices_[index].first));

  RecordWebUsbChooserClosure(
      devices_[index].first->serial_number().empty()
          ? WEBUSB_CHOOSER_CLOSED_EPHEMERAL_PERMISSION_GRANTED
          : WEBUSB_CHOOSER_CLOSED_PERMISSION_GRANTED);
}

void UsbChooserController::Cancel() {
  RecordWebUsbChooserClosure(devices_.size() == 0
                                 ? WEBUSB_CHOOSER_CLOSED_CANCELLED_NO_DEVICES
                                 : WEBUSB_CHOOSER_CLOSED_CANCELLED);
}

void UsbChooserController::Close() {}

void UsbChooserController::OpenHelpCenterUrl() const {
  web_contents_->OpenURL(content::OpenURLParams(
      GURL(chrome::kChooserUsbOverviewURL), content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false /* is_renderer_initialized */));
}

void UsbChooserController::OnDeviceAdded(scoped_refptr<UsbDevice> device) {
  if (DisplayDevice(device)) {
    base::string16 device_name = FormatUsbDeviceName(device);
    devices_.push_back(std::make_pair(device, device_name));
    ++device_name_map_[device_name];
    if (view())
      view()->OnOptionAdded(devices_.size() - 1);
  }
}

void UsbChooserController::OnDeviceRemoved(scoped_refptr<UsbDevice> device) {
  for (auto it = devices_.begin(); it != devices_.end(); ++it) {
    if (it->first == device) {
      size_t index = it - devices_.begin();
      DCHECK_GT(device_name_map_[it->second], 0);
      if (--device_name_map_[it->second] == 0)
        device_name_map_.erase(it->second);
      devices_.erase(it);
      if (view())
        view()->OnOptionRemoved(index);
      return;
    }
  }
}

// Get a list of devices that can be shown in the chooser bubble UI for
// user to grant permsssion.
void UsbChooserController::GotUsbDeviceList(
    const std::vector<scoped_refptr<UsbDevice>>& devices) {
  for (const auto& device : devices) {
    if (DisplayDevice(device)) {
      base::string16 device_name = FormatUsbDeviceName(device);
      devices_.push_back(std::make_pair(device, device_name));
      ++device_name_map_[device_name];
    }
  }
  if (view())
    view()->OnOptionsInitialized();
}

bool UsbChooserController::DisplayDevice(
    scoped_refptr<UsbDevice> device) const {
  if (!UsbDeviceFilterMatchesAny(filters_, *device))
    return false;

  if (UsbBlocklist::Get().IsExcluded(device))
    return false;

  return true;
}
