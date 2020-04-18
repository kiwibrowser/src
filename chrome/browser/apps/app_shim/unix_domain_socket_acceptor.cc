// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_shim/unix_domain_socket_acceptor.h"

#include <utility>

#include "base/logging.h"
#include "base/message_loop/message_loop_current.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"

namespace apps {

UnixDomainSocketAcceptor::UnixDomainSocketAcceptor(const base::FilePath& path,
                                                   Delegate* delegate)
    : server_listen_connection_watcher_(FROM_HERE),
      named_pipe_(path.value()),
      delegate_(delegate),
      listen_handle_(mojo::edk::CreateServerHandle(named_pipe_)) {
  DCHECK(delegate_);
}

UnixDomainSocketAcceptor::~UnixDomainSocketAcceptor() {
  Close();
}

bool UnixDomainSocketAcceptor::Listen() {
  if (!listen_handle_.is_valid())
    return false;

  // Watch the fd for connections, and turn any connections into
  // active sockets.
  base::MessageLoopCurrentForIO::Get()->WatchFileDescriptor(
      listen_handle_.get().handle, true, base::MessagePumpForIO::WATCH_READ,
      &server_listen_connection_watcher_, this);
  return true;
}

// Called by libevent when we can read from the fd without blocking.
void UnixDomainSocketAcceptor::OnFileCanReadWithoutBlocking(int fd) {
  DCHECK(fd == listen_handle_.get().handle);
  mojo::edk::ScopedInternalPlatformHandle connection_handle;
  if (!mojo::edk::ServerAcceptConnection(listen_handle_, &connection_handle)) {
    Close();
    delegate_->OnListenError();
    return;
  }

  if (!connection_handle.is_valid()) {
    // The accept() failed, but not in such a way that the factory needs to be
    // shut down.
    return;
  }

  delegate_->OnClientConnected(std::move(connection_handle));
}

void UnixDomainSocketAcceptor::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED() << "Listen fd should never be writable.";
}

void UnixDomainSocketAcceptor::Close() {
  if (!listen_handle_.is_valid())
    return;
  listen_handle_.reset();
  if (unlink(named_pipe_.name.c_str()) < 0)
    PLOG(ERROR) << "unlink";

  // Unregister libevent for the listening socket and close it.
  server_listen_connection_watcher_.StopWatchingFileDescriptor();
}

}  // namespace apps
