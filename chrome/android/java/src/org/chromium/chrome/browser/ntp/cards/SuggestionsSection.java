// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SectionHeader;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.OfflinePageItem;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsOfflineModelObserver;
import org.chromium.chrome.browser.suggestions.SuggestionsRanker;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.util.FeatureUtilities;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * A group of suggestions, with a header, a status card, and a progress indicator. This is
 * responsible for tracking whether its suggestions have been saved offline.
 */
public class SuggestionsSection extends InnerNode {
    private static final String TAG = "NtpCards";

    private final Delegate mDelegate;
    private final SuggestionsCategoryInfo mCategoryInfo;
    private final OfflineModelObserver mOfflineModelObserver;
    private final SuggestionsSource mSuggestionsSource;

    // Children
    private final SectionHeader mHeader;
    private final SuggestionsList mSuggestionsList;
    private final @Nullable StatusItem mStatus;
    private final ActionItem mMoreButton;

    /**
     * Stores whether any suggestions have been appended to the list. In this case the list can
     * generally be longer than what is served by the Source. Thus, the list should never be
     * replaced again.
     */
    private boolean mHasAppended;

    /**
     * Whether the data displayed by this section is not the latest available and should be updated
     * when the user stops interacting with this UI surface.
     */
    private boolean mIsDataStale;

    /** Whether content has been recently inserted. We reset this flag upon reading its value. */
    private boolean mHasInsertedContent;

    /**
     * Delegate interface that allows dismissing this section without introducing
     * a circular dependency.
     */
    public interface Delegate {
        /**
         * Dismisses a section.
         * @param section The section to be dismissed.
         */
        void dismissSection(SuggestionsSection section);

        /** Returns whether the UI surface is in a state that allows the suggestions to be reset. */
        boolean isResetAllowed();
    }

    public SuggestionsSection(Delegate delegate, SuggestionsUiDelegate uiDelegate,
            SuggestionsRanker ranker, OfflinePageBridge offlinePageBridge,
            SuggestionsCategoryInfo info) {
        mDelegate = delegate;
        mCategoryInfo = info;
        mSuggestionsSource = uiDelegate.getSuggestionsSource();

        boolean isExpandable = ChromeFeatureList.isEnabled(
                                       ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)
                && getCategory() == KnownCategories.ARTICLES;
        boolean isExpanded =
                PrefServiceBridge.getInstance().getBoolean(Pref.NTP_ARTICLES_LIST_VISIBLE);
        mHeader = isExpandable ? new SectionHeader(info.getTitle(), isExpanded,
                                         this::updateSuggestionsVisibilityForExpandableHeader)
                               : new SectionHeader(info.getTitle());
        mSuggestionsList = new SuggestionsList(mSuggestionsSource, ranker, info);
        mMoreButton = new ActionItem(this, ranker);

        boolean isChromeHomeEnabled = FeatureUtilities.isChromeHomeEnabled();
        if (isChromeHomeEnabled) {
            mStatus = null;
            addChildren(mHeader, mSuggestionsList, mMoreButton);
        } else {
            mStatus = StatusItem.createNoSuggestionsItem(info);
            addChildren(mHeader, mSuggestionsList, mStatus, mMoreButton);
        }

        mOfflineModelObserver = new OfflineModelObserver(offlinePageBridge);
        uiDelegate.addDestructionObserver(mOfflineModelObserver);

        if (!isChromeHomeEnabled) {
            mStatus.setVisible(shouldShowStatusItem());
        }
    }

    private static class SuggestionsList extends ChildNode implements Iterable<SnippetArticle> {
        private final List<SnippetArticle> mSuggestions = new ArrayList<>();

        private final SuggestionsSource mSuggestionsSource;
        private final SuggestionsRanker mSuggestionsRanker;
        private final SuggestionsCategoryInfo mCategoryInfo;

        public SuggestionsList(SuggestionsSource suggestionsSource, SuggestionsRanker ranker,
                SuggestionsCategoryInfo categoryInfo) {
            mSuggestionsSource = suggestionsSource;
            mSuggestionsRanker = ranker;
            mCategoryInfo = categoryInfo;
        }

