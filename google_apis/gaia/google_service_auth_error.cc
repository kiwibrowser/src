// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/google_service_auth_error.h"

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "net/base/net_errors.h"

GoogleServiceAuthError::Captcha::Captcha() : image_width(0), image_height(0) {
}

GoogleServiceAuthError::Captcha::Captcha(
    const std::string& token, const GURL& audio, const GURL& img,
    const GURL& unlock, int width, int height)
    : token(token), audio_url(audio), image_url(img), unlock_url(unlock),
      image_width(width), image_height(height) {
}

GoogleServiceAuthError::Captcha::Captcha(const Captcha& other) = default;

GoogleServiceAuthError::Captcha::~Captcha() {
}

bool GoogleServiceAuthError::Captcha::operator==(const Captcha& b) const {
  return (token == b.token &&
          audio_url == b.audio_url &&
          image_url == b.image_url &&
          unlock_url == b.unlock_url &&
          image_width == b.image_width &&
          image_height == b.image_height);
}

GoogleServiceAuthError::SecondFactor::SecondFactor() : field_length(0) {
}

GoogleServiceAuthError::SecondFactor::SecondFactor(
    const std::string& token, const std::string& prompt,
    const std::string& alternate, int length)
    : token(token), prompt_text(prompt), alternate_text(alternate),
      field_length(length) {
}

GoogleServiceAuthError::SecondFactor::SecondFactor(const SecondFactor& other) =
    default;

GoogleServiceAuthError::SecondFactor::~SecondFactor() {
}

bool GoogleServiceAuthError::SecondFactor::operator==(
    const SecondFactor& b) const {
  return (token == b.token &&
          prompt_text == b.prompt_text &&
          alternate_text == b.alternate_text &&
          field_length == b.field_length);
}

bool GoogleServiceAuthError::operator==(
    const GoogleServiceAuthError& b) const {
  return (state_ == b.state_) && (network_error_ == b.network_error_) &&
         (captcha_ == b.captcha_) && (second_factor_ == b.second_factor_) &&
         (error_message_ == b.error_message_) &&
         (invalid_gaia_credentials_reason_ ==
          b.invalid_gaia_credentials_reason_);
}

bool GoogleServiceAuthError::operator!=(
    const GoogleServiceAuthError& b) const {
  return !(*this == b);
}

GoogleServiceAuthError::GoogleServiceAuthError()
    : GoogleServiceAuthError(NONE) {}

GoogleServiceAuthError::GoogleServiceAuthError(State s)
    : GoogleServiceAuthError(s, std::string()) {}

GoogleServiceAuthError::GoogleServiceAuthError(State state,
                                               const std::string& error_message)
    : GoogleServiceAuthError(
          state,
          (state == CONNECTION_FAILED) ? net::ERR_FAILED : 0) {
  error_message_ = error_message;
}

GoogleServiceAuthError::GoogleServiceAuthError(
    const GoogleServiceAuthError& other) = default;

// static
GoogleServiceAuthError
    GoogleServiceAuthError::FromConnectionError(int error) {
  return GoogleServiceAuthError(CONNECTION_FAILED, error);
}

// static
GoogleServiceAuthError GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
    InvalidGaiaCredentialsReason reason) {
  GoogleServiceAuthError error(INVALID_GAIA_CREDENTIALS);
  error.invalid_gaia_credentials_reason_ = reason;
  return error;
}

// static
GoogleServiceAuthError GoogleServiceAuthError::FromClientLoginCaptchaChallenge(
    const std::string& captcha_token,
    const GURL& captcha_image_url,
    const GURL& captcha_unlock_url) {
  return GoogleServiceAuthError(CAPTCHA_REQUIRED, captcha_token, GURL(),
                                captcha_image_url, captcha_unlock_url, 0, 0);
}

// static
GoogleServiceAuthError GoogleServiceAuthError::FromServiceError(
    const std::string& error_message) {
  return GoogleServiceAuthError(SERVICE_ERROR, error_message);
}

// static
GoogleServiceAuthError GoogleServiceAuthError::FromUnexpectedServiceResponse(
    const std::string& error_message) {
  return GoogleServiceAuthError(UNEXPECTED_SERVICE_RESPONSE, error_message);
}

// static
GoogleServiceAuthError GoogleServiceAuthError::AuthErrorNone() {
  return GoogleServiceAuthError(NONE);
}

