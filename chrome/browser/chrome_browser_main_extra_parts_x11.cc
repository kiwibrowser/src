// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_main_extra_parts_x11.h"

#include "base/bind.h"
#include "base/debug/debugger.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/common/chrome_result_codes.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"

using content::BrowserThread;

namespace {

// Indicates that we're currently responding to an IO error (by shutting down).
bool g_in_x11_io_error_handler = false;

// Number of seconds to wait for UI thread to get an IO error if we get it on
// the background thread.
const int kWaitForUIThreadSeconds = 10;

int BrowserX11ErrorHandler(Display* d, XErrorEvent* error) {
  if (!g_in_x11_io_error_handler) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&ui::LogErrorEventDescription, d, *error));
  }
  return 0;
}


// This function is used to help us diagnose crash dumps that happen
// during the shutdown process.
NOINLINE void WaitingForUIThreadToHandleIOError() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(kWaitForUIThreadSeconds);
}

int BrowserX11IOErrorHandler(Display* d) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // Wait for the UI thread (which has a different connection to the X server)
    // to get the error. We can't call shutdown from this thread without
    // tripping an error. Doing it through a function so that we'll be able
    // to see it in any crash dumps.
    WaitingForUIThreadToHandleIOError();
    return 0;
  }

  // If there's an IO error it likely means the X server has gone away.
  // If this CHECK fails, then that means SessionEnding() below triggered some
  // code that tried to talk to the X server, resulting in yet another error.
  CHECK(!g_in_x11_io_error_handler);

  g_in_x11_io_error_handler = true;
  LOG(ERROR) << "X IO error received (X server probably went away)";
  chrome::SessionEnding();

  return 0;
}

int X11EmptyErrorHandler(Display* d, XErrorEvent* error) {
  return 0;
}

int X11EmptyIOErrorHandler(Display* d) {
  return 0;
}

}  // namespace

ChromeBrowserMainExtraPartsX11::ChromeBrowserMainExtraPartsX11() {
}

ChromeBrowserMainExtraPartsX11::~ChromeBrowserMainExtraPartsX11() {
}

void ChromeBrowserMainExtraPartsX11::PreEarlyInitialization() {
  // Installs the X11 error handlers for the browser process used during
  // startup. They simply print error messages and exit because
  // we can't shutdown properly while creating and initializing services.
  ui::SetX11ErrorHandlers(NULL, NULL);
}

void ChromeBrowserMainExtraPartsX11::PostMainMessageLoopStart() {
  // Installs the X11 error handlers for the browser process after the
  // main message loop has started. This will allow us to exit cleanly
  // if X exits before us.
  ui::SetX11ErrorHandlers(BrowserX11ErrorHandler, BrowserX11IOErrorHandler);
}

void ChromeBrowserMainExtraPartsX11::PostMainMessageLoopRun() {
  // Unset the X11 error handlers. The X11 error handlers log the errors using a
  // |PostTask()| on the message-loop. But since the message-loop is in the
  // process of terminating, this can cause errors.
  ui::SetX11ErrorHandlers(X11EmptyErrorHandler, X11EmptyIOErrorHandler);
}
