// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.base.annotations.CalledByNative;

import java.util.List;

import javax.annotation.Nullable;

/**
 * Methods to provide most recent urls, titles and thumbnails.
 */
public interface MostVisitedSites {
    /**
     * An interface for handling events in {@link MostVisitedSites}.
     */
    interface Observer {
        /** This is called when the list of most visited URLs is initially available or updated. */
        void onSiteSuggestionsAvailable(List<SiteSuggestion> siteSuggestions);

        /**
         * This is called when a previously uncached icon has been fetched.
         * Parameters guaranteed to be non-null.
         *
         * @param siteUrl URL of site with newly-cached icon.
         */
        void onIconMadeAvailable(String siteUrl);
    }

    /**
     * An interface to provide {@link MostVisitedSites} with platform-specific home page data.
     */
    interface HomePageClient {
        /**
         * @return True if a home page is active and set.
         */
        @CalledByNative("HomePageClient")
        boolean isHomePageEnabled();

        /**
         * @return True if the new tab page was set as home page.
         */
        @CalledByNative("HomePageClient")
        boolean isNewTabPageUsedAsHomePage();

        /**
         * @return The raw URL of the currently set home page.
         */
        @CalledByNative("HomePageClient")
        @Nullable
        String getHomePageUrl();
    }

    /**
     * This instance must not be used after calling destroy().
     */
    void destroy();

    /**
     * Sets the recipient for events from {@link MostVisitedSites}. The observer may be notified
     * synchronously or asynchronously.
     * @param observer The observer to be notified.
     * @param numSites The maximum number of sites to return.
     */
    void setObserver(Observer observer, int numSites);

    /**
     * Blacklists a URL from the most visited URLs list.
     */
    void addBlacklistedUrl(String url);

    /**
     * Removes a URL from the most visited URLs blacklist.
     */
    void removeBlacklistedUrl(String url);

    /**
     * Records metrics about an impression of the surface with tiles.
     * @param tilesCount Count of tiles available on the surface at the moment.
     */
    void recordPageImpression(int tilesCount);

    /**
     * Records metrics about an impression of a tile including its source (local, server, ...) and
     * its visual type.
     * @param tile Object holding the details of a tile.
     */
    void recordTileImpression(Tile tile);

    /**
     * Records the opening of a Most Visited Item.
     * @param index The index of the item that was opened.
     * @param type The visual type of the item as defined in {@link TileVisualType}.
     * @param titleSource The {@link TileTitleSource} where the item's title originated from.
     * @param source The {@link TileSource} that generated this item.
     */
    void recordOpenedMostVisitedItem(Tile tile);
}