// static
bool GoogleServiceAuthError::IsDeprecated(State state) {
  return state == HOSTED_NOT_ALLOWED_DEPRECATED;
}

GoogleServiceAuthError::State GoogleServiceAuthError::state() const {
  return state_;
}

const GoogleServiceAuthError::Captcha& GoogleServiceAuthError::captcha() const {
  return captcha_;
}

const GoogleServiceAuthError::SecondFactor&
GoogleServiceAuthError::second_factor() const {
  return second_factor_;
}

int GoogleServiceAuthError::network_error() const {
  return network_error_;
}

const std::string& GoogleServiceAuthError::token() const {
  switch (state_) {
    case CAPTCHA_REQUIRED:
      return captcha_.token;
      break;
    case TWO_FACTOR:
      return second_factor_.token;
      break;
    default:
      NOTREACHED();
  }
  return base::EmptyString();
}

const std::string& GoogleServiceAuthError::error_message() const {
  return error_message_;
}

GoogleServiceAuthError::InvalidGaiaCredentialsReason
GoogleServiceAuthError::GetInvalidGaiaCredentialsReason() const {
  DCHECK_EQ(INVALID_GAIA_CREDENTIALS, state());
  return invalid_gaia_credentials_reason_;
}

std::string GoogleServiceAuthError::ToString() const {
  switch (state_) {
    case NONE:
      return std::string();
    case INVALID_GAIA_CREDENTIALS:
      return base::StringPrintf(
          "Invalid credentials (%d).",
          static_cast<int>(invalid_gaia_credentials_reason_));
    case USER_NOT_SIGNED_UP:
      return "Not authorized.";
    case CONNECTION_FAILED:
      return base::StringPrintf("Connection failed (%d).", network_error_);
    case CAPTCHA_REQUIRED:
      return base::StringPrintf("CAPTCHA required (%s).",
                                captcha_.token.c_str());
    case ACCOUNT_DELETED:
      return "Account deleted.";
    case ACCOUNT_DISABLED:
      return "Account disabled.";
    case SERVICE_UNAVAILABLE:
      return "Service unavailable; try again later.";
    case TWO_FACTOR:
      return base::StringPrintf("2-step verification required (%s).",
                                second_factor_.token.c_str());
    case REQUEST_CANCELED:
      return "Request canceled.";
    case UNEXPECTED_SERVICE_RESPONSE:
      return base::StringPrintf("Unexpected service response (%s)",
                                error_message_.c_str());
    case SERVICE_ERROR:
      return base::StringPrintf("Service responded with error: '%s'",
                                error_message_.c_str());
    case WEB_LOGIN_REQUIRED:
      return "Less secure apps may not authenticate with this account. "
             "Please visit: "
             "https://www.google.com/settings/security/lesssecureapps";
    default:
      NOTREACHED();
      return std::string();
  }
}

bool GoogleServiceAuthError::IsPersistentError() const {
  if (state_ == GoogleServiceAuthError::NONE) return false;
  return !IsTransientError();
}

bool GoogleServiceAuthError::IsTransientError() const {
  switch (state_) {
  // These are failures that are likely to succeed if tried again.
  case GoogleServiceAuthError::CONNECTION_FAILED:
  case GoogleServiceAuthError::SERVICE_UNAVAILABLE:
  case GoogleServiceAuthError::REQUEST_CANCELED:
    return true;
  // Everything else will have the same result.
  default:
    return false;
  }
}

GoogleServiceAuthError::GoogleServiceAuthError(State s, int error)
    : state_(s),
      network_error_(error),
      invalid_gaia_credentials_reason_(InvalidGaiaCredentialsReason::UNKNOWN) {}

GoogleServiceAuthError::GoogleServiceAuthError(State s,
                                               const std::string& captcha_token,
                                               const GURL& captcha_audio_url,
                                               const GURL& captcha_image_url,
                                               const GURL& captcha_unlock_url,
                                               int image_width,
                                               int image_height)
    : state_(s),
      captcha_(captcha_token,
               captcha_audio_url,
               captcha_image_url,
               captcha_unlock_url,
               image_width,
               image_height),
      network_error_((state_ == CONNECTION_FAILED) ? net::ERR_FAILED : net::OK),
      invalid_gaia_credentials_reason_(InvalidGaiaCredentialsReason::UNKNOWN) {}
