// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.LogoView;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder.PartialBindCallback;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SectionHeaderViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.suggestions.DestructionObserver;
import org.chromium.chrome.browser.suggestions.LogoItem;
import org.chromium.chrome.browser.suggestions.SiteSection;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.suggestions.TileGroup;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;
import java.util.Set;

/**
 * A class that handles merging above the fold elements and below the fold cards into an adapter
 * that will be used to back the NTP RecyclerView. The first element in the adapter should always be
 * the above-the-fold view (containing the logo, search box, and most visited tiles) and subsequent
 * elements will be the cards shown to the user
 */
public class NewTabPageAdapter extends Adapter<NewTabPageViewHolder> implements NodeParent {
    private final SuggestionsUiDelegate mUiDelegate;
    private final ContextMenuManager mContextMenuManager;
    private final OfflinePageBridge mOfflinePageBridge;

    private final @Nullable View mAboveTheFoldView;
    private final @Nullable LogoView mLogoView;
    private final UiConfig mUiConfig;
    private SuggestionsRecyclerView mRecyclerView;

    private final InnerNode mRoot;

    private final @Nullable AboveTheFoldItem mAboveTheFold;
    private final @Nullable LogoItem mLogo;
    private final @Nullable SiteSection mSiteSection;
    private final SectionList mSections;
    private final @Nullable SignInPromo mSigninPromo;
    private final AllDismissedItem mAllDismissed;
    private final Footer mFooter;

    /**
     * Creates the adapter that will manage all the cards to display on the NTP.
     * @param uiDelegate used to interact with the rest of the system.
     * @param aboveTheFoldView the layout encapsulating all the above-the-fold elements
     *         (logo, search box, most visited tiles), or null if only suggestions should
     *         be displayed.
     * @param logoView the view for the logo, which may be provided when {@code aboveTheFoldView} is
     *         null. They are not expected to be both non-null as that would lead to showing the
     *         logo twice.
     * @param uiConfig the NTP UI configuration, to be passed to created views.
     * @param offlinePageBridge used to determine if articles are available.
     * @param contextMenuManager used to build context menus.
     * @param tileGroupDelegate if not null this is used to build a {@link SiteSection}.
     */
    public NewTabPageAdapter(SuggestionsUiDelegate uiDelegate, @Nullable View aboveTheFoldView,
            @Nullable LogoView logoView, UiConfig uiConfig, OfflinePageBridge offlinePageBridge,
            ContextMenuManager contextMenuManager, @Nullable TileGroup.Delegate tileGroupDelegate) {
        assert !(aboveTheFoldView != null && logoView != null);

        mUiDelegate = uiDelegate;
        mContextMenuManager = contextMenuManager;

        mAboveTheFoldView = aboveTheFoldView;
        mLogoView = logoView;
        mUiConfig = uiConfig;
        mRoot = new InnerNode();
        mSections = new SectionList(mUiDelegate, offlinePageBridge);
        mSigninPromo = SignInPromo.maybeCreatePromo(mUiDelegate);
        mAllDismissed = new AllDismissedItem();

        if (mAboveTheFoldView == null) {
            mAboveTheFold = null;
        } else {
            mAboveTheFold = new AboveTheFoldItem();
            mRoot.addChild(mAboveTheFold);
        }

        if (mLogoView == null) {
            mLogo = null;
        } else {
            mLogo = new LogoItem();
            mRoot.addChild(mLogo);
        }

        if (tileGroupDelegate == null) {
            mSiteSection = null;
        } else {
            mSiteSection = new SiteSection(uiDelegate, mContextMenuManager, tileGroupDelegate,
                    offlinePageBridge, uiConfig);
            mRoot.addChild(mSiteSection);
        }

        if (SuggestionsConfig.scrollToLoad()) {
            // If scroll-to-load is enabled, show the sign-in promo above suggested content.
            if (mSigninPromo != null) mRoot.addChild(mSigninPromo);
            mRoot.addChild(mAllDismissed);
            mRoot.addChild(mSections);
        } else {
            mRoot.addChild(mSections);
            if (mSigninPromo != null) mRoot.addChild(mSigninPromo);
            mRoot.addChild(mAllDismissed);
        }

        mFooter = new Footer();
        mRoot.addChild(mFooter);

        mOfflinePageBridge = offlinePageBridge;

        RemoteSuggestionsStatusObserver suggestionsObserver = new RemoteSuggestionsStatusObserver();
        mUiDelegate.addDestructionObserver(suggestionsObserver);

        updateAllDismissedVisibility();
        mRoot.setParent(this);
    }

    @Override
    @ItemViewType
    public int getItemViewType(int position) {
        return mRoot.getItemViewType(position);
    }

    @Override
    public NewTabPageViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        assert parent == mRecyclerView;

