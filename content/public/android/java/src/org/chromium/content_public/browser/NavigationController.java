// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.base.VisibleForTesting;

/**
 * The NavigationController Java wrapper to allow communicating with the native
 * NavigationController object.
 */
public interface NavigationController {
    /**
     * @return Whether back navigation is possible from the "current entry".
     */
    boolean canGoBack();

    /**
     * @return Whether forward navigation is possible from the "current entry".
     */
    boolean canGoForward();

    /**
     * @param offset The offset into the navigation history.
     * @return Whether we can move in history by given offset
     */
    boolean canGoToOffset(int offset);

    /**
     * Navigates to the specified offset from the "current entry". Does nothing if the offset is
     * out of bounds.
     * @param offset The offset into the navigation history.
     */
    void goToOffset(int offset);

    /**
     * Navigates to the specified index in the navigation entry for this page.
     * @param index The navigation index to navigate to.
     */
    void goToNavigationIndex(int index);

    /**
     * Goes to the navigation entry before the current one.
     */
    void goBack();

    /**
     * Goes to the navigation entry following the current one.
     */
    void goForward();

    /**
     * @return Whether the tab is navigating to the URL the tab is opened with.
     */
    boolean isInitialNavigation();

    /**
     * Loads the current navigation if there is a pending lazy load (after tab restore).
     */
    public void loadIfNecessary();

    /**
     * @return Whether a reload has been requested.
     */
    public boolean needsReload();

    /**
     * Requests the current navigation to be loaded upon the next call to loadIfNecessary().
     */
    public void setNeedsReload();

    /**
     * Reload the current page.
     */
    public void reload(boolean checkForRepost);

    /**
     * Reload the current page, bypassing the contents of the cache.
     */
    public void reloadBypassingCache(boolean checkForRepost);

    /**
     * Cancel the pending reload.
     */
    public void cancelPendingReload();

    /**
     * Continue the pending reload.
     */
    public void continuePendingReload();

    /**
     * Load url without fixing up the url string. Consumers of NavigationController are
     * responsible for ensuring the URL passed in is properly formatted (i.e. the
     * scheme has been added if left off during user input).
     * @param params Parameters for this load.
     */
    public void loadUrl(LoadUrlParams params);

    /**
     * Clears NavigationController's page history in both backwards and
     * forwards directions.
     */
    @VisibleForTesting
    public void clearHistory();

    /**
     * Get a copy of the navigation history of NavigationController.
     * @return navigation history of NavigationController.
     */
    public NavigationHistory getNavigationHistory();

    /**
    * Get the navigation history of NavigationController from current navigation entry index
    * with direction (forward/backward)
    * @param isForward determines forward or backward from current index
    * @param itemLimit maximum number of entries to be retrieved in specified
    * diection.
    * @return navigation history by keeping above constraints.
    */
    public NavigationHistory getDirectedNavigationHistory(boolean isForward, int itemLimit);

    /**
     * Get Original URL for current Navigation entry of NavigationController.
     * @return The original request URL for the current navigation entry, or null if there is no
     *         current entry.
     */
    public String getOriginalUrlForVisibleNavigationEntry();

    /**
     * Clears SSL preferences for this NavigationController.
     */
    public void clearSslPreferences();

    /**
     * Get whether or not we're using a desktop user agent for the currently loaded page.
     * @return true, if use a desktop user agent and false for a mobile one.
     */
    public boolean getUseDesktopUserAgent();

    /**
     * Set whether or not we're using a desktop user agent for the currently loaded page.
     * @param override If true, use a desktop user agent.  Use a mobile one otherwise.
     * @param reloadOnChange Reload the page if the UA has changed.
     */
    public void setUseDesktopUserAgent(boolean override, boolean reloadOnChange);

    /**
     * Return the NavigationEntry at the given index.
     * @param index Index to retrieve the NavigationEntry for.
     * @return Entry containing info about the navigation, null if the index is out of bounds.
     */
    @VisibleForTesting
    public NavigationEntry getEntryAtIndex(int index);

    /**
     * @return The pending {@link NavigationEntry} for this controller or {@code null} if none
     *         exists.
     */
    public NavigationEntry getPendingEntry();

    /**
     * @return The index of the last committed entry.
     */
    public int getLastCommittedEntryIndex();

    /**
     * Removes the entry at the specified |index|.
     * @return false, if the index is the last committed index or the pending entry. Otherwise this
     *         call discards any transient or pending entries.
     */
    public boolean removeEntryAtIndex(int index);

    /**
     * @return Whether it is safe to call CopyStateFrom (i.e. the navigation state is empty).
     */
    public boolean canCopyStateOver();

    /**
     * @return Whether it is safe to call CopyStateFromAndPrune.
     */
    public boolean canPruneAllButLastCommitted();

    /**
     * Copies the navigation state from the given controller to this one. This one should be empty.
     * @param source A source of the navigation state
     * @param needsReload Indicates whether a reload needs to happen when activated.
     */
    public void copyStateFrom(NavigationController source, boolean needsReload);

    /**
     * A variant of CopyStateFrom. Removes all entries from this except the last committed entry,
     * and inserts all entries from |source| before and including its last committed entry.
     * See navigation_controller.h for more detailed description.
     * @param source A source of the navigation state
     * @param replaceEntry Whether to replace the current entry in source
     */
    public void copyStateFromAndPrune(NavigationController source, boolean replaceEntry);

    /**
     * Gets extra data on the {@link NavigationEntry} at {@code index}.
     * @param index The index of the navigation entry.
     * @param key The data key.
     * @return The data value, or null if not found.
     */
    String getEntryExtraData(int index, String key);

    /**
     * Sets extra data on the {@link NavigationEntry} at {@code index}.
     * @param index The index of the navigation entry.
     * @param key The data key.
     * @param value The data value.
     */
    void setEntryExtraData(int index, String key, String value);
}
