// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MAC_H_
#define CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MAC_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/apps/app_shim/app_shim_handler_mac.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"
#include "mojo/edk/embedder/peer_connection.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace IPC {
class ChannelProxy;
class Message;
}  // namespace IPC

// This is the counterpart to AppShimController in
// chrome/app/chrome_main_app_mode_mac.mm. The AppShimHost owns itself, and is
// destroyed when the app it corresponds to is closed or when the channel
// connected to the app shim is closed.
class AppShimHost : public IPC::Listener,
                    public IPC::Sender,
                    public apps::AppShimHandler::Host {
 public:
  AppShimHost();
  ~AppShimHost() override;

  // Creates a new server-side IPC channel at |handle|, which should contain a
  // file descriptor of a channel created by an UnixDomainSocketAcceptor,
  // and begins listening for messages on it.
  void ServeChannel(mojo::edk::ScopedInternalPlatformHandle handle);

 protected:
  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelError() override;

  // IPC::Sender implementation.
  bool Send(IPC::Message* message) override;

 private:
  // The app shim process is requesting to be associated with the given profile
  // and app_id. Once the profile and app_id are stored, and all future
  // messages from the app shim relate to this app. The app is launched
  // immediately if |launch_now| is true.
  void OnLaunchApp(const base::FilePath& profile_dir,
                   const std::string& app_id,
                   apps::AppShimLaunchType launch_type,
                   const std::vector<base::FilePath>& files);

  // Called when the app shim process notifies that the app was focused.
  void OnFocus(apps::AppShimFocusType focus_type,
               const std::vector<base::FilePath>& files);

  void OnSetHidden(bool hidden);

  // Called when the app shim process notifies that the app should quit.
  void OnQuit();

  // apps::AppShimHandler::Host overrides:
  void OnAppLaunchComplete(apps::AppShimLaunchResult result) override;
  void OnAppClosed() override;
  void OnAppHide() override;
  void OnAppUnhideWithoutActivation() override;
  void OnAppRequestUserAttention(apps::AppShimAttentionType type) override;
  base::FilePath GetProfilePath() const override;
  std::string GetAppId() const override;

  // Closes the channel and destroys the AppShimHost.
  void Close();

  mojo::edk::PeerConnection peer_connection_;
  std::unique_ptr<IPC::ChannelProxy> channel_;
  std::string app_id_;
  base::FilePath profile_path_;
  bool initial_launch_finished_;

  THREAD_CHECKER(thread_checker_);
};

#endif  // CHROME_BROWSER_APPS_APP_SHIM_APP_SHIM_HOST_MAC_H_
