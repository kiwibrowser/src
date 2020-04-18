// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import android.support.annotation.IntDef;

import org.chromium.components.offline_items_collection.OfflineItemFilter;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/** Helper containing a list of Downloads Home filter types and conversion methods. */
public class Filters {
    /** A list of possible filter types on offlined items. */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({NONE, VIDEOS, MUSIC, IMAGES, SITES, OTHER, PREFETCHED})
    public @interface FilterType {}
    public static final int NONE = 0;
    public static final int VIDEOS = 1;
    public static final int MUSIC = 2;
    public static final int IMAGES = 3;
    public static final int SITES = 4;
    public static final int OTHER = 5;
    public static final int PREFETCHED = 6;

    /**
     * Converts from a {@link OfflineItem#filter} to a {@link FilterType}.  Note that not all
     * {@link OfflineItem#filter} types have a corresponding match and may return {@link #NONE}
     * as they don't correspond to any UI filter.
     *
     * @param filter The {@link OfflineItem#filter} type to convert.
     * @return       The corresponding {@link FilterType}.
     */
    public static @FilterType int fromOfflineItem(@OfflineItemFilter int filter) {
        switch (filter) {
            case OfflineItemFilter.FILTER_PAGE:
                return SITES;
            case OfflineItemFilter.FILTER_VIDEO:
                return VIDEOS;
            case OfflineItemFilter.FILTER_AUDIO:
                return MUSIC;
            case OfflineItemFilter.FILTER_IMAGE:
                return IMAGES;
            case OfflineItemFilter.FILTER_OTHER:
            case OfflineItemFilter.FILTER_DOCUMENT:
                return OTHER;
            default:
                return NONE;
        }
    }

    private Filters() {}
}