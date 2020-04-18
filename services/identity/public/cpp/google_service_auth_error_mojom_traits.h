// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_MOJOM_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_MOJOM_TRAITS_H_

#include <string>

#include "google_apis/gaia/google_service_auth_error.h"
#include "services/identity/public/mojom/google_service_auth_error.mojom.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::GoogleServiceAuthError::DataView,
                    ::GoogleServiceAuthError> {
  static int state(const ::GoogleServiceAuthError& r) { return r.state(); }
  static const ::GoogleServiceAuthError::Captcha& captcha(
      const ::GoogleServiceAuthError& r) {
    return r.captcha();
  }
  static const ::GoogleServiceAuthError::SecondFactor& second_factor(
      const ::GoogleServiceAuthError& r) {
    return r.second_factor();
  }
  static int network_error(const ::GoogleServiceAuthError& r) {
    return r.network_error();
  }

  static const std::string& error_message(const ::GoogleServiceAuthError& r) {
    return r.error_message();
  }

  static bool Read(identity::mojom::GoogleServiceAuthErrorDataView data,
                   ::GoogleServiceAuthError* out);
};

template <>
struct StructTraits<identity::mojom::Captcha::DataView,
                    ::GoogleServiceAuthError::Captcha> {
  static const std::string& token(const ::GoogleServiceAuthError::Captcha& r) {
    return r.token;
  }
  static const GURL& audio_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.audio_url;
  }
  static const GURL& image_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_url;
  }
  static const GURL& unlock_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.unlock_url;
  }
  static int image_width(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_width;
  }
  static int image_height(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_height;
  }

  static bool Read(identity::mojom::CaptchaDataView data,
                   ::GoogleServiceAuthError::Captcha* out);
};

template <>
struct StructTraits<identity::mojom::SecondFactor::DataView,
                    ::GoogleServiceAuthError::SecondFactor> {
  static std::string token(const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.token;
  }
  static const std::string& prompt_text(
      const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.prompt_text;
  }
  static const std::string& alternate_text(
      const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.alternate_text;
  }
  static int field_length(const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.field_length;
  }

  static bool Read(identity::mojom::SecondFactorDataView data,
                   ::GoogleServiceAuthError::SecondFactor* out);
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_MOJOM_TRAITS_H_
