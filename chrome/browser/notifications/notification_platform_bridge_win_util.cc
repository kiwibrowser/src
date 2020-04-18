// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_win_util.h"

#include <wrl/client.h>

#include "base/win/scoped_hstring.h"
#include "chrome/browser/notifications/notification_launch_id.h"
#include "chrome/browser/notifications/notification_platform_bridge_win_metrics.h"
#include "chrome/browser/notifications/notification_template_builder.h"

namespace mswr = Microsoft::WRL;
namespace winfoundtn = ABI::Windows::Foundation;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

using notifications_uma::GetNotificationLaunchIdStatus;
using base::win::ScopedHString;

NotificationLaunchId GetNotificationLaunchId(
    winui::Notifications::IToastNotification* notification) {
  mswr::ComPtr<winxml::Dom::IXmlDocument> document;
  HRESULT hr = notification->get_Content(&document);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::NOTIFICATION_GET_CONTENT_FAILED);
    DLOG(ERROR) << "Failed to get XML document";
    return NotificationLaunchId();
  }

  ScopedHString tag = ScopedHString::Create(kNotificationToastElement);
  mswr::ComPtr<winxml::Dom::IXmlNodeList> elements;
  hr = document->GetElementsByTagName(tag.get(), &elements);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_ELEMENTS_BY_TAG_FAILED);
    DLOG(ERROR) << "Failed to get <toast> elements from document";
    return NotificationLaunchId();
  }

  UINT32 length;
  hr = elements->get_Length(&length);
  if (length == 0) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::MISSING_TOAST_ELEMENT_IN_DOC);
    DLOG(ERROR) << "No <toast> elements in document.";
    return NotificationLaunchId();
  }

  mswr::ComPtr<winxml::Dom::IXmlNode> node;
  hr = elements->Item(0, &node);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::ITEM_AT_FAILED);
    DLOG(ERROR) << "Failed to get first <toast> element";
    return NotificationLaunchId();
  }

  mswr::ComPtr<winxml::Dom::IXmlNamedNodeMap> attributes;
  hr = node->get_Attributes(&attributes);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_ATTRIBUTES_FAILED);
    DLOG(ERROR) << "Failed to get attributes of <toast>";
    return NotificationLaunchId();
  }

  mswr::ComPtr<winxml::Dom::IXmlNode> leaf;
  ScopedHString id = ScopedHString::Create(kNotificationLaunchAttribute);
  hr = attributes->GetNamedItem(id.get(), &leaf);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_NAMED_ITEM_FAILED);
    DLOG(ERROR) << "Failed to get launch attribute of <toast>";
    return NotificationLaunchId();
  }

  mswr::ComPtr<winxml::Dom::IXmlNode> child;
  hr = leaf->get_FirstChild(&child);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_FIRST_CHILD_FAILED);
    DLOG(ERROR) << "Failed to get content of launch attribute";
    return NotificationLaunchId();
  }

  mswr::ComPtr<IInspectable> inspectable;
  hr = child->get_NodeValue(&inspectable);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_NODE_VALUE_FAILED);
    DLOG(ERROR) << "Failed to get node value of launch attribute";
    return NotificationLaunchId();
  }

  mswr::ComPtr<winfoundtn::IPropertyValue> property_value;
  hr = inspectable.As<winfoundtn::IPropertyValue>(&property_value);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::CONVERSION_TO_PROP_VALUE_FAILED);
    DLOG(ERROR) << "Failed to convert node value of launch attribute";
    return NotificationLaunchId();
  }

  HSTRING value_hstring;
  hr = property_value->GetString(&value_hstring);
  if (FAILED(hr)) {
    LogGetNotificationLaunchIdStatus(
        GetNotificationLaunchIdStatus::GET_STRING_FAILED);
    DLOG(ERROR) << "Failed to get string for launch attribute";
    return NotificationLaunchId();
  }

  LogGetNotificationLaunchIdStatus(GetNotificationLaunchIdStatus::SUCCESS);

  ScopedHString value(value_hstring);
  return NotificationLaunchId(value.GetAsUTF8());
}
