// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_KEY_SYSTEM_PROPERTIES_H_
#define MEDIA_BASE_KEY_SYSTEM_PROPERTIES_H_

#include <string>

#include "build/build_config.h"
#include "media/base/decrypt_config.h"
#include "media/base/eme_constants.h"
#include "media/base/media_export.h"

namespace media {

// Provides an interface for querying the properties of a registered key system.
class MEDIA_EXPORT KeySystemProperties {
 public:
  virtual ~KeySystemProperties() {}

  // Gets the name of this key system.
  virtual std::string GetKeySystemName() const = 0;

  // Returns whether |init_data_type| is supported by this key system.
  virtual bool IsSupportedInitDataType(
      EmeInitDataType init_data_type) const = 0;

  // Returns whether |encryption_scheme| is supported by this key system.
  virtual bool IsEncryptionSchemeSupported(
      EncryptionMode encryption_scheme) const = 0;

  // Returns the codecs supported by this key system.
  virtual SupportedCodecs GetSupportedCodecs() const = 0;

#if defined(OS_ANDROID)
  // Returns the codecs with hardware-secure support in this key system.
  virtual SupportedCodecs GetSupportedSecureCodecs() const;
#endif

  // Returns the configuration rule for supporting a robustness requirement.
  virtual EmeConfigRule GetRobustnessConfigRule(
      EmeMediaType media_type,
      const std::string& requested_robustness) const = 0;

  // Returns the support this key system provides for persistent-license
  // sessions.
  virtual EmeSessionTypeSupport GetPersistentLicenseSessionSupport() const = 0;

  // Returns the support this key system provides for persistent-release-message
  // sessions.
  virtual EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport()
      const = 0;

  // Returns the support this key system provides for persistent state.
  virtual EmeFeatureSupport GetPersistentStateSupport() const = 0;

  // Returns the support this key system provides for distinctive identifiers.
  virtual EmeFeatureSupport GetDistinctiveIdentifierSupport() const = 0;

  // Returns whether AesDecryptor can be used for this key system.
  virtual bool UseAesDecryptor() const;
};

}  // namespace media

#endif  // MEDIA_BASE_KEY_SYSTEM_PROPERTIES_H_
