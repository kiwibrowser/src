// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/resource_handler.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/browser/loader/resource_request_info_impl.h"

namespace content {

ResourceHandler::Delegate::Delegate() {}

ResourceHandler::Delegate::~Delegate() {}

void ResourceHandler::Delegate::PauseReadingBodyFromNet() {}

void ResourceHandler::Delegate::ResumeReadingBodyFromNet() {}

void ResourceHandler::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

ResourceHandler::~ResourceHandler() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

ResourceHandler::ResourceHandler(net::URLRequest* request)
    : request_(request) {}

void ResourceHandler::HoldController(
    std::unique_ptr<ResourceController> controller) {
  controller_ = std::move(controller);
}

std::unique_ptr<ResourceController> ResourceHandler::ReleaseController() {
  DCHECK(controller_);

  return std::move(controller_);
}

void ResourceHandler::Resume() {
  ReleaseController()->Resume();
}

void ResourceHandler::Cancel() {
  ReleaseController()->Cancel();
}

void ResourceHandler::CancelWithError(int error_code) {
  ReleaseController()->CancelWithError(error_code);
}

void ResourceHandler::OutOfBandCancel(int error_code, bool tell_renderer) {
  delegate_->OutOfBandCancel(error_code, tell_renderer);
}

void ResourceHandler::PauseReadingBodyFromNet() {
  delegate_->PauseReadingBodyFromNet();
}

void ResourceHandler::ResumeReadingBodyFromNet() {
  delegate_->ResumeReadingBodyFromNet();
}

void ResourceHandler::GetNumericArg(const std::string& name, int* result) {
  const std::string& value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(name);
  if (!value.empty())
    base::StringToInt(value, result);
}

ResourceRequestInfoImpl* ResourceHandler::GetRequestInfo() const {
  return ResourceRequestInfoImpl::ForRequest(request_);
}

int ResourceHandler::GetRequestID() const {
  return GetRequestInfo()->GetRequestID();
}

ResourceMessageFilter* ResourceHandler::GetFilter() const {
  return GetRequestInfo()->requester_info()->filter();
}

}  // namespace content
