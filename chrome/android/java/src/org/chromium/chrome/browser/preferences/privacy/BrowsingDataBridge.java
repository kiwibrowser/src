// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.privacy;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * Communicates between ClearBrowsingData, ImportantSitesUtils (C++) and
 * ClearBrowsingDataPreferences (Java UI).
 */
public final class BrowsingDataBridge {
    private static BrowsingDataBridge sInstance;

    // Object to notify when "clear browsing data" completes.
    private OnClearBrowsingDataListener mClearBrowsingDataListener;

    /**
     * Interface for a class that is listening to clear browser data events.
     */
    public interface OnClearBrowsingDataListener { void onBrowsingDataCleared(); }

    /**
     * Interface for a class that is fetching important site information.
     */
    public interface ImportantSitesCallback {
        /**
         * Called when the list of important registerable domains has been fetched from cpp.
         * See net/base/registry_controlled_domains/registry_controlled_domain.h for more details on
         * registrable domains and the current list of effective eTLDs.
         * @param domains Important registerable domains.
         * @param exampleOrigins Example origins for each domain. These can be used to retrieve
         *                       favicons.
         * @param importantReasons Bitfield of reasons why this domain was selected. Pass this back
         *                         to clearBrowinsgData so we can record metrics.
         * @param dialogDisabled If the important dialog has been ignored too many times and should
         *                       not be shown.
         */
        @CalledByNative("ImportantSitesCallback")
        void onImportantRegisterableDomainsReady(String[] domains, String[] exampleOrigins,
                int[] importantReasons, boolean dialogDisabled);
    }

    /**
     * Interface to a class that receives callbacks instructing it to inform the user about other
     * forms of browsing history.
     */
    public interface OtherFormsOfBrowsingHistoryListener {
        /**
         * Called by the web history service when it discovers that other forms of browsing history
         * exist.
         */
        @CalledByNative("OtherFormsOfBrowsingHistoryListener")
        void enableDialogAboutOtherFormsOfBrowsingHistory();
    }

    private BrowsingDataBridge() {}

    /**
     * @return The singleton bridge object.
     */
    public static BrowsingDataBridge getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) sInstance = new BrowsingDataBridge();
        return sInstance;
    }

    @CalledByNative
    private void browsingDataCleared() {
        if (mClearBrowsingDataListener != null) {
            mClearBrowsingDataListener.onBrowsingDataCleared();
            mClearBrowsingDataListener = null;
        }
    }

    /**
     * Clear the specified types of browsing data asynchronously.
     * |listener| is an object to be notified when clearing completes.
     * It can be null, but many operations (e.g. navigation) are
     * ill-advised while browsing data is being cleared.
     * @param listener A listener to call back when the clearing is finished.
     * @param dataTypes An array of browsing data types to delete, represented as values from
     *                  the shared enum {@link BrowsingDataType}.
     * @param timePeriod The time period for which to delete the data, represented as a value from
     *                   the shared enum {@link TimePeriod}.
     */
    public void clearBrowsingData(
            OnClearBrowsingDataListener listener, int[] dataTypes, int timePeriod) {
        clearBrowsingDataExcludingDomains(listener, dataTypes, timePeriod, new String[0],
                new int[0], new String[0], new int[0]);
    }

    /**
     * Same as above, but now we can specify a list of domains to exclude from clearing browsing
     * data.
     * Do not use this method unless caller knows what they're doing. Not all backends are supported
     * yet, and more data than expected could be deleted. See crbug.com/113621.
     * @param listener A listener to call back when the clearing is finished.
     * @param dataTypes An array of browsing data types to delete, represented as values from
     *                  the shared enum {@link BrowsingDataType}.
     * @param timePeriod The time period for which to delete the data, represented as a value from
     *                   the shared enum {@link TimePeriod}.
     * @param blacklistDomains A list of registerable domains that we don't clear data for.
     * @param blacklistedDomainReasons A list of the reason metadata for the blacklisted domains.
     * @param ignoredDomains A list of ignored domains that the user chose to not blacklist. We use
     *                       these to remove important site entries if the user ignores them enough.
     * @param ignoredDomainReasons A list of reason metadata for the ignored domains.
     */
    public void clearBrowsingDataExcludingDomains(OnClearBrowsingDataListener listener,
            int[] dataTypes, int timePeriod, String[] blacklistDomains,
            int[] blacklistedDomainReasons, String[] ignoredDomains, int[] ignoredDomainReasons) {
        assert mClearBrowsingDataListener == null;
        mClearBrowsingDataListener = listener;
        nativeClearBrowsingData(getProfile(), dataTypes, timePeriod, blacklistDomains,
                blacklistedDomainReasons, ignoredDomains, ignoredDomainReasons);
    }

    /**
     * This fetches sites (registerable domains) that we consider important. This combines many
     * pieces of information, including site engagement and permissions. The callback is called
     * with the list of important registerable domains.
     *
     * See net/base/registry_controlled_domains/registry_controlled_domain.h for more details on
     * registrable domains and the current list of effective eTLDs.
     * @param callback The callback that will be used to set the list of important sites.
     */
    public static void fetchImportantSites(ImportantSitesCallback callback) {
        nativeFetchImportantSites(getProfile(), callback);
    }

    /**
     * @return The maximum number of important sites that will be returned from the call above.
     *         This is a constant that won't change.
     */
    public static int getMaxImportantSites() {
        return nativeGetMaxImportantSites();
    }

    /** This lets us mark an origin as important for testing. */
    @VisibleForTesting
    public static void markOriginAsImportantForTesting(String origin) {
        nativeMarkOriginAsImportantForTesting(getProfile(), origin);
    }

    /**
     * Requests that the web history service finds out if we should inform the user about the
     * existence of other forms of browsing history. The response will be asynchronous, through
     * {@link OtherFormsOfBrowsingHistoryListener}.
     */
    public void requestInfoAboutOtherFormsOfBrowsingHistory(
            OtherFormsOfBrowsingHistoryListener listener) {
        nativeRequestInfoAboutOtherFormsOfBrowsingHistory(getProfile(), listener);
    }

    /**
     * @returns The profile on which all UI-based browsing data operations should be performed,
     *         which is the currently active regular profile.
     */
    private static Profile getProfile() {
        return Profile.getLastUsedProfile().getOriginalProfile();
    }

    private native void nativeClearBrowsingData(Profile profile, int[] dataTypes, int timePeriod,
            String[] blacklistDomains, int[] blacklistedDomainReasons, String[] ignoredDomains,
            int[] ignoredDomainReasons);
    private native void nativeRequestInfoAboutOtherFormsOfBrowsingHistory(
            Profile profile, OtherFormsOfBrowsingHistoryListener listener);
    private static native void nativeFetchImportantSites(
            Profile profile, ImportantSitesCallback callback);
    private static native int nativeGetMaxImportantSites();
    private static native void nativeMarkOriginAsImportantForTesting(
            Profile profile, String origin);
}
