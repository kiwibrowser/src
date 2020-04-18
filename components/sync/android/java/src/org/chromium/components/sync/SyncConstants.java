// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync;

/** This class stores constants that need to be in org.chromium.components.sync. */
public final class SyncConstants {
    // This should always have the same value as GaiaConstants::kChromeSyncOAuth2Scope.
    public static final String CHROME_SYNC_OAUTH2_SCOPE =
            "https://www.googleapis.com/auth/chromesync";

    // This should always have the same value as TabNodePool::kInvalidTabNodeID.
    public static final int INVALID_TAB_NODE_ID = -1;

    private SyncConstants() {}
}
