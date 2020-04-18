// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/linux/x11_util.h"

#include "base/bind.h"
#include "ui/gfx/x/x11.h"

namespace remoting {

static ScopedXErrorHandler* g_handler = nullptr;

ScopedXErrorHandler::ScopedXErrorHandler(const Handler& handler):
    handler_(handler),
    ok_(true) {
  // This is a non-exhaustive check for incorrect usage. It doesn't handle the
  // case where a mix of ScopedXErrorHandler and raw XSetErrorHandler calls are
  // used, and it disallows nested ScopedXErrorHandlers on the same thread,
  // despite these being perfectly safe.
  DCHECK(g_handler == nullptr);
  g_handler = this;
  previous_handler_ = XSetErrorHandler(HandleXErrors);
}

ScopedXErrorHandler::~ScopedXErrorHandler() {
  g_handler = nullptr;
  XSetErrorHandler(previous_handler_);
}

namespace {
void IgnoreXErrors(Display* display, XErrorEvent* error) {}
}  // namespace

// Static
ScopedXErrorHandler::Handler ScopedXErrorHandler::Ignore() {
  return base::Bind(IgnoreXErrors);
}

int ScopedXErrorHandler::HandleXErrors(Display* display, XErrorEvent* error) {
  DCHECK(g_handler != nullptr);
  g_handler->ok_ = false;
  g_handler->handler_.Run(display, error);
  return 0;
}


ScopedXGrabServer::ScopedXGrabServer(Display* display)
    : display_(display) {
  XGrabServer(display_);
}

ScopedXGrabServer::~ScopedXGrabServer() {
  XUngrabServer(display_);
  XFlush(display_);
}


bool IgnoreXServerGrabs(Display* display, bool ignore) {
  int test_event_base = 0;
  int test_error_base = 0;
  int major = 0;
  int minor = 0;
  if (!XTestQueryExtension(display, &test_event_base, &test_error_base,
                           &major, &minor)) {
    return false;
  }

  XTestGrabControl(display, ignore);
  return true;
}

}  // namespace remoting
