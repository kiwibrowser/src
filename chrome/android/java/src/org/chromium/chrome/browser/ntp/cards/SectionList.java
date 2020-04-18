// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsRanker;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.net.NetworkChangeNotifier;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A node in the tree containing a list of all suggestions sections. It listens to changes in the
 * suggestions source and updates the corresponding sections.
 */
public class SectionList
        extends InnerNode implements SuggestionsSource.Observer, SuggestionsSection.Delegate {
    private static final String TAG = "Ntp";

    /** Maps suggestion categories to sections, with stable iteration ordering. */
    private final Map<Integer, SuggestionsSection> mSections = new LinkedHashMap<>();
    /** List of categories that are hidden because they have no content to show. */
    private final Set<Integer> mBlacklistedCategories = new HashSet<>();
    private final SuggestionsUiDelegate mUiDelegate;
    private final OfflinePageBridge mOfflinePageBridge;

    private boolean mHasExternalSections;

    public SectionList(SuggestionsUiDelegate uiDelegate, OfflinePageBridge offlinePageBridge) {
        mUiDelegate = uiDelegate;
        mUiDelegate.getSuggestionsSource().addObserver(this);
        mOfflinePageBridge = offlinePageBridge;

        mUiDelegate.addDestructionObserver(this::removeAllSections);
    }

    /**
     * Returns whether prefetched suggestions metrics should be reported for a given category.
     * @param category given category to check.
     */
    static public boolean shouldReportPrefetchedSuggestionsMetrics(@CategoryInt int category) {
        return category == KnownCategories.ARTICLES && !NetworkChangeNotifier.isOnline();
    }

    /**
     * Resets the sections, reloading the whole new tab page content.
     * @param alwaysAllowEmptySections Whether sections are always allowed to be displayed when
     *     they are empty, even when they are normally not.
     */
    private void resetSections(boolean alwaysAllowEmptySections) {
        removeAllSections();

        SuggestionsSource suggestionsSource = mUiDelegate.getSuggestionsSource();
        int[] categories = suggestionsSource.getCategories();
        for (int category : categories) {
            int categoryStatus = suggestionsSource.getCategoryStatus(category);
            if (SnippetsBridge.isCategoryEnabled(categoryStatus)) {
                resetSection(category, categoryStatus, alwaysAllowEmptySections,
                        shouldReportPrefetchedSuggestionsMetrics(category));
            } else {
                // If articles category is currently disabled, we may still need to show an
                // expandable header for the section.
                maybeAddSectionForHeader(category);
            }
        }

        if (!ChromeFeatureList.isEnabled(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)) {
            maybeHideArticlesHeader();
        }
        recordDisplayedSuggestions(categories);
    }

    /**
     * Resets the section for {@code category}. Removes the section if there are no suggestions for
     * it and it is not allowed to be empty. Otherwise, creates the section if it is not present
     * yet. Sets the available suggestions on the section.
     * @param category The category for which the section must be reset.
     * @param categoryStatus The category status.
     * @param alwaysAllowEmptySections Whether sections are always allowed to be displayed when
     *     they are empty, even when they are normally not.
     * @param reportPrefetchedSuggestionsCount Whether to report number of prefetched article
     *     suggestions.
     */
    private void resetSection(@CategoryInt int category, @CategoryStatus int categoryStatus,
            boolean alwaysAllowEmptySections, boolean reportPrefetchedSuggestionsCount) {
        SuggestionsSource suggestionsSource = mUiDelegate.getSuggestionsSource();
        List<SnippetArticle> suggestions = suggestionsSource.getSuggestionsForCategory(category);
        SuggestionsCategoryInfo info = suggestionsSource.getCategoryInfo(category);

        SuggestionsSection section = mSections.get(category);

        // Do not show an empty section if not allowed.
        if (suggestions.isEmpty() && !info.showIfEmpty() && !alwaysAllowEmptySections) {
            mBlacklistedCategories.add(category);
            if (section != null) removeSection(section);
            return;
        } else {
            mBlacklistedCategories.remove(category);
        }

        // Create the section if needed.
        if (section == null) {
            SuggestionsRanker suggestionsRanker = mUiDelegate.getSuggestionsRanker();
            section = new SuggestionsSection(
                    this, mUiDelegate, suggestionsRanker, mOfflinePageBridge, info);
            mSections.put(category, section);
            suggestionsRanker.registerCategory(category);
            addChild(section);
        } else {
            section.clearData();
        }

        // Set the new suggestions.
        section.setStatus(categoryStatus);
        if (!section.isLoading()) {
            section.appendSuggestions(
                    suggestions, /* keepSectionSize = */ true, reportPrefetchedSuggestionsCount);
        }
    }

    @Override
    public void onNewSuggestions(@CategoryInt int category) {
        @CategoryStatus
        int status = mUiDelegate.getSuggestionsSource().getCategoryStatus(category);
        if (!canProcessSuggestions(category, status)) return;

        SuggestionsSection section = mSections.get(category);
        section.setStatus(status);
        section.updateSuggestions();
    }

    @Override
    public void onCategoryStatusChanged(@CategoryInt int category, @CategoryStatus int status) {
        // Observers should not be registered for this state.
        assert status != CategoryStatus.ALL_SUGGESTIONS_EXPLICITLY_DISABLED;

        // If the category was blacklisted, we note that there might be new content to show.
        mBlacklistedCategories.remove(category);

        // If there is no section for this category there is nothing to do.
        if (!mSections.containsKey(category)) return;

        switch (status) {
            case CategoryStatus.NOT_PROVIDED:
                // The section provider has gone away. Keep open UIs as they are.
                return;

            case CategoryStatus.CATEGORY_EXPLICITLY_DISABLED:
            case CategoryStatus.LOADING_ERROR:
                // Need to remove the entire section from the UI immediately.
                removeSection(mSections.get(category));
                return;

            default:
                mSections.get(category).setStatus(status);
                return;
        }
    }

    @Override
    public void onSuggestionInvalidated(@CategoryInt int category, String idWithinCategory) {
        if (!mSections.containsKey(category)) return;
        mSections.get(category).removeSuggestionById(idWithinCategory);
    }

    @Override
    public void onFullRefreshRequired() {
        refreshSuggestions();
    }

    @Override
    public void onSuggestionsVisibilityChanged(@CategoryInt int category) {
        if (!mSections.containsKey(category)) return;
        mSections.get(category).updateExpandableHeader();
    }

    @Override
    public void dismissSection(SuggestionsSection section) {
        mUiDelegate.getSuggestionsSource().dismissCategory(section.getCategory());
        removeSection(section);
    }

    @Override
    public boolean isResetAllowed() {
        if (!FeatureUtilities.isChromeHomeEnabled()) return false;

        // TODO(dgn): Also check if the bottom sheet is closed and how long since it has been closed
        // or opened, so that we don't refresh content while the user still cares about it.
        // Note: don't only use visibility, as pending FetchMore requests can still come, we don't
        // want to clear all the current suggestions in that case. See https://crbug.com/711414

        return !mUiDelegate.isVisible();
    }

    /**
     * Resets all the sections, getting the current list of categories and the associated
     * suggestions from the backend.
     */
    public void refreshSuggestions() {
        resetSections(/* alwaysAllowEmptySections = */false);
    }

    /**
     * Restores any sections that have been dismissed and triggers a new fetch.
     */
    public void restoreDismissedSections() {
        mUiDelegate.getSuggestionsSource().restoreDismissedCategories();
        resetSections(/* alwaysAllowEmptySections = */ true);
        mUiDelegate.getSuggestionsSource().fetchRemoteSuggestions();
    }

    /**
     * @return Whether the list of sections is empty.
     */
    public boolean isEmpty() {
        return mSections.isEmpty();
    }

    /** Returns whether content has recently been inserted in any of the sections. */
    public boolean hasRecentlyInsertedContent() {
        boolean value = false;
        for (SuggestionsSection section : mSections.values()) {
            // We explicitly go through all the sections to make sure we reset the flag everywhere.
            boolean sectionHasRecentInsertion = section.hasRecentlyInsertedContent();
            value = value || sectionHasRecentInsertion;
        }
        return value;
    }

    /**
     * Fetches more suggestions. The SectionList should contain exactly 1 SuggestionsSection that
     * supports fetching more.
     */
    public void fetchMore() {
        List<SuggestionsSection> supportingSections = new LinkedList<>();

        for (SuggestionsSection section : mSections.values()) {
            if (section.getCategoryInfo().isFetchMoreSupported()) {
                supportingSections.add(section);
            }
        }

        if (supportingSections.size() > 1) {
            assert false : "SectionList.fetchMore - Multiple supporting sections: "
                           + getCategoriesForDebugging();
        } else if (supportingSections.size() == 0) {
            Log.d(TAG, "SectionList.fetchMore - No supporting sections: %s",
                    getCategoriesForDebugging());
        } else if (getChildren().get(getChildren().size() - 1) != supportingSections.get(0)) {
            Log.d(TAG, "SectionList.fetchMore - Supporting section not at end: %s",
                    getCategoriesForDebugging());
        } else if (supportingSections.get(0).isLoading()) {
            Log.d(TAG, "SectionList.fetchMore - Supporting section is already loading.");
        } else {
            // Fetch more is called when the user does not explicitly trigger a fetch (eg, the user
            // scrolls down). In this case we don't inform the user of the outcomes, hence the null
            // parameters.
            supportingSections.get(0).fetchSuggestions(null, null);
        }
    }

    /**
     * Drops all but the first {@code n} thumbnails on articles.
     * @param n The number of article thumbnails to keep.
     */
    public void dropAllButFirstNArticleThumbnails(int n) {
        SuggestionsSection articles = mSections.get(KnownCategories.ARTICLES);
        if (articles == null) return;

        articles.dropAllButFirstNThumbnails(n);
    }

    /** Returns a string showing the categories of all the contained sections. */
    private String getCategoriesForDebugging() {
        StringBuilder sb = new StringBuilder();
        String sep = "";
        for (SuggestionsSection section : mSections.values()) {
            sb.append(sep);
            sb.append(section.getCategory());
            sep = ", ";
        }

        return sb.toString();
    }

    /**
     * Synchronises the data of the sections with that of the suggestions source, resetting the ones
     * that are stale. (see {@link SuggestionsSection#isDataStale()})
     */
    public void synchroniseWithSource() {
        int[] categories = mUiDelegate.getSuggestionsSource().getCategories();

        if (categoriesChanged(categories)) {
            Log.d(TAG, "The categories have changed: old=%s, new=%s - Resetting all the sections.",
                    mSections.keySet(), Arrays.toString(categories));
            // The number or the order of the sections changed. We reset everything.
            resetSections(/* alwaysAllowEmptySections = */ false);
            return;
        }

        for (Map.Entry<Integer, SuggestionsSection> sectionsEntry : mSections.entrySet()) {
            if (!sectionsEntry.getValue().isDataStale()) continue;

            @CategoryInt
            int category = sectionsEntry.getKey();
            Log.d(TAG, "The section for category %d is stale - Resetting.", category);
            resetSection(category, mUiDelegate.getSuggestionsSource().getCategoryStatus(category),
                    /* alwaysAllowEmptySections = */ false,
                    shouldReportPrefetchedSuggestionsMetrics(category));
        }

        // We may have updated (or not) the visible suggestions, so we still record the new state,
        // for UMA parity with the [if categories changed] code path.
        recordDisplayedSuggestions(categories);
    }

    private void removeSection(SuggestionsSection section) {
        mSections.remove(section.getCategory());
        removeChild(section);
    }

    private void removeAllSections() {
        mSections.clear();
        removeChildren();
    }

    /** Hides the header for the {@link KnownCategories#ARTICLES} section when necessary. */
    private void maybeHideArticlesHeader() {
        // If there is more than a section we want to show the headers for disambiguation purposes.
        if (mSections.size() != 1 || mHasExternalSections) return;

        SuggestionsSection articlesSection = mSections.get(KnownCategories.ARTICLES);
        if (articlesSection == null) return;

        articlesSection.setHeaderVisibility(false);
    }

    /**
     * A section that allows zero items should be created for showing the section header if it is
     * not yet created.
     * @param category The category that needs a correspond section shown for the header.
     */
    private void maybeAddSectionForHeader(@CategoryInt int category) {
        if (!ChromeFeatureList.isEnabled(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER))
            return;
        if (category != KnownCategories.ARTICLES) return;

        // Don't add a header if the entire articles section is disabled by policy.
        if (!PrefServiceBridge.getInstance().getBoolean(Pref.NTP_ARTICLES_SECTION_ENABLED)) return;

        SuggestionsSection section = mSections.get(category);
        if (section != null) return;

        int status = mUiDelegate.getSuggestionsSource().getCategoryStatus(category);
        resetSection(category, status, true, shouldReportPrefetchedSuggestionsMetrics(category));
    }

    /**
     * Sets whether there are external sections shown above or below the section list.
     * Only intended for use in a rough contextual suggestions prototype.
     */
    void setHasExternalSections(boolean hasExternalSections) {
        mHasExternalSections = hasExternalSections;
    }

    /**
     * Checks that the list of categories currently displayed by this list is the same as
     * {@code newCategories}: same categories in the same order.
     */
    @VisibleForTesting
    boolean categoriesChanged(@CategoryInt int[] newCategories) {
        Iterator<Integer> shownCategories = mSections.keySet().iterator();
        for (int category : newCategories) {
            if (mBlacklistedCategories.contains(category)) {
                Log.d(TAG, "categoriesChanged: ignoring blacklisted category %d", category);
                continue;
            }
            if (!shownCategories.hasNext()) return true;
            if (shownCategories.next() != category) return true;
        }

        return shownCategories.hasNext();
    }

    /**
     * Returns whether the category is able to process the suggestions. The category might decide
     * not to show incoming suggestions later, but this check ensures it's in a basic state
     * compatible with displaying content.
     */
    private boolean canProcessSuggestions(@CategoryInt int category, @CategoryStatus int status) {
        // If the category was blacklisted, we note that there might be new content to show.
        mBlacklistedCategories.remove(category);

        // We never want to add suggestions from unknown categories.
        if (!mSections.containsKey(category)) return false;

        // The status may have changed while the suggestions were loading, perhaps they should not
        // be displayed any more.
        if (!SnippetsBridge.isCategoryEnabled(status)) {
            Log.w(TAG, "Received suggestions for a disabled category (id=%d, status=%d)", category,
                    status);
            return false;
        }

        return true;
    }

    /**
     * Records the currently visible suggestion state: which categories are visible and how many
     * (prefetched) suggestions per category.
     * @see org.chromium.chrome.browser.suggestions.SuggestionsEventReporter#onPageShown
     */
    private void recordDisplayedSuggestions(int[] categories) {
        int[] suggestionsPerCategory = new int[categories.length];
        boolean[] isCategoryVisible = new boolean[categories.length];

        for (int i = 0; i < categories.length; ++i) {
            SuggestionsSection section = mSections.get(categories[i]);
            suggestionsPerCategory[i] = section != null ? section.getSuggestionsCount() : 0;
            isCategoryVisible[i] = section != null;
        }

        mUiDelegate.getEventReporter().onPageShown(
                categories, suggestionsPerCategory, isCategoryVisible);
    }

    /**
     * Returns the {@link SuggestionsSection} for a given {@code categoryId}, or null if the
     * category doesn't exist.
     * @param categoryId The category ID of the section that should be returned.
     * @return The Section with the given category ID.
     */
    public SuggestionsSection getSection(@CategoryInt int categoryId) {
        return mSections.get(categoryId);
    }
}
