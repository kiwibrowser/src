// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.text.TextUtils;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Multiple download items may reference the same location on disk. This class maintains a mapping
 * of file paths to download items that reference the file path.
 * TODO(twellington): remove this class after the backend handles duplicate removal.
 */
class FilePathsToDownloadItemsMap {
    private final Map<String, Set<DownloadHistoryItemWrapper>> mMap = new HashMap<>();

    /**
     * Adds a DownloadHistoryItemWrapper to the map if it has a valid path.
     * @param wrapper The item to add to the map.
     */
    void addItem(DownloadHistoryItemWrapper wrapper) {
        if (TextUtils.isEmpty(wrapper.getFilePath())) return;

        if (!mMap.containsKey(wrapper.getFilePath())) {
            mMap.put(wrapper.getFilePath(), new HashSet<DownloadHistoryItemWrapper>());
        }
        mMap.get(wrapper.getFilePath()).add(wrapper);
    }

    /**
     * Removes a DownloadHistoryItemWrapper from the map. Does nothing if the item does not exist in
     * the map.
     * @param wrapper The item to remove from the map.
     */
    void removeItem(DownloadHistoryItemWrapper wrapper) {
        Set<DownloadHistoryItemWrapper> matchingItems = mMap.get(wrapper.getFilePath());
        if (matchingItems == null || !matchingItems.contains(wrapper)) return;

        if (matchingItems.size() == 1) {
            // If this is the only DownloadHistoryItemWrapper that references the file path,
            // remove the file path from the map.
            mMap.remove(wrapper.getFilePath());
        } else {
            matchingItems.remove(wrapper);
        }
    }

    /**
     * Gets all DownloadHistoryItemWrappers that point to the same path in the user's storage.
     * @param filePath The file path used to retrieve items.
     * @return DownloadHistoryItemWrappers associated with filePath.
     */
    Set<DownloadHistoryItemWrapper> getItemsForFilePath(String filePath) {
        return mMap.get(filePath);
    }
}
