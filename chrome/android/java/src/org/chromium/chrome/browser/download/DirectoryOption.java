// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.support.annotation.IntDef;

import org.chromium.base.metrics.RecordHistogram;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Denotes a given option for directory selection; includes name, location, and space.
 */
public class DirectoryOption {
    // Type to track user's selection of directory option. This enum is used in histogram and must
    // match DownloadLocationDirectoryType in enums.xml, so don't delete or reuse values.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({DEFAULT_OPTION, ADDITIONAL_OPTION, ERROR_OPTION, OPTION_COUNT})
    public @interface DownloadLocationDirectoryType {}

    public static final int DEFAULT_OPTION = 0;
    public static final int ADDITIONAL_OPTION = 1;
    public static final int ERROR_OPTION = 2;
    public static final int OPTION_COUNT = 3;

    /**
     * Name of the current download directory.
     */
    public final String name;

    /**
     * The absolute path of the download location.
     */
    public final String location;

    /**
     * The available space in this download directory.
     */
    public final long availableSpace;

    /**
     * The total disk space of the partition.
     */
    public final long totalSpace;

    /**
     * The type of the directory option.
     */
    public final int type;

    public DirectoryOption(String name, String location, long availableSpace, long totalSpace,
            @DownloadLocationDirectoryType int type) {
        this.name = name;
        this.location = location;
        this.availableSpace = availableSpace;
        this.totalSpace = totalSpace;
        this.type = type;
    }

    /**
     * Records a histogram for this directory option when the user selects this directory option.
     */
    public void recordDirectoryOptionType() {
        RecordHistogram.recordEnumeratedHistogram("MobileDownload.Location.Setting.DirectoryType",
                type, DirectoryOption.OPTION_COUNT);
    }
}
