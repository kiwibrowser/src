// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Don't include this file from any .h files because it pulls in some X headers.

#ifndef REMOTING_HOST_LINUX_X_SERVER_CLIPBOARD_H_
#define REMOTING_HOST_LINUX_X_SERVER_CLIPBOARD_H_

#include <set>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "ui/gfx/x/x11.h"

namespace remoting {

// A class to allow manipulation of the X clipboard, using only X API calls.
// This class is not thread-safe, so all its methods must be called on the
// application's main event-processing thread.
class XServerClipboard {
 public:
  // Called when new clipboard data has been received from the owner of the X
  // selection (primary or clipboard).
  // |mime_type| is the MIME type associated with the data. This will be one of
  // the types listed in remoting/base/constants.h.
  // |data| is the clipboard data from the associated X event, encoded with the
  // specified MIME-type.
  typedef base::Callback<void(const std::string& mime_type,
                              const std::string& data)>
      ClipboardChangedCallback;

  XServerClipboard();
  ~XServerClipboard();

  // Start monitoring |display|'s selections, and invoke |callback| whenever
  // their content changes. The caller must ensure |display| is still valid
  // whenever any other methods are called on this object.
  void Init(Display* display, const ClipboardChangedCallback& callback);

  // Copy data to the X Clipboard.  This acquires ownership of the
  // PRIMARY and CLIPBOARD selections.
  void SetClipboard(const std::string& mime_type, const std::string& data);

  // Process |event| if it is an X selection notification. The caller should
  // invoke this for every event it receives from |display|.
  void ProcessXEvent(XEvent* event);

 private:
  // Handlers called by ProcessXEvent() for each event type.
  void OnSetSelectionOwnerNotify(Atom selection, Time timestamp);
  void OnPropertyNotify(XEvent* event);
  void OnSelectionNotify(XEvent* event);
  void OnSelectionRequest(XEvent* event);
  void OnSelectionClear(XEvent* event);

  // Used by OnSelectionRequest() to respond to requests for details of our
  // clipboard content. This is done by changing the property |property| of the
  // |requestor| window (these values come from the XSelectionRequestEvent).
  // |target| must be a string type (STRING or UTF8_STRING).
  void SendTargetsResponse(Window requestor, Atom property);
  void SendTimestampResponse(Window requestor, Atom property);
  void SendStringResponse(Window requestor, Atom property, Atom target);

  // Called by OnSelectionNotify() when the selection owner has replied to a
  // request for information about a selection.
  // |event| is the raw X event from the notification.
  // |type|, |format| etc are the results from XGetWindowProperty(), or 0 if
  // there is no associated data.
  void HandleSelectionNotify(XSelectionEvent* event,
                             Atom type,
                             int format,
                             int item_count,
                             void* data);

  // These methods return true if selection processing is complete, false
  // otherwise. They are called from HandleSelectionNotify(), and take the same
  // arguments.
  bool HandleSelectionTargetsEvent(XSelectionEvent* event,
                                   int format,
                                   int item_count,
                                   void* data);
  bool HandleSelectionStringEvent(XSelectionEvent* event,
                                  int format,
                                  int item_count,
                                  void* data);

  // Notify the registered callback of new clipboard text.
  void NotifyClipboardText(const std::string& text);

  // These methods trigger the X server or selection owner to send back an
  // event containing the requested information.
  void RequestSelectionTargets(Atom selection);
  void RequestSelectionString(Atom selection, Atom target);

  // Assert ownership of the specified |selection|.
  void AssertSelectionOwnership(Atom selection);
  bool IsSelectionOwner(Atom selection);

  // Stores the Display* supplied to Init().
  Display* display_;

  // Window through which clipboard events are received, or BadValue if the
  // window could not be created.
  Window clipboard_window_;

  // The event base returned by XFixesQueryExtension(). If XFixes is
  // unavailable, the clipboard window will not be created, and no
  // event-processing will take place.
  int xfixes_event_base_;

  // Cached atoms for various strings, initialized during Init().
  Atom clipboard_atom_;
  Atom large_selection_atom_;
  Atom selection_string_atom_;
  Atom targets_atom_;
  Atom timestamp_atom_;
  Atom utf8_string_atom_;

  // The set of X selections owned by |clipboard_window_| (can be Primary or
  // Clipboard or both).
  std::set<Atom> selections_owned_;

  // Clipboard content to return to other applications when |clipboard_window_|
  // owns a selection.
  std::string data_;

  // Stores the property to use for large transfers, or None if a large
  // transfer is not currently in-progress.
  Atom large_selection_property_;

  // Remembers the start time of selection processing, and is set to null when
  // processing is complete. This is used to decide whether to begin processing
  // a new selection or continue with the current selection.
  base::TimeTicks get_selections_time_;

  // |callback| argument supplied to Init().
  ClipboardChangedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(XServerClipboard);
};

}  // namespace remoting

#endif  // REMOTING_HOST_LINUX_X_SERVER_CLIPBOARD_H_
