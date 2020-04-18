// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.items;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.offline_items_collection.OfflineContentProvider;

/**
 * Basic factory that creates and returns an {@link OfflineContentProvider} that is attached
 * natively to {@link Profile}.
 */
public class OfflineContentAggregatorFactory {
    private OfflineContentAggregatorFactory() {}

    /**
     * Used to get access to the {@link OfflineContentProvider} associated with {@code profile}.
     * The same {@link OfflineContentProvider} will be returned for the same {@link Profile}.
     * @param profile The {@link Profile} that owns the {@link OfflineContentProvider}.
     * @return An {@link OfflineContentProvider} instance.
     */
    public static OfflineContentProvider forProfile(Profile profile) {
        return nativeGetOfflineContentAggregatorForProfile(profile);
    }

    private static native OfflineContentProvider nativeGetOfflineContentAggregatorForProfile(
            Profile profile);
}