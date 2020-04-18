// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/test/test_content_utility_client.h"

#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace extensions {

TestContentUtilityClient::TestContentUtilityClient() = default;

TestContentUtilityClient::~TestContentUtilityClient() = default;

void TestContentUtilityClient::UtilityThreadStarted() {
  auto registry = std::make_unique<service_manager::BinderRegistry>();
  content::ChildThread::Get()
      ->GetServiceManagerConnection()
      ->AddConnectionFilter(std::make_unique<content::SimpleConnectionFilter>(
          std::move(registry)));
}

}  // namespace extensions
