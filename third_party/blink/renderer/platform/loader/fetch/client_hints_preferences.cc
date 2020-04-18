// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/client_hints_preferences.h"

#include "base/macros.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

namespace {

void ParseAcceptChHeader(const String& header_value,
                         WebEnabledClientHints& enabled_hints) {
  CommaDelimitedHeaderSet accept_client_hints_header;
  ParseCommaDelimitedHeader(header_value, accept_client_hints_header);

  for (size_t i = 0;
       i < static_cast<int>(mojom::WebClientHintsType::kMaxValue) + 1; ++i) {
    enabled_hints.SetIsEnabled(
        static_cast<mojom::WebClientHintsType>(i),
        accept_client_hints_header.Contains(kClientHintsHeaderMapping[i]));
  }

  enabled_hints.SetIsEnabled(
      mojom::WebClientHintsType::kDeviceMemory,
      enabled_hints.IsEnabled(mojom::WebClientHintsType::kDeviceMemory) &&
          RuntimeEnabledFeatures::DeviceMemoryHeaderEnabled());

  enabled_hints.SetIsEnabled(
      mojom::WebClientHintsType::kRtt,
      enabled_hints.IsEnabled(mojom::WebClientHintsType::kRtt) &&
          RuntimeEnabledFeatures::NetInfoRttHeaderEnabled());

  enabled_hints.SetIsEnabled(
      mojom::WebClientHintsType::kDownlink,
      enabled_hints.IsEnabled(mojom::WebClientHintsType::kDownlink) &&
          RuntimeEnabledFeatures::NetInfoDownlinkHeaderEnabled());

  enabled_hints.SetIsEnabled(
      mojom::WebClientHintsType::kEct,
      enabled_hints.IsEnabled(mojom::WebClientHintsType::kEct) &&
          RuntimeEnabledFeatures::NetInfoEffectiveTypeHeaderEnabled());
}

}  // namespace

ClientHintsPreferences::ClientHintsPreferences() {
  DCHECK_EQ(static_cast<size_t>(mojom::WebClientHintsType::kMaxValue) + 1,
            kClientHintsHeaderMappingCount);
}

void ClientHintsPreferences::UpdateFrom(
    const ClientHintsPreferences& preferences) {
  for (size_t i = 0;
       i < static_cast<int>(mojom::WebClientHintsType::kMaxValue) + 1; ++i) {
    mojom::WebClientHintsType type = static_cast<mojom::WebClientHintsType>(i);
    enabled_hints_.SetIsEnabled(type, preferences.ShouldSend(type));
  }
}

void ClientHintsPreferences::UpdateFromAcceptClientHintsHeader(
    const String& header_value,
    const KURL& url,
    Context* context) {
  if (header_value.IsEmpty())
    return;

  // If the persistent client hint feature is enabled, then client hints
  // should be allowed only on secure URLs.
  if (blink::RuntimeEnabledFeatures::ClientHintsPersistentEnabled() &&
      !IsClientHintsAllowed(url)) {
    return;
  }

  WebEnabledClientHints new_enabled_types;

  ParseAcceptChHeader(header_value, new_enabled_types);

  for (size_t i = 0;
       i < static_cast<int>(mojom::WebClientHintsType::kMaxValue) + 1; ++i) {
    mojom::WebClientHintsType type = static_cast<mojom::WebClientHintsType>(i);
    enabled_hints_.SetIsEnabled(type, enabled_hints_.IsEnabled(type) ||
                                          new_enabled_types.IsEnabled(type));
  }

  if (context) {
    for (size_t i = 0;
         i < static_cast<int>(mojom::WebClientHintsType::kMaxValue) + 1; ++i) {
      mojom::WebClientHintsType type =
          static_cast<mojom::WebClientHintsType>(i);
      if (enabled_hints_.IsEnabled(type))
        context->CountClientHints(type);
    }
  }
}

// static
void ClientHintsPreferences::UpdatePersistentHintsFromHeaders(
    const ResourceResponse& response,
    Context* context,
    WebEnabledClientHints& enabled_hints,
    TimeDelta* persist_duration) {
  *persist_duration = base::TimeDelta();

  if (response.WasCached())
    return;

  String accept_ch_header_value =
      response.HttpHeaderField(HTTPNames::Accept_CH);
  String accept_ch_lifetime_header_value =
      response.HttpHeaderField(HTTPNames::Accept_CH_Lifetime);

  if (!RuntimeEnabledFeatures::ClientHintsPersistentEnabled() ||
      accept_ch_header_value.IsEmpty() ||
      accept_ch_lifetime_header_value.IsEmpty()) {
    return;
  }

  const KURL url = response.Url();
  if (!IsClientHintsAllowed(url))
    return;

  bool conversion_ok = false;
  int64_t persist_duration_seconds =
      accept_ch_lifetime_header_value.ToInt64Strict(&conversion_ok);
  if (!conversion_ok || persist_duration_seconds <= 0)
    return;

  *persist_duration = TimeDelta::FromSeconds(persist_duration_seconds);
  if (context)
    context->CountPersistentClientHintHeaders();

  ParseAcceptChHeader(accept_ch_header_value, enabled_hints);
}

// static
bool ClientHintsPreferences::IsClientHintsAllowed(const KURL& url) {
  return (url.ProtocolIs("http") || url.ProtocolIs("https")) &&
         (SecurityOrigin::IsSecure(url) ||
          SecurityOrigin::Create(url)->IsLocalhost());
}

}  // namespace blink
