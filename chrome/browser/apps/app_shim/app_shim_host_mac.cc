// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_shim/app_shim_host_mac.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "chrome/browser/apps/app_shim/app_shim_handler_mac.h"
#include "chrome/common/mac/app_shim_messages.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_channel_mojo.h"
#include "ipc/ipc_channel_proxy.h"
#include "mojo/edk/embedder/embedder.h"

AppShimHost::AppShimHost() : initial_launch_finished_(false) {}

AppShimHost::~AppShimHost() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  apps::AppShimHandler* handler = apps::AppShimHandler::GetForAppMode(app_id_);
  if (handler)
    handler->OnShimClose(this);
}

void AppShimHost::ServeChannel(mojo::edk::ScopedInternalPlatformHandle handle) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!channel_.get());
  channel_ = IPC::ChannelProxy::Create(
      IPC::ChannelMojo::CreateServerFactory(
          peer_connection_.Connect(mojo::edk::ConnectionParams(
              mojo::edk::TransportProtocol::kLegacy, std::move(handle))),
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::IO)
              .get(),
          base::ThreadTaskRunnerHandle::Get()),
      this,
      content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
          .get(),
      base::ThreadTaskRunnerHandle::Get());
}

base::FilePath AppShimHost::GetProfilePath() const {
  return profile_path_;
}

std::string AppShimHost::GetAppId() const {
  return app_id_;
}

bool AppShimHost::OnMessageReceived(const IPC::Message& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AppShimHost, message)
    IPC_MESSAGE_HANDLER(AppShimHostMsg_LaunchApp, OnLaunchApp)
    IPC_MESSAGE_HANDLER(AppShimHostMsg_FocusApp, OnFocus)
    IPC_MESSAGE_HANDLER(AppShimHostMsg_SetAppHidden, OnSetHidden)
    IPC_MESSAGE_HANDLER(AppShimHostMsg_QuitApp, OnQuit)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AppShimHost::OnChannelError() {
  Close();
}

bool AppShimHost::Send(IPC::Message* message) {
  DCHECK(channel_.get());
  return channel_->Send(message);
}

void AppShimHost::OnLaunchApp(const base::FilePath& profile_dir,
                              const std::string& app_id,
                              apps::AppShimLaunchType launch_type,
                              const std::vector<base::FilePath>& files) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(profile_path_.empty());
  // Only one app launch message per channel.
  if (!profile_path_.empty())
    return;

  profile_path_ = profile_dir;
  app_id_ = app_id;

  apps::AppShimHandler* handler = apps::AppShimHandler::GetForAppMode(app_id_);
  if (handler)
    handler->OnShimLaunch(this, launch_type, files);
  // |handler| can only be NULL after AppShimHostManager is destroyed. Since
  // this only happens at shutdown, do nothing here.
}

void AppShimHost::OnFocus(apps::AppShimFocusType focus_type,
                          const std::vector<base::FilePath>& files) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  apps::AppShimHandler* handler = apps::AppShimHandler::GetForAppMode(app_id_);
  if (handler)
    handler->OnShimFocus(this, focus_type, files);
}

void AppShimHost::OnSetHidden(bool hidden) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  apps::AppShimHandler* handler = apps::AppShimHandler::GetForAppMode(app_id_);
  if (handler)
    handler->OnShimSetHidden(this, hidden);
}

void AppShimHost::OnQuit() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  apps::AppShimHandler* handler = apps::AppShimHandler::GetForAppMode(app_id_);
  if (handler)
    handler->OnShimQuit(this);
}

void AppShimHost::OnAppLaunchComplete(apps::AppShimLaunchResult result) {
  if (!initial_launch_finished_) {
    Send(new AppShimMsg_LaunchApp_Done(result));
    initial_launch_finished_ = true;
  }
}

void AppShimHost::OnAppClosed() {
  Close();
}

void AppShimHost::OnAppHide() {
  Send(new AppShimMsg_Hide);
}

void AppShimHost::OnAppUnhideWithoutActivation() {
  Send(new AppShimMsg_UnhideWithoutActivation);
}

void AppShimHost::OnAppRequestUserAttention(apps::AppShimAttentionType type) {
  Send(new AppShimMsg_SetUserAttention(type));
}

void AppShimHost::Close() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  delete this;
}