        switch (viewType) {
            case ItemViewType.ABOVE_THE_FOLD:
                return new NewTabPageViewHolder(mAboveTheFoldView);

            case ItemViewType.LOGO:
                return new LogoItem.ViewHolder(mLogoView);

            case ItemViewType.SITE_SECTION:
                return SiteSection.createViewHolder(
                        SiteSection.inflateSiteSection(parent), mUiConfig);

            case ItemViewType.HEADER:
                return new SectionHeaderViewHolder(mRecyclerView, mUiConfig);

            case ItemViewType.SNIPPET:
                return new SnippetArticleViewHolder(mRecyclerView, mContextMenuManager, mUiDelegate,
                        mUiConfig, mOfflinePageBridge);

            case ItemViewType.STATUS:
                return new StatusCardViewHolder(mRecyclerView, mContextMenuManager, mUiConfig);

            case ItemViewType.PROGRESS:
                return new ProgressViewHolder(mRecyclerView);

            case ItemViewType.ACTION:
                return new ActionItem.ViewHolder(
                        mRecyclerView, mContextMenuManager, mUiDelegate, mUiConfig);

            case ItemViewType.PROMO:
                return mSigninPromo.createViewHolder(mRecyclerView, mContextMenuManager, mUiConfig);

            case ItemViewType.FOOTER:
                return new Footer.ViewHolder(mRecyclerView, mUiDelegate.getNavigationDelegate());

            case ItemViewType.ALL_DISMISSED:
                return new AllDismissedItem.ViewHolder(mRecyclerView, mSections);
        }

