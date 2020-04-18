// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/test/test_app_list_client.h"

#include "ash/shell.h"

namespace ash {

TestAppListClient::TestAppListClient() : binding_(this) {}

TestAppListClient::~TestAppListClient() {}

mojom::AppListClientPtr TestAppListClient::CreateInterfacePtrAndBind() {
  mojom::AppListClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TestAppListClient::StartVoiceInteractionSession() {
  ++voice_session_count_;
}

void TestAppListClient::ToggleVoiceInteractionSession() {
  ++voice_session_count_;
}

}  // namespace ash