        @Override
        protected int getItemCountForDebugging() {
            return mSuggestions.size();
        }

        @Override
        @ItemViewType
        public int getItemViewType(int position) {
            checkIndex(position);
            return ItemViewType.SNIPPET;
        }

        @Override
        public void onBindViewHolder(NewTabPageViewHolder holder, int position) {
            checkIndex(position);
            SnippetArticle suggestion = getSuggestionAt(position);
            mSuggestionsRanker.rankSuggestion(suggestion);
            ((SnippetArticleViewHolder) holder).onBindViewHolder(suggestion, mCategoryInfo);
        }

        public SnippetArticle getSuggestionAt(int position) {
            return mSuggestions.get(position);
        }

        public void clear() {
            int itemCount = mSuggestions.size();
            if (itemCount == 0) return;

            mSuggestions.clear();
            notifyItemRangeRemoved(0, itemCount);
        }

        /**
         * Clears all suggestions except for the first {@code n} suggestions.
         */
        private void clearAllButFirstN(int n) {
            int itemCount = mSuggestions.size();
            if (itemCount > n) {
                mSuggestions.subList(n, itemCount).clear();
                notifyItemRangeRemoved(n, itemCount - n);
            }
        }

        public void addAll(List<SnippetArticle> suggestions) {
            if (suggestions.isEmpty()) return;

            int insertionPointIndex = mSuggestions.size();
            mSuggestions.addAll(suggestions);
            notifyItemRangeInserted(insertionPointIndex, suggestions.size());
        }

        public SnippetArticle remove(int position) {
            SnippetArticle suggestion = mSuggestions.remove(position);
            notifyItemRemoved(position);
            return suggestion;
        }

        @NonNull
        @Override
        public Iterator<SnippetArticle> iterator() {
            return mSuggestions.iterator();
        }

        @Override
        public void visitItems(NodeVisitor visitor) {
            for (SnippetArticle suggestion : mSuggestions) {
                visitor.visitSuggestion(suggestion);
            }
        }

        @Override
        public Set<Integer> getItemDismissalGroup(int position) {
            return Collections.singleton(position);
        }

        @Override
        public void dismissItem(int position, Callback<String> itemRemovedCallback) {
            checkIndex(position);
            if (!isAttached()) {
                // It is possible for this method to be called after the NewTabPage has had
                // destroy() called. This can happen when
                // NewTabPageRecyclerView.dismissWithAnimation() is called and the animation ends
                // after the user has navigated away. In this case we cannot inform the native side
                // that the snippet has been dismissed (http://crbug.com/649299).
                return;
            }

            SnippetArticle suggestion = remove(position);
            mSuggestionsSource.dismissSuggestion(suggestion);
            itemRemovedCallback.onResult(suggestion.mTitle);
        }

        public void updateSuggestionOfflineId(
                SnippetArticle article, Long newId, boolean isPrefetched) {
            int index = mSuggestions.indexOf(article);
            // The suggestions could have been removed / replaced in the meantime.
            if (index == -1) return;

            Long oldId = article.getOfflinePageOfflineId();
            article.setOfflinePageOfflineId(newId);
            article.setIsPrefetched(isPrefetched);

            if ((oldId == null) == (newId == null)) return;
            notifyItemChanged(index, SnippetArticleViewHolder::refreshOfflineBadgeVisibility);
        }
    }

    @Override
    @CallSuper
    public void detach() {
        mOfflineModelObserver.onDestroy();
        super.detach();
    }

    private void onSuggestionsListCountChanged(int oldSuggestionsCount) {
        int newSuggestionsCount = getSuggestionsCount();
        if ((newSuggestionsCount == 0) == (oldSuggestionsCount == 0)) return;

        // We should be able to check here whether Chrome Home is enabled or not, however because
        // of https://crbug.com/778004, we check whether mStatus is null. That crash is caused by
        // the SuggestionsSection being created when Chrome Home was enabled (and so mStatus is
        // null) and then this method being called when Chrome Home is disabled.
        // When Chrome Home is disabled while Chrome is running the Activity is restarted, the
        // SnippetsBridge is destroyed and things should be kept consistent, however the crash
        // reports suggest otherwise.
        // We put an assert in here to cause a crash in debug builds only - so hopefully we can
        // track down what's going wrong.
        assert (mStatus == null) == FeatureUtilities.isChromeHomeEnabled();
        if (mStatus != null) {
            mStatus.setVisible(shouldShowStatusItem());
        }

        // When the ActionItem stops being dismissable, it is possible that it was being
        // interacted with. We need to reset the view's related property changes.
        if (mMoreButton.isVisible()) {
            mMoreButton.notifyItemChanged(0, NewTabPageRecyclerView::resetForDismissCallback);
        }
    }

