// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.text.TextUtils;

import org.chromium.chrome.browser.widget.DateDividedAdapter.TimedItem;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

/**
 * Stores a List of DownloadHistoryItemWrappers for a particular download backend.
 */
public abstract class BackendItems extends ArrayList<DownloadHistoryItemWrapper> {
    /** See {@link #findItemIndex}. */
    public static final int INVALID_INDEX = -1;

    /** Whether or not the list has been initialized. */
    private boolean mIsInitialized;

    /**
     * Determines how many bytes are occupied by completed downloads.
     * @return Total size of completed downloads in bytes.
     */
    public long getTotalBytes() {
        long totalSize = 0;
        HashSet<String> filePaths = new HashSet<>();
        for (DownloadHistoryItemWrapper item : this) {
            String path = item.getFilePath();
            if (item.isVisibleToUser(DownloadFilter.FILTER_ALL) && !filePaths.contains(path)) {
                totalSize += item.getFileSize();
            }
            if (path != null && !path.isEmpty()) filePaths.add(path);
        }
        return totalSize;
    }

    /**
     * TODO(shaktisahu) : Remove this when not needed.
     * Filters out items that match the query and are displayed in this list for the current filter.
     * @param filterType    Filter to use.
     * @param query         The text to match.
     * @param filteredItems List for appending items that match the filter.
     */
    public void filter(int filterType, String query, List<TimedItem> filteredItems) {
        if (TextUtils.isEmpty(query)) {
            filter(filterType, filteredItems);
            return;
        }

        for (DownloadHistoryItemWrapper item : this) {
            query = query.toLowerCase(Locale.getDefault());
            Locale locale = Locale.getDefault();
            if (item.isVisibleToUser(filterType)
                    && (item.getDisplayHostname().toLowerCase(locale).contains(query)
                    || item.getDisplayFileName().toLowerCase(locale).contains(query))) {
                filteredItems.add(item);
            }
        }
    }

    /**
     * Search for an existing entry with the given ID.
     * @param guid GUID of the entry.
     * @return The index of the item, or INVALID_INDEX if it couldn't be found.
     */
    public int findItemIndex(String guid) {
        for (int i = 0; i < size(); i++) {
            if (TextUtils.equals(get(i).getId(), guid)) return i;
        }
        return INVALID_INDEX;
    }

    /**
     * Removes the item matching the given guid.
     * @param guid GUID of the download to remove.
     * @return Item that was removed, or null if the item wasn't found.
     */
    public DownloadHistoryItemWrapper removeItem(String guid) {
        int index = findItemIndex(guid);
        if (index == INVALID_INDEX) return null;
        return remove(index);
    }

    public boolean isInitialized() {
        return mIsInitialized;
    }

    public void setIsInitialized() {
        mIsInitialized = true;
    }

    /**
     * Filters out items that are displayed in this list for the current filter.
     *
     * @param filterType    Filter to use.
     * @param filteredItems List for appending items that match the filter.
     */
    private void filter(int filterType, List<TimedItem> filteredItems) {
        for (DownloadHistoryItemWrapper item : this) {
            if (item.isVisibleToUser(filterType)) filteredItems.add(item);
        }
    }
}
