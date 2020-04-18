// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/clipboard.h"

#include "base/memory/ptr_util.h"

#include "base/bind.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/logging.h"
#include "base/macros.h"
#include "remoting/host/linux/x_server_clipboard.h"
#include "remoting/proto/event.pb.h"
#include "remoting/protocol/clipboard_stub.h"
#include "ui/gfx/x/x11.h"

namespace remoting {

// This code is expected to be called on the desktop thread only.
class ClipboardX11 : public Clipboard {
 public:
  ClipboardX11();
  ~ClipboardX11() override;

  // Clipboard interface.
  void Start(
      std::unique_ptr<protocol::ClipboardStub> client_clipboard) override;
  void InjectClipboardEvent(const protocol::ClipboardEvent& event) override;

 private:
  void OnClipboardChanged(const std::string& mime_type,
                          const std::string& data);
  void PumpXEvents();

  std::unique_ptr<protocol::ClipboardStub> client_clipboard_;

  // Underlying X11 clipboard implementation.
  XServerClipboard x_server_clipboard_;

  // Connection to the X server, used by |x_server_clipboard_|. This is created
  // and owned by this class.
  Display* display_;

  // Watcher used to handle X11 events from |display_|.
  std::unique_ptr<base::FileDescriptorWatcher::Controller>
      x_connection_watch_controller_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardX11);
};

ClipboardX11::ClipboardX11()
    : display_(nullptr) {
}

ClipboardX11::~ClipboardX11() {
  if (display_)
    XCloseDisplay(display_);
}

void ClipboardX11::Start(
    std::unique_ptr<protocol::ClipboardStub> client_clipboard) {
  // TODO(lambroslambrou): Share the X connection with InputInjector.
  DCHECK(!display_);
  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    LOG(ERROR) << "Couldn't open X display";
    return;
  }
  client_clipboard_.swap(client_clipboard);

  x_server_clipboard_.Init(display_,
                           base::Bind(&ClipboardX11::OnClipboardChanged,
                                      base::Unretained(this)));

  x_connection_watch_controller_ = base::FileDescriptorWatcher::WatchReadable(
      ConnectionNumber(display_),
      base::Bind(&ClipboardX11::PumpXEvents, base::Unretained(this)));
  PumpXEvents();
}

void ClipboardX11::InjectClipboardEvent(
    const protocol::ClipboardEvent& event) {
  x_server_clipboard_.SetClipboard(event.mime_type(), event.data());
}

void ClipboardX11::OnClipboardChanged(const std::string& mime_type,
                                      const std::string& data) {
  protocol::ClipboardEvent event;
  event.set_mime_type(mime_type);
  event.set_data(data);

  if (client_clipboard_.get()) {
    client_clipboard_->InjectClipboardEvent(event);
  }
}

void ClipboardX11::PumpXEvents() {
  DCHECK(display_);

  while (XPending(display_)) {
    XEvent event;
    XNextEvent(display_, &event);
    x_server_clipboard_.ProcessXEvent(&event);
  }
}

std::unique_ptr<Clipboard> Clipboard::Create() {
  return base::WrapUnique(new ClipboardX11());
}

}  // namespace remoting