    @Override
    public void dismissItem(int position, Callback<String> itemRemovedCallback) {
        if (getSectionDismissalRange().contains(position)) {
            mDelegate.dismissSection(this);
            itemRemovedCallback.onResult(getHeaderText());
            return;
        }
        super.dismissItem(position, itemRemovedCallback);
    }

    @Override
    public void onItemRangeRemoved(TreeNode child, int index, int count) {
        super.onItemRangeRemoved(child, index, count);
        if (child == mSuggestionsList) onSuggestionsListCountChanged(getSuggestionsCount() + count);
    }

    @Override
    public void onItemRangeInserted(TreeNode child, int index, int count) {
        super.onItemRangeInserted(child, index, count);
        if (child == mSuggestionsList) {
            mHasInsertedContent = true;
            onSuggestionsListCountChanged(getSuggestionsCount() - count);
        }
    }

    @Override
    protected void notifyItemRangeInserted(int index, int count) {
        super.notifyItemRangeInserted(index, count);
        notifyNeighboursModified(index - 1, index + count);
    }

    @Override
    protected void notifyItemRangeRemoved(int index, int count) {
        super.notifyItemRangeRemoved(index, count);
        notifyNeighboursModified(index - 1, index);
    }

    /** Sends a notification to the items at the provided indices to refresh their background. */
    private void notifyNeighboursModified(int aboveNeighbour, int belowNeighbour) {
        assert aboveNeighbour < belowNeighbour;

        if (aboveNeighbour >= 0) {
            notifyItemChanged(aboveNeighbour, NewTabPageViewHolder::updateLayoutParams);
        }

        if (belowNeighbour < getItemCount()) {
            notifyItemChanged(belowNeighbour, NewTabPageViewHolder::updateLayoutParams);
        }
    }

    /**
     * Removes a suggestion. Does nothing if the ID is unknown.
     * @param idWithinCategory The ID of the suggestion to remove.
     */
    public void removeSuggestionById(String idWithinCategory) {
        int i = 0;
        for (SnippetArticle suggestion : mSuggestionsList) {
            if (suggestion.mIdWithinCategory.equals(idWithinCategory)) {
                mSuggestionsList.remove(i);
                return;
            }
            i++;
        }
    }

    private int getNumberOfSuggestionsExposed() {
        int exposedCount = 0;
        int suggestionsCount = 0;
        for (SnippetArticle suggestion : mSuggestionsList) {
            ++suggestionsCount;
            // We treat all suggestions preceding an exposed suggestion as exposed too.
            if (suggestion.mExposed) exposedCount = suggestionsCount;
        }

        return exposedCount;
    }

    private boolean hasSuggestions() {
        return mSuggestionsList.getItemCount() != 0;
    }

    public int getSuggestionsCount() {
        return mSuggestionsList.getItemCount();
    }

    public boolean isDataStale() {
        return mIsDataStale;
    }

    /** Whether the section is waiting for content to be loaded. */
    public boolean isLoading() {
        return mMoreButton.getState() == ActionItem.State.LOADING;
    }

    /**
     * @return Whether the section is showing content cards. The placeholder is included in this
     * check, as it's standing for content, but the status card is not.
     */
    public boolean hasCards() {
        return hasSuggestions();
    }

    /**
     * Returns whether content has been inserted in the section since last time this method was
     * called.
     */
    public boolean hasRecentlyInsertedContent() {
        boolean value = mHasInsertedContent;
        mHasInsertedContent = false;
        return value;
    }

