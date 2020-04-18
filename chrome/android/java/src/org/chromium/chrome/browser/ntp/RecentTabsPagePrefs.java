// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import org.chromium.chrome.browser.ntp.ForeignSessionHelper.ForeignSession;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * Allows Java code to read and modify preferences related to the {@link RecentTabsPage}.
 */
class RecentTabsPagePrefs {
    private long mNativePrefs;

    /**
     * Initialize this class with the given profile.
     * @param profile Profile that will be used for syncing.
     */
    RecentTabsPagePrefs(Profile profile) {
        mNativePrefs = nativeInit(profile);
    }

    /**
     * Clean up the C++ side of this class. After the call, this class instance shouldn't be used.
     */
    void destroy() {
        assert mNativePrefs != 0;
        nativeDestroy(mNativePrefs);
        mNativePrefs = 0;
    }

    /**
     * Sets whether the list of snapshot documents is collapsed (vs expanded) on the Recent Tabs
     * page.
     * @param isCollapsed Whether we want the snapshot documents list to be collapsed.
     */
    void setSnapshotDocumentCollapsed(boolean isCollapsed) {
        nativeSetSnapshotDocumentCollapsed(mNativePrefs, isCollapsed);
    }

    /**
     * Gets whether the list of snapshot documents is collapsed (vs expanded) on
     * the Recent Tabs page.
     * @return Whether the list of snapshot documents is collapsed (vs expanded) on
     *         the Recent Tabs page.
     */
    boolean getSnapshotDocumentCollapsed() {
        return nativeGetSnapshotDocumentCollapsed(mNativePrefs);
    }

    /**
     * Sets whether the list of recently closed tabs is collapsed (vs expanded) on the Recent Tabs
     * page.
     * @param isCollapsed Whether we want the recently closed tabs list to be collapsed.
     */
    void setRecentlyClosedTabsCollapsed(boolean isCollapsed) {
        nativeSetRecentlyClosedTabsCollapsed(mNativePrefs, isCollapsed);
    }

    /**
     * Gets whether the list of recently closed tabs is collapsed (vs expanded) on
     * the Recent Tabs page.
     * @return Whether the list of recently closed tabs is collapsed (vs expanded) on
     *         the Recent Tabs page.
     */
    boolean getRecentlyClosedTabsCollapsed() {
        return nativeGetRecentlyClosedTabsCollapsed(mNativePrefs);
    }

    /**
     * Sets whether sync promo is collapsed (vs expanded) on the Recent Tabs page.
     * @param isCollapsed Whether we want the sync promo to be collapsed.
     */
    void setSyncPromoCollapsed(boolean isCollapsed) {
        nativeSetSyncPromoCollapsed(mNativePrefs, isCollapsed);
    }

    /**
     * Gets whether sync promo is collapsed (vs expanded) on the Recent Tabs page.
     * @return Whether the sync promo is collapsed (vs expanded) on the Recent Tabs page.
     */
    boolean getSyncPromoCollapsed() {
        return nativeGetSyncPromoCollapsed(mNativePrefs);
    }

    /**
     * Sets whether the given foreign session is collapsed (vs expanded) on the Recent Tabs page.
     * @param session Session to set collapsed or expanded.
     * @param isCollapsed Whether we want the foreign session to be collapsed.
     */
    void setForeignSessionCollapsed(ForeignSession session, boolean isCollapsed) {
        nativeSetForeignSessionCollapsed(mNativePrefs, session.tag, isCollapsed);
    }

    /**
     * Gets whether the given foreign session is collapsed (vs expanded) on the Recent Tabs page.
     * @param  session Session to fetch collapsed state.
     * @return Whether the given foreign session is collapsed (vs expanded) on the Recent Tabs page.
     */
    boolean getForeignSessionCollapsed(ForeignSession session) {
        return nativeGetForeignSessionCollapsed(mNativePrefs, session.tag);
    }

    private static native long nativeInit(Profile profile);
    private static native void nativeDestroy(long nativeRecentTabsPagePrefs);
    private static native void nativeSetSnapshotDocumentCollapsed(
            long nativeRecentTabsPagePrefs, boolean isCollapsed);
    private static native boolean nativeGetSnapshotDocumentCollapsed(
            long nativeRecentTabsPagePrefs);
    private static native void nativeSetRecentlyClosedTabsCollapsed(
            long nativeRecentTabsPagePrefs, boolean isCollapsed);
    private static native boolean nativeGetRecentlyClosedTabsCollapsed(
            long nativeRecentTabsPagePrefs);
    private static native void nativeSetSyncPromoCollapsed(
            long nativeRecentTabsPagePrefs, boolean isCollapsed);
    private static native boolean nativeGetSyncPromoCollapsed(long nativeRecentTabsPagePrefs);
    private static native void nativeSetForeignSessionCollapsed(
            long nativeRecentTabsPagePrefs, String sessionTag, boolean isCollapsed);
    private static native boolean nativeGetForeignSessionCollapsed(
            long nativeRecentTabsPagePrefs, String sessionTag);
}
