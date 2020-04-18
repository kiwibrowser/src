// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import org.chromium.chrome.browser.download.home.filter.Filters.FilterType;
import org.chromium.components.offline_items_collection.OfflineItem;

/**
 * An {@link OfflineItemFilter} responsible for pruning out items based on
 * {@link OfflineItem#filter} and {@link FilterType}.
 */
public class TypeOfflineItemFilter extends OfflineItemFilter {
    private @FilterType int mFilter = Filters.NONE;

    /** Creates an instance of this filter and wraps {@code source}. */
    public TypeOfflineItemFilter(OfflineItemFilterSource source) {
        super(source);
        onFilterChanged();
    }

    public void onFilterSelected(@FilterType int filter) {
        mFilter = filter;
        onFilterChanged();
    }

    // OfflineItemFilter implementation.
    @Override
    protected boolean isFilteredOut(OfflineItem item) {
        int requiredFilter = Filters.fromOfflineItem(item.filter);

        // Filter out based on prefetch suggestions before resorting to other types.
        if (mFilter == Filters.PREFETCHED) return !item.isSuggested;
        if (item.isSuggested) return mFilter != Filters.PREFETCHED;
        if (mFilter == Filters.NONE) return false;
        if (mFilter == requiredFilter) return false;

        return true;
    }
}