    private String[] getDisplayedSuggestionIds() {
        String[] suggestionIds = new String[mSuggestionsList.getItemCount()];
        for (int i = 0; i < mSuggestionsList.getItemCount(); ++i) {
            suggestionIds[i] = mSuggestionsList.getSuggestionAt(i).mIdWithinCategory;
        }
        return suggestionIds;
    }

    /**
     * Requests the section to update itself. If possible, it will retrieve suggestions from the
     * backend and use them to replace the current ones. This call may have no or only partial
     * effect if changing the list of suggestions is not allowed (e.g. because the user has already
     * seen the suggestions). In that case, the section will be flagged as stale.
     * (see {@link #isDataStale()})
     * Note, that this method also gets called if the user hits the "More" button on an empty list
     * (either because all suggestions got dismissed or because they were removed due to privacy
     * reasons; e.g. a user clearing their history).
     */
    public void updateSuggestions() {
        if (mDelegate.isResetAllowed()) clearData();

        int numberOfSuggestionsExposed = getNumberOfSuggestionsExposed();
        if (!canUpdateSuggestions(numberOfSuggestionsExposed)) {
            mIsDataStale = true;
            Log.d(TAG, "updateSuggestions: Category %d is stale, it can't replace suggestions.",
                    getCategory());
            return;
        }

        List<SnippetArticle> suggestions =
                mSuggestionsSource.getSuggestionsForCategory(getCategory());
        Log.d(TAG, "Received %d new suggestions for category %d, had %d previously.",
                suggestions.size(), getCategory(), mSuggestionsList.getItemCount());

        // Nothing to append, we can just exit now.
        // TODO(dgn): Distinguish the init case where we have to wait? (https://crbug.com/711457)
        if (suggestions.isEmpty()) return;

        if (numberOfSuggestionsExposed > 0) {
            mIsDataStale = true;
            Log.d(TAG,
                    "updateSuggestions: Category %d is stale, will keep already seen suggestions.",
                    getCategory());
        }
        appendSuggestions(suggestions, /* keepSectionSize = */ true,
                /* reportPrefetchedSuggestionsCount = */ false);
    }

    /**
     * Adds the provided suggestions to the ones currently displayed by the section.
     *
     * @param suggestions The suggestions to be added at the end of the current list.
     * @param keepSectionSize Whether the section size should stay the same -- will be enforced by
     *         replacing not-yet-seen suggestions with the new suggestions.
     * @param reportPrefetchedSuggestionsCount Whether to report the number of prefetched article
     *         suggestions.
     */
    public void appendSuggestions(List<SnippetArticle> suggestions, boolean keepSectionSize,
            boolean reportPrefetchedSuggestionsCount) {
        if (!shouldShowSuggestions()) return;

        int numberOfSuggestionsExposed = getNumberOfSuggestionsExposed();
        if (keepSectionSize) {
            Log.d(TAG, "updateSuggestions: keeping the first %d suggestion",
                    numberOfSuggestionsExposed);
            int numberofSuggestionsToAppend =
                    Math.max(0, suggestions.size() - numberOfSuggestionsExposed);
            mSuggestionsList.clearAllButFirstN(numberOfSuggestionsExposed);
            trimIncomingSuggestions(suggestions,
                    /* targetSize = */ numberofSuggestionsToAppend);
        }
        mSuggestionsList.addAll(suggestions);

        mOfflineModelObserver.updateAllSuggestionsOfflineAvailability(
                reportPrefetchedSuggestionsCount);

        if (!keepSectionSize) {
            NewTabPageUma.recordUIUpdateResult(NewTabPageUma.UI_UPDATE_SUCCESS_APPENDED);
            mHasAppended = true;
        } else {
            NewTabPageUma.recordNumberOfSuggestionsSeenBeforeUIUpdateSuccess(
                    numberOfSuggestionsExposed);
            NewTabPageUma.recordUIUpdateResult(NewTabPageUma.UI_UPDATE_SUCCESS_REPLACED);
        }
    }

