// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/google_service_auth_error_mojom_traits.h"

#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<identity::mojom::GoogleServiceAuthError::DataView,
                  ::GoogleServiceAuthError>::
    Read(identity::mojom::GoogleServiceAuthErrorDataView data,
         ::GoogleServiceAuthError* out) {
  int state = data.state();
  ::GoogleServiceAuthError::Captcha captcha;
  ::GoogleServiceAuthError::SecondFactor second_factor;
  std::string error_message;
  if (state < 0 || state > ::GoogleServiceAuthError::State::NUM_STATES ||
      !data.ReadCaptcha(&captcha) || !data.ReadSecondFactor(&second_factor) ||
      !data.ReadErrorMessage(&error_message)) {
    return false;
  }

  out->state_ = ::GoogleServiceAuthError::State(state);
  out->captcha_ = captcha;
  out->second_factor_ = second_factor;
  out->error_message_ = error_message;
  out->network_error_ = data.network_error();

  return true;
}

// static
bool StructTraits<identity::mojom::Captcha::DataView,
                  ::GoogleServiceAuthError::Captcha>::
    Read(identity::mojom::CaptchaDataView data,
         ::GoogleServiceAuthError::Captcha* out) {
  std::string token;
  GURL audio_url;
  GURL image_url;
  GURL unlock_url;
  if (data.image_width() < 0 || data.image_height() < 0 ||
      !data.ReadToken(&token) || !data.ReadAudioUrl(&audio_url) ||
      !data.ReadImageUrl(&image_url) || !data.ReadUnlockUrl(&unlock_url)) {
    return false;
  }

  out->token = token;
  out->audio_url = audio_url;
  out->image_url = image_url;
  out->unlock_url = unlock_url;
  out->image_width = data.image_width();
  out->image_height = data.image_height();

  return true;
}

// static
bool StructTraits<identity::mojom::SecondFactor::DataView,
                  ::GoogleServiceAuthError::SecondFactor>::
    Read(identity::mojom::SecondFactorDataView data,
         ::GoogleServiceAuthError::SecondFactor* out) {
  std::string token;
  std::string prompt_text;
  std::string alternate_text;
  if (data.field_length() < 0 || !data.ReadToken(&token) ||
      !data.ReadPromptText(&prompt_text) ||
      !data.ReadAlternateText(&alternate_text)) {
    return false;
  }

  out->token = token;
  out->prompt_text = prompt_text;
  out->alternate_text = alternate_text;
  out->field_length = data.field_length();

  return true;
}

}  // namespace mojo
