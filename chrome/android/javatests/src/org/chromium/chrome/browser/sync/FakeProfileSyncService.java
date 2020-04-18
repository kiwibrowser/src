// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.sync;

/**
 * Fake some ProfileSyncService methods for testing.
 *
 * Only what has been needed for tests so far has been faked.
 */
public class FakeProfileSyncService extends ProfileSyncService {
    private boolean mEngineInitialized;

    public FakeProfileSyncService() {
        super();
    }

    @Override
    public boolean isEngineInitialized() {
        return mEngineInitialized;
    }

    public void setEngineInitialized(boolean engineInitialized) {
        mEngineInitialized = engineInitialized;
    }

    @Override
    public boolean isUsingSecondaryPassphrase() {
        return true;
    }
}