        assert false : viewType;
        return null;
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder, int position, List<Object> payloads) {
        if (payloads.isEmpty()) {
            mRoot.onBindViewHolder(holder, position);
            return;
        }

        for (Object payload : payloads) {
            ((PartialBindCallback) payload).onResult(holder);
        }
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder, final int position) {
        mRoot.onBindViewHolder(holder, position);
    }

    @Override
    public int getItemCount() {
        return mRoot.getItemCount();
    }

    /** Resets suggestions, pulling the current state as known by the backend. */
    public void refreshSuggestions() {
        if (FeatureUtilities.isChromeHomeEnabled()) {
            mSections.synchroniseWithSource();
        } else {
            mSections.refreshSuggestions();
        }

        if (mSiteSection != null) {
            mSiteSection.getTileGroup().onSwitchToForeground(/* trackLoadTask = */ true);
        }
    }

    public int getAboveTheFoldPosition() {
        if (mAboveTheFoldView == null) return RecyclerView.NO_POSITION;

        return getChildPositionOffset(mAboveTheFold);
    }

    public int getFirstHeaderPosition() {
        return getFirstPositionForType(ItemViewType.HEADER);
    }

    public int getFirstSnippetPosition() {
        return getFirstPositionForType(ItemViewType.SNIPPET);
    }

    /**
     * Returns the position in the adapter of the header to the article suggestions if it exists.
     * @return The article header position. RecyclerView.NO_POSITION if articles or their header
     *         does not exist.
     */
    public int getArticleHeaderPosition() {
        SuggestionsSection suggestions = mSections.getSection(KnownCategories.ARTICLES);
        if (suggestions == null || !suggestions.hasCards()) return RecyclerView.NO_POSITION;

        int articlesRank = RecyclerView.NO_POSITION;
        int emptySectionCount = 0;
        int[] categories = mUiDelegate.getSuggestionsSource().getCategories();
        for (int i = 0; i < categories.length; i++) {
            // The categories array includes empty sections.
            if (mSections.getSection(categories[i]) == null) emptySectionCount++;
            if (categories[i] == KnownCategories.ARTICLES) {
                articlesRank = i - emptySectionCount;
                break;
            }
        }
        if (articlesRank == RecyclerView.NO_POSITION) return RecyclerView.NO_POSITION;

        int headerRank = RecyclerView.NO_POSITION;
        for (int i = 0; i < getItemCount(); i++) {
            if (getItemViewType(i) == ItemViewType.HEADER && ++headerRank == articlesRank) return i;
        }
        return RecyclerView.NO_POSITION;
    }

    public int getFirstCardPosition() {
        for (int i = 0; i < getItemCount(); ++i) {
            if (CardViewHolder.isCard(getItemViewType(i))) return i;
        }
        return RecyclerView.NO_POSITION;
    }

    private void updateAllDismissedVisibility() {
        boolean areRemoteSuggestionsEnabled =
                mUiDelegate.getSuggestionsSource().areRemoteSuggestionsEnabled();
        boolean allDismissed = hasAllBeenDismissed() && !areArticlesLoading();
        boolean isArticleSectionVisible =
                ChromeFeatureList.isEnabled(
                        ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)
                && mSections.getSection(KnownCategories.ARTICLES) != null;

        mAllDismissed.setVisible(areRemoteSuggestionsEnabled && allDismissed);
        mFooter.setVisible(!SuggestionsConfig.scrollToLoad() && !allDismissed
                && (areRemoteSuggestionsEnabled || isArticleSectionVisible));
    }

    private boolean areArticlesLoading() {
        for (int category : mUiDelegate.getSuggestionsSource().getCategories()) {
            if (category != KnownCategories.ARTICLES) continue;

            return mUiDelegate.getSuggestionsSource().getCategoryStatus(KnownCategories.ARTICLES)
                    == CategoryStatus.AVAILABLE_LOADING;
        }
        return false;
    }

    @Override
    public void onItemRangeChanged(TreeNode child, int itemPosition, int itemCount,
            @Nullable PartialBindCallback callback) {
        assert child == mRoot;
        notifyItemRangeChanged(itemPosition, itemCount, callback);
    }

    @Override
    public void onItemRangeInserted(TreeNode child, int itemPosition, int itemCount) {
        assert child == mRoot;
        notifyItemRangeInserted(itemPosition, itemCount);
        if (mRecyclerView != null && FeatureUtilities.isChromeHomeEnabled()
                && mSections.hasRecentlyInsertedContent()) {
            mRecyclerView.highlightContentLength();
        }

        updateAllDismissedVisibility();
    }

    @Override
    public void onItemRangeRemoved(TreeNode child, int itemPosition, int itemCount) {
        assert child == mRoot;
        notifyItemRangeRemoved(itemPosition, itemCount);

        updateAllDismissedVisibility();
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);

        if (mRecyclerView == recyclerView) return;

        // We are assuming for now that the adapter is used with a single RecyclerView.
        // Getting the reference as we are doing here is going to be broken if that changes.
        assert mRecyclerView == null;

        mRecyclerView = (SuggestionsRecyclerView) recyclerView;

        if (SuggestionsConfig.scrollToLoad()) {
            mRecyclerView.setScrollToLoadListener(new ScrollToLoadListener(
                    this, mRecyclerView.getLinearLayoutManager(), mSections));
        }
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
        super.onDetachedFromRecyclerView(recyclerView);

        if (SuggestionsConfig.scrollToLoad()) mRecyclerView.clearScrollToLoadListener();

        mRecyclerView = null;
    }

    @Override
    public void onViewRecycled(NewTabPageViewHolder holder) {
        holder.recycle();
    }

    /**
     * @return the set of item positions that should be dismissed simultaneously when dismissing the
     *         item at the given {@code position} (including the position itself), or an empty set
     *         if the item can't be dismissed.
     */
    public Set<Integer> getItemDismissalGroup(int position) {
        return mRoot.getItemDismissalGroup(position);
    }

    /**
     * Dismisses the item at the provided adapter position. Can also cause the dismissal of other
     * items or even entire sections.
     * @param position the position of an item to be dismissed.
     * @param itemRemovedCallback
     */
    public void dismissItem(int position, Callback<String> itemRemovedCallback) {
        mRoot.dismissItem(position, itemRemovedCallback);
    }

    /**
     * Sets the visibility of the logo.
     * @param visible Whether the logo should be visible.
     */
    public void setLogoVisibility(boolean visible) {
        assert mLogo != null;
        mLogo.setVisible(visible);
    }

    /**
     * Drops all but the first {@code n} thumbnails on articles.
     * @param n The number of article thumbnails to keep.
     */
    public void dropAllButFirstNArticleThumbnails(int n) {
        mSections.dropAllButFirstNArticleThumbnails(n);
    }

    private boolean hasAllBeenDismissed() {
        if (mSigninPromo != null && mSigninPromo.isVisible()) return false;

        if (!FeatureUtilities.isChromeHomeEnabled()) return mSections.isEmpty();

        // In Chrome Home we only consider articles.
        SuggestionsSection suggestions = mSections.getSection(KnownCategories.ARTICLES);
        return suggestions == null || !suggestions.hasCards();
    }

    private int getChildPositionOffset(TreeNode child) {
        return mRoot.getStartingOffsetForChild(child);
    }

    @VisibleForTesting
    public int getFirstPositionForType(@ItemViewType int viewType) {
        int count = getItemCount();
        for (int i = 0; i < count; i++) {
            if (getItemViewType(i) == viewType) return i;
        }
        return RecyclerView.NO_POSITION;
    }

    public SectionList getSectionListForTesting() {
        return mSections;
    }

    public InnerNode getRootForTesting() {
        return mRoot;
    }

    private class RemoteSuggestionsStatusObserver
            extends SuggestionsSource.EmptyObserver implements DestructionObserver {
        public RemoteSuggestionsStatusObserver() {
            mUiDelegate.getSuggestionsSource().addObserver(this);
        }

        @Override
        public void onCategoryStatusChanged(
                @CategoryInt int category, @CategoryStatus int newStatus) {
            if (!SnippetsBridge.isCategoryRemote(category)) return;

            updateAllDismissedVisibility();
        }

        @Override
        public void onDestroy() {
            mUiDelegate.getSuggestionsSource().removeObserver(this);
        }
    }
}