    /**
     * De-duplicates the new suggestions with the ones kept in {@link #mSuggestionsList} and removes
     * the excess of incoming items to make sure that the merged list has at most as many items as
     * the incoming list.
     */
    private void trimIncomingSuggestions(List<SnippetArticle> suggestions, int targetSize) {
        for (SnippetArticle suggestion : mSuggestionsList) {
            suggestions.remove(suggestion);
        }

        if (suggestions.size() > targetSize) {
            Log.d(TAG, "trimIncomingSuggestions: removing %d excess elements from the end",
                    suggestions.size() - targetSize);
            suggestions.subList(targetSize, suggestions.size()).clear();
        }
    }

    /**
     * Returns whether the list of suggestions can be updated at the moment.
     */
    private boolean canUpdateSuggestions(int numberOfSuggestionsExposed) {
        if (!shouldShowSuggestions()) return false;
        if (!hasSuggestions()) return true; // If we don't have any, we always accept updates.

        if (CardsVariationParameters.ignoreUpdatesForExistingSuggestions()) {
            Log.d(TAG, "updateSuggestions: replacing existing suggestion disabled");
            NewTabPageUma.recordUIUpdateResult(NewTabPageUma.UI_UPDATE_FAIL_DISABLED);
            return false;
        }

        if (numberOfSuggestionsExposed >= getSuggestionsCount() || mHasAppended) {
            // In case that suggestions got removed, we assume they already were seen. This might
            // be over-simplifying things, but given the rare occurences it should be good enough.
            Log.d(TAG, "updateSuggestions: replacing existing suggestion not possible, all seen");
            NewTabPageUma.recordUIUpdateResult(NewTabPageUma.UI_UPDATE_FAIL_ALL_SEEN);
            return false;
        }

        return true;
    }

    /**
     * Fetches additional suggestions only for this section.
     * @param onFailure A {@link Runnable} that will be run if the fetch fails.
     * @param onNoNewSuggestions A {@link Runnable} that will be run if the fetch succeeds but
     *                           provides no new suggestions.
     */
    public void fetchSuggestions(@Nullable final Runnable onFailure,
            @Nullable Runnable onNoNewSuggestions) {
        assert !isLoading();

        if (getSuggestionsCount() == 0 && getCategoryInfo().isRemote()) {
            // Trigger a full refresh of the section to ensure we persist content locally.
            // If a fetch can be made, the status will be synchronously updated from the backend.
            mSuggestionsSource.fetchRemoteSuggestions();
            return;
        }

        mMoreButton.updateState(ActionItem.State.LOADING);
        mSuggestionsSource.fetchSuggestions(mCategoryInfo.getCategory(),
                getDisplayedSuggestionIds(),
                suggestions -> {  /* successCallback */
                    if (!isAttached()) return; // The section has been dismissed.

                    mMoreButton.updateState(ActionItem.State.BUTTON);

                    appendSuggestions(suggestions, /* keepSectionSize = */ false,
                            /* reportPrefetchedSuggestionsCount = */ false);
                    if (onNoNewSuggestions != null && suggestions.size() == 0) {
                        onNoNewSuggestions.run();
                    }
                },
                () -> {  /* failureRunnable */
                    if (!isAttached()) return; // The section has been dismissed.

                    mMoreButton.updateState(ActionItem.State.BUTTON);
                    if (onFailure != null) onFailure.run();
                });
    }

    /** Sets the status for the section. Some statuses can cause the suggestions to be cleared. */
    public void setStatus(@CategoryStatus int status) {
        if (!SnippetsBridge.isCategoryStatusAvailable(status)) {
            clearData();
            Log.d(TAG, "setStatus: unavailable status, cleared suggestions.");
        }

        boolean isLoading = SnippetsBridge.isCategoryLoading(status);
        mMoreButton.updateState(!shouldShowSuggestions()
                        ? ActionItem.State.HIDDEN
                        : (isLoading ? ActionItem.State.LOADING : ActionItem.State.BUTTON));
    }

    /** Clears the suggestions and related data, resetting the state of the section. */
    public void clearData() {
        mSuggestionsList.clear();
        mHasAppended = false;
        mIsDataStale = false;
    }

