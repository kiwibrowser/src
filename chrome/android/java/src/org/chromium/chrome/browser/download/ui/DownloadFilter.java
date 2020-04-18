// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.support.annotation.IntDef;
import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.UrlConstants;

import java.util.Locale;

/**
 * A class holding constants and convenience methods about filters and their corresponding
 * resources.
 */
public class DownloadFilter {
    // These statics are used for UMA logging. Please update the AndroidDownloadFilterType enum in
    // histograms.xml if these change.
    @IntDef({FILTER_ALL, FILTER_PAGE, FILTER_VIDEO, FILTER_AUDIO, FILTER_IMAGE, FILTER_DOCUMENT,
            FILTER_OTHER, FILTER_BOUNDARY})
    public @interface Type {}

    public static final int FILTER_ALL = 0;
    public static final int FILTER_PAGE = 1;
    public static final int FILTER_VIDEO = 2;
    public static final int FILTER_AUDIO = 3;
    public static final int FILTER_IMAGE = 4;
    public static final int FILTER_DOCUMENT = 5;
    public static final int FILTER_OTHER = 6;
    public static final int FILTER_BOUNDARY = 7;

    private static final String MIMETYPE_VIDEO = "video";
    private static final String MIMETYPE_AUDIO = "audio";
    private static final String MIMETYPE_IMAGE = "image";
    private static final String MIMETYPE_DOCUMENT = "text";

    /**
     * Icons and labels for each filter in the menu.
     *
     * Changing the ordering of these items requires changing the FILTER_* values in
     * {@link DownloadHistoryAdapter} and the values in mCanonicalDirectoryPairs in
     * {@link DownloadDirectoryList}.
     */
    static final int[][] FILTER_LIST = new int[][] {
            {R.drawable.ic_file_download_24dp, R.string.download_manager_ui_all_downloads},
            {R.drawable.ic_globe_24dp, R.string.download_manager_ui_pages},
            {R.drawable.ic_videocam_24dp, R.string.download_manager_ui_video},
            {R.drawable.ic_music_note_24dp, R.string.download_manager_ui_audio},
            {R.drawable.ic_drive_image_24dp, R.string.download_manager_ui_images},
            {R.drawable.ic_drive_document_24dp, R.string.download_manager_ui_documents},
            {R.drawable.ic_drive_file_24dp, R.string.download_manager_ui_other}};

    private static final String TAG = "download_ui";

    /**
     * @return The number of filters that exist.
     */
    static int getFilterCount() {
        return FILTER_LIST.length;
    }

    /**
     * @return The drawable id representing the given filter.
     */
    public static int getDrawableForFilter(@Type int filter) {
        return FILTER_LIST[filter][0];
    }

    /**
     * @return The resource id of the title representing the given filter.
     */
    public static int getStringIdForFilter(@Type int filter) {
        return FILTER_LIST[filter][1];
    }

    /**
     * @return The URL representing the filter.
     */
    public static String getUrlForFilter(@Type int filter) {
        if (filter == FILTER_ALL) {
            return UrlConstants.DOWNLOADS_URL;
        }
        return UrlConstants.DOWNLOADS_FILTER_URL + filter;
    }

    /**
     * @return The filter that the given URL represents.
     */
    public static @Type int getFilterFromUrl(String url) {
        if (TextUtils.isEmpty(url) || UrlConstants.DOWNLOADS_HOST.equals(url)) return FILTER_ALL;
        int result = FILTER_ALL;
        if (url.startsWith(UrlConstants.DOWNLOADS_FILTER_URL)) {
            try {
                result = Integer
                        .parseInt(url.substring(UrlConstants.DOWNLOADS_FILTER_URL.length()));
            } catch (NumberFormatException e) {
                Log.e(TAG, "Url parsing failed.");
            }
        }
        return result;
    }

    /** Identifies the type of file represented by the given MIME type string. */
    public static @Type int fromMimeType(String mimeType) {
        if (TextUtils.isEmpty(mimeType)) return DownloadFilter.FILTER_OTHER;

        String[] pieces = mimeType.toLowerCase(Locale.getDefault()).split("/");
        if (pieces.length != 2) return DownloadFilter.FILTER_OTHER;

        if (MIMETYPE_VIDEO.equals(pieces[0])) {
            return DownloadFilter.FILTER_VIDEO;
        } else if (MIMETYPE_AUDIO.equals(pieces[0])) {
            return DownloadFilter.FILTER_AUDIO;
        } else if (MIMETYPE_IMAGE.equals(pieces[0])) {
            return DownloadFilter.FILTER_IMAGE;
        } else if (MIMETYPE_DOCUMENT.equals(pieces[0])) {
            return DownloadFilter.FILTER_DOCUMENT;
        } else {
            return DownloadFilter.FILTER_OTHER;
        }
    }
}
