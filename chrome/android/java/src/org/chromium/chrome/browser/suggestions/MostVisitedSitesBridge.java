// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNIAdditionalImport;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.partnercustomizations.HomepageManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.FeatureUtilities;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Methods to bridge into native history to provide most recent urls, titles and thumbnails.
 */
@JNIAdditionalImport(MostVisitedSites.class) // Needed for the Observer usage in the native calls.
public class MostVisitedSitesBridge
        implements MostVisitedSites, HomepageManager.HomepageStateListener {
    /**
     * Maximum number of tiles that is explicitly supported. UMA relies on this value, so even if
     * the UI supports it, getting more can raise unexpected issues.
     */
    public static final int MAX_TILE_COUNT = 12;

    private long mNativeMostVisitedSitesBridge;

    private MostVisitedSites.Observer mWrappedObserver;

    /**
     * MostVisitedSites constructor requires a valid user profile object.
     *
     * @param profile The profile for which to fetch most visited sites.
     */
    public MostVisitedSitesBridge(Profile profile) {
        mNativeMostVisitedSitesBridge = nativeInit(profile);
        // The first tile replaces the home page button (only) in Chrome Home. To support that,
        // provide information about the home page.
        if (FeatureUtilities.isChromeHomeEnabled()) {
            nativeSetHomePageClient(mNativeMostVisitedSitesBridge, new HomePageClient() {
                @Override
                public boolean isHomePageEnabled() {
                    return HomepageManager.isHomepageEnabled();
                }

                @Override
                public boolean isNewTabPageUsedAsHomePage() {
                    return NewTabPage.isNTPUrl(getHomePageUrl());
                }

                @Override
                public String getHomePageUrl() {
                    return HomepageManager.getHomepageUri();
                }
            });
            HomepageManager.getInstance().addListener(this);
        }
    }

    /**
     * Cleans up the C++ side of this class. This instance must not be used after calling destroy().
     */
    @Override
    public void destroy() {
        // Stop listening even if it was not started in the first place. (Handled without errors.)
        HomepageManager.getInstance().removeListener(this);
        assert mNativeMostVisitedSitesBridge != 0;
        nativeDestroy(mNativeMostVisitedSitesBridge);
        mNativeMostVisitedSitesBridge = 0;
    }

    @Override
    public void setObserver(Observer observer, int numSites) {
        assert numSites <= MAX_TILE_COUNT;
        mWrappedObserver = observer;

        nativeSetObserver(mNativeMostVisitedSitesBridge, this, numSites);
    }

    @Override
    public void addBlacklistedUrl(String url) {
        nativeAddOrRemoveBlacklistedUrl(mNativeMostVisitedSitesBridge, url, true);
    }

    @Override
    public void removeBlacklistedUrl(String url) {
        nativeAddOrRemoveBlacklistedUrl(mNativeMostVisitedSitesBridge, url, false);
    }

    @Override
    public void recordPageImpression(int tilesCount) {
        nativeRecordPageImpression(mNativeMostVisitedSitesBridge, tilesCount);
    }

    @Override
    public void recordTileImpression(Tile tile) {
        nativeRecordTileImpression(mNativeMostVisitedSitesBridge, tile.getIndex(), tile.getType(),
                tile.getIconType(), tile.getTitleSource(), tile.getSource(),
                tile.getData().dataGenerationTime.getTime(), tile.getUrl());
    }

    @Override
    public void recordOpenedMostVisitedItem(Tile tile) {
        nativeRecordOpenedMostVisitedItem(mNativeMostVisitedSitesBridge, tile.getIndex(),
                tile.getType(), tile.getTitleSource(), tile.getSource(),
                tile.getData().dataGenerationTime.getTime());
    }

    @Override
    public void onHomepageStateUpdated() {
        assert mNativeMostVisitedSitesBridge != 0;
        // Ensure even a blacklisted home page can be set as tile when (re-)enabling it.
        if (HomepageManager.isHomepageEnabled()) {
            removeBlacklistedUrl(HomepageManager.getHomepageUri());
        }
        nativeOnHomePageStateChanged(mNativeMostVisitedSitesBridge);
    }

    /**
     * Utility function to convert JNI friendly site suggestion data to a Java friendly list of
     * {@link SiteSuggestion}s.
     */
    public static List<SiteSuggestion> buildSiteSuggestions(String[] titles, String[] urls,
            int[] sections, String[] whitelistIconPaths, int[] titleSources, int[] sources,
            long[] dataGenerationTimesMs) {
        List<SiteSuggestion> siteSuggestions = new ArrayList<>(titles.length);
        for (int i = 0; i < titles.length; ++i) {
            siteSuggestions.add(new SiteSuggestion(titles[i], urls[i], whitelistIconPaths[i],
                    titleSources[i], sources[i], sections[i], new Date(dataGenerationTimesMs[i])));
        }
        return siteSuggestions;
    }

    /**
     * This is called when the list of most visited URLs is initially available or updated.
     * Parameters guaranteed to be non-null.
     *
     * @param titles Array of most visited url page titles.
     * @param urls Array of most visited URLs, including popular URLs if
     *             available and necessary (i.e. there aren't enough most
     *             visited URLs).
     * @param whitelistIconPaths The paths to the icon image files for whitelisted tiles, empty
     *                           strings otherwise.
     * @param sources For each tile, the {@code TileSource} that generated the tile.
     */
    @CalledByNative
    private void onURLsAvailable(String[] titles, String[] urls, int[] sections,
            String[] whitelistIconPaths, int[] titleSources, int[] sources,
            long[] dataGenerationTimesMs) {
        // Don't notify observer if we've already been destroyed.
        if (mNativeMostVisitedSitesBridge == 0) return;

        List<SiteSuggestion> suggestions = new ArrayList<>();

        suggestions.addAll(buildSiteSuggestions(titles, urls, sections, whitelistIconPaths,
                titleSources, sources, dataGenerationTimesMs));

        mWrappedObserver.onSiteSuggestionsAvailable(suggestions);
    }

    private boolean allSuggestionsArePersonalized(List<SiteSuggestion> suggestions) {
        for (SiteSuggestion suggestion : suggestions) {
            if (suggestion.sectionType != TileSectionType.PERSONALIZED) return false;
        }
        return true;
    }

    /**
     * This is called when a previously uncached icon has been fetched.
     * Parameters guaranteed to be non-null.
     *
     * @param siteUrl URL of site with newly-cached icon.
     */
    @CalledByNative
    private void onIconMadeAvailable(String siteUrl) {
        // Don't notify observer if we've already been destroyed.
        if (mNativeMostVisitedSitesBridge != 0) {
            mWrappedObserver.onIconMadeAvailable(siteUrl);
        }
    }

    private native long nativeInit(Profile profile);
    private native void nativeDestroy(long nativeMostVisitedSitesBridge);
    private native void nativeOnHomePageStateChanged(long nativeMostVisitedSitesBridge);
    private native void nativeSetObserver(
            long nativeMostVisitedSitesBridge, MostVisitedSitesBridge observer, int numSites);
    private native void nativeSetHomePageClient(
            long nativeMostVisitedSitesBridge, MostVisitedSites.HomePageClient homePageClient);
    private native void nativeAddOrRemoveBlacklistedUrl(
            long nativeMostVisitedSitesBridge, String url, boolean addUrl);
    private native void nativeRecordPageImpression(
            long nativeMostVisitedSitesBridge, int tilesCount);
    private native void nativeRecordTileImpression(long nativeMostVisitedSitesBridge, int index,
            int type, int iconType, int titleSource, int source, long dataGenerationTimeMs,
            String url);
    private native void nativeRecordOpenedMostVisitedItem(long nativeMostVisitedSitesBridge,
            int index, int tileType, int titleSource, int source, long dataGenerationTimeMs);
}
