// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/encryptedmedia/encrypted_media_utils.h"

namespace blink {

namespace {

const char kTemporary[] = "temporary";
const char kPersistentLicense[] = "persistent-license";

}  // namespace

// static
WebEncryptedMediaInitDataType EncryptedMediaUtils::ConvertToInitDataType(
    const String& init_data_type) {
  if (init_data_type == "cenc")
    return WebEncryptedMediaInitDataType::kCenc;
  if (init_data_type == "keyids")
    return WebEncryptedMediaInitDataType::kKeyids;
  if (init_data_type == "webm")
    return WebEncryptedMediaInitDataType::kWebm;

  // |initDataType| is not restricted in the idl, so anything is possible.
  return WebEncryptedMediaInitDataType::kUnknown;
}

// static
String EncryptedMediaUtils::ConvertFromInitDataType(
    WebEncryptedMediaInitDataType init_data_type) {
  switch (init_data_type) {
    case WebEncryptedMediaInitDataType::kCenc:
      return "cenc";
    case WebEncryptedMediaInitDataType::kKeyids:
      return "keyids";
    case WebEncryptedMediaInitDataType::kWebm:
      return "webm";
    case WebEncryptedMediaInitDataType::kUnknown:
      // Chromium should not use Unknown, but we use it in Blink when the
      // actual value has been blocked for non-same-origin or mixed content.
      return String();
  }

  NOTREACHED();
  return String();
}

// static
WebEncryptedMediaSessionType EncryptedMediaUtils::ConvertToSessionType(
    const String& session_type) {
  if (session_type == kTemporary)
    return WebEncryptedMediaSessionType::kTemporary;
  if (session_type == kPersistentLicense)
    return WebEncryptedMediaSessionType::kPersistentLicense;

  // |sessionType| is not restricted in the idl, so anything is possible.
  return WebEncryptedMediaSessionType::kUnknown;
}

// static
String EncryptedMediaUtils::ConvertFromSessionType(
    WebEncryptedMediaSessionType session_type) {
  switch (session_type) {
    case WebEncryptedMediaSessionType::kTemporary:
      return kTemporary;
    case WebEncryptedMediaSessionType::kPersistentLicense:
      return kPersistentLicense;
    // FIXME: Remove once removed from Chromium (crbug.com/448888).
    case WebEncryptedMediaSessionType::kPersistentReleaseMessage:
    case WebEncryptedMediaSessionType::kUnknown:
      // Chromium should not use Unknown.
      NOTREACHED();
      return String();
  }

  NOTREACHED();
  return String();
}

// static
String EncryptedMediaUtils::ConvertKeyStatusToString(
    const WebEncryptedMediaKeyInformation::KeyStatus status) {
  switch (status) {
    case WebEncryptedMediaKeyInformation::KeyStatus::kUsable:
      return "usable";
    case WebEncryptedMediaKeyInformation::KeyStatus::kExpired:
      return "expired";
    case WebEncryptedMediaKeyInformation::KeyStatus::kReleased:
      return "released";
    case WebEncryptedMediaKeyInformation::KeyStatus::kOutputRestricted:
      return "output-restricted";
    case WebEncryptedMediaKeyInformation::KeyStatus::kOutputDownscaled:
      return "output-downscaled";
    case WebEncryptedMediaKeyInformation::KeyStatus::kStatusPending:
      return "status-pending";
    case WebEncryptedMediaKeyInformation::KeyStatus::kInternalError:
      return "internal-error";
  }

  NOTREACHED();
  return "internal-error";
}

}  // namespace blink
