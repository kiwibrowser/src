// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/shell/shell_browser_state.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/shell/shell_url_request_context_getter.h"
#include "services/test/user_id/user_id_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

ShellBrowserState::ShellBrowserState() : BrowserState() {
  CHECK(base::PathService::Get(base::DIR_APP_DATA, &path_));

  request_context_getter_ = new ShellURLRequestContextGetter(
      GetStatePath(),
      web::WebThread::GetTaskRunnerForThread(web::WebThread::IO));

  BrowserState::Initialize(this, path_);
}

ShellBrowserState::~ShellBrowserState() {
}

bool ShellBrowserState::IsOffTheRecord() const {
  return false;
}

base::FilePath ShellBrowserState::GetStatePath() const {
  return path_;
}

net::URLRequestContextGetter* ShellBrowserState::GetRequestContext() {
  return request_context_getter_.get();
}

void ShellBrowserState::RegisterServices(StaticServiceMap* services) {
  service_manager::EmbeddedServiceInfo user_id_info;
  user_id_info.factory = base::Bind(&user_id::CreateUserIdService);
  user_id_info.task_runner = base::ThreadTaskRunnerHandle::Get();
  services->insert(std::make_pair("user_id", user_id_info));
}

}  // namespace web
