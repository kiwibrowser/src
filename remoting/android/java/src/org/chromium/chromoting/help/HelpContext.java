// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting.help;

/**
 * Enumeration of contexts from which the user could request help in the application. The
 * HelpAndFeedback implementation is responsible for displaying an appropriate Help article for
 * each context.
 */
public enum HelpContext {
  // Help for the host-list screen.
  HOST_LIST,

  // Help on setting up a new host.
  HOST_SETUP,

  // Help for the connected desktop screen, including touch gestures.
  DESKTOP,
}
