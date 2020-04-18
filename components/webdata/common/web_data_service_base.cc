// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webdata/common/web_data_service_base.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/thread.h"
#include "components/webdata/common/web_database_service.h"

////////////////////////////////////////////////////////////////////////////////
//
// WebDataServiceBase implementation.
//
////////////////////////////////////////////////////////////////////////////////

using base::Bind;
using base::Time;

WebDataServiceBase::WebDataServiceBase(
    scoped_refptr<WebDatabaseService> wdbs,
    const ProfileErrorCallback& callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner)
    : base::RefCountedDeleteOnSequence<WebDataServiceBase>(ui_task_runner),
      wdbs_(wdbs),
      profile_error_callback_(callback) {}

void WebDataServiceBase::ShutdownOnUISequence() {}

void WebDataServiceBase::Init() {
  DCHECK(wdbs_.get());
  wdbs_->RegisterDBErrorCallback(profile_error_callback_);
  wdbs_->LoadDatabase();
}

void WebDataServiceBase::ShutdownDatabase() {
  if (!wdbs_.get())
    return;
  wdbs_->ShutdownDatabase();
}

void WebDataServiceBase::CancelRequest(Handle h) {
  if (!wdbs_.get())
    return;
  wdbs_->CancelRequest(h);
}

bool WebDataServiceBase::IsDatabaseLoaded() {
  if (!wdbs_.get())
    return false;
  return wdbs_->db_loaded();
}

void WebDataServiceBase::RegisterDBLoadedCallback(
    const DBLoadedCallback& callback) {
  if (!wdbs_.get())
    return;
  wdbs_->RegisterDBLoadedCallback(callback);
}

WebDatabase* WebDataServiceBase::GetDatabase() {
  if (!wdbs_.get())
    return nullptr;
  return wdbs_->GetDatabaseOnDB();
}

WebDataServiceBase::~WebDataServiceBase() {
}