    /**
     * Drops all but the first {@code n} thumbnails on suggestions.
     * @param n The number of thumbnails to keep.
     */
    public void dropAllButFirstNThumbnails(int n) {
        for (SnippetArticle suggestion : mSuggestionsList) {
            if (n-- > 0) continue;
            suggestion.clearThumbnail();
        }
    }

    @CategoryInt
    public int getCategory() {
        return mCategoryInfo.getCategory();
    }

    @Override
    public Set<Integer> getItemDismissalGroup(int position) {
        // The section itself can be dismissed via any of the items in the dismissal group,
        // otherwise we fall back to the default implementation, which dispatches to our children.
        Set<Integer> sectionDismissalRange = getSectionDismissalRange();
        if (sectionDismissalRange.contains(position)) return sectionDismissalRange;

        return super.getItemDismissalGroup(position);
    }

    /** Sets the visibility of this section's header. */
    public void setHeaderVisibility(boolean headerVisibility) {
        mHeader.setVisible(headerVisibility);
    }

    /**
     * @return Whether or not the suggestions should be shown in this section.
     */
    private boolean shouldShowSuggestions() {
        return !mHeader.isExpandable() || mHeader.isExpanded();
    }

    /**
     * @return Whether or not the {@link StatusItem} should be shown in this section.
     */
    private boolean shouldShowStatusItem() {
        return shouldShowSuggestions() && !hasSuggestions();
    }

    /**
     * @return The set of indices corresponding to items that can dismiss this entire section
     * (as opposed to individual items in it).
     */
    private Set<Integer> getSectionDismissalRange() {
        if (hasSuggestions() || FeatureUtilities.isChromeHomeEnabled()) {
            return Collections.emptySet();
        }

        int statusCardIndex = getStartingOffsetForChild(mStatus);
        if (!mMoreButton.isVisible()) return Collections.singleton(statusCardIndex);

        assert statusCardIndex + 1 == getStartingOffsetForChild(mMoreButton);
        return new HashSet<>(Arrays.asList(statusCardIndex, statusCardIndex + 1));
    }

    /**
     * Update the expandable header state to match the preference value if necessary. This can
     * happen when the preference is updated by a user click on another new tab page.
     */
    void updateExpandableHeader() {
        if (mHeader.isExpandable()
                && mHeader.isExpanded()
                        != PrefServiceBridge.getInstance().getBoolean(
                                   Pref.NTP_ARTICLES_LIST_VISIBLE)) {
            mHeader.toggleHeader();
        }
    }

    /**
     * Update the visibility of the suggestions based on whether the header is expanded. This is
     * called when the section header is toggled.
     */
    private void updateSuggestionsVisibilityForExpandableHeader() {
        assert mHeader.isExpandable();
        PrefServiceBridge.getInstance().setBoolean(
                Pref.NTP_ARTICLES_LIST_VISIBLE, mHeader.isExpanded());
        clearData();
        if (mHeader.isExpanded()) updateSuggestions();
        setStatus(mSuggestionsSource.getCategoryStatus(getCategory()));
        mStatus.setVisible(shouldShowStatusItem());
    }

    public SuggestionsCategoryInfo getCategoryInfo() {
        return mCategoryInfo;
    }

    public String getHeaderText() {
        return mHeader.getHeaderText();
    }

    ActionItem getActionItemForTesting() {
        return mMoreButton;
    }

    public SectionHeader getHeaderItemForTesting() {
        return mHeader;
    }

    private class OfflineModelObserver extends SuggestionsOfflineModelObserver<SnippetArticle> {
        public OfflineModelObserver(OfflinePageBridge bridge) {
            super(bridge);
        }

        @Override
        public void onSuggestionOfflineIdChanged(SnippetArticle suggestion, OfflinePageItem item) {
            boolean isPrefetched = item != null
                    && TextUtils.equals(item.getClientId().getNamespace(),
                               OfflinePageBridge.SUGGESTED_ARTICLES_NAMESPACE);
            mSuggestionsList.updateSuggestionOfflineId(
                    suggestion, item == null ? null : item.getOfflineId(), isPrefetched);
        }

        @Override
        public Iterable<SnippetArticle> getOfflinableSuggestions() {
            return mSuggestionsList;
        }
    }
}
