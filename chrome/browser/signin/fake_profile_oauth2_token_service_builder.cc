// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"

#include <utility>

#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"

// TODO(blundell): Should these be namespaced?
std::unique_ptr<KeyedService> BuildFakeProfileOAuth2TokenService(
    content::BrowserContext* context) {
  std::unique_ptr<FakeProfileOAuth2TokenService> service(
      new FakeProfileOAuth2TokenService());
  return std::move(service);
}

std::unique_ptr<KeyedService> BuildAutoIssuingFakeProfileOAuth2TokenService(
    content::BrowserContext* context) {
  std::unique_ptr<FakeProfileOAuth2TokenService> service(
      new FakeProfileOAuth2TokenService());
  service->set_auto_post_fetch_response_on_message_loop(true);
  return std::move(service);
}
