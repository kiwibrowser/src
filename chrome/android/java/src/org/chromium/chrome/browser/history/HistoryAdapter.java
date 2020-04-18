// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.content.res.Resources;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.history.HistoryProvider.BrowsingHistoryObserver;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.widget.DateDividedAdapter;
import org.chromium.chrome.browser.widget.selection.SelectableItemViewHolder;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate.SelectionObserver;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

import java.util.ArrayList;
import java.util.List;

/**
 * Bridges the user's browsing history and the UI used to display it.
 */
public class HistoryAdapter extends DateDividedAdapter implements BrowsingHistoryObserver {
    private static final String EMPTY_QUERY = "";

    private final SelectionDelegate<HistoryItem> mSelectionDelegate;
    private final HistoryProvider mHistoryProvider;
    private final HistoryManager mHistoryManager;
    private final ArrayList<HistoryItemView> mItemViews;
    private RecyclerView mRecyclerView;

    private View mPrivacyDisclaimerBottomSpace;
    private Button mClearBrowsingDataButton;
    private HeaderItem mPrivacyDisclaimerHeaderItem;
    private HeaderItem mClearBrowsingDataButtonHeaderItem;

    private boolean mHasOtherFormsOfBrowsingData;
    private boolean mIsDestroyed;
    private boolean mAreHeadersInitialized;
    private boolean mIsLoadingItems;
    private boolean mIsSearching;
    private boolean mHasMorePotentialItems;
    private boolean mClearOnNextQueryComplete;
    private boolean mPrivacyDisclaimersVisible;
    private boolean mClearBrowsingDataButtonVisible;
    private String mQueryText = EMPTY_QUERY;

    public HistoryAdapter(SelectionDelegate<HistoryItem> delegate, HistoryManager manager,
            HistoryProvider provider) {
        setHasStableIds(true);
        mSelectionDelegate = delegate;
        mHistoryProvider = provider;
        mHistoryProvider.setObserver(this);
        mHistoryManager = manager;
        mItemViews = new ArrayList<>();
    }

    /**
     * Called when the activity/native page is destroyed.
     */
    public void onDestroyed() {
        mHistoryProvider.destroy();
        mIsDestroyed = true;
        mRecyclerView = null;
    }

    /**
     * Initializes the HistoryAdapter and loads the first set of browsing history items.
     */
    public void initialize() {
        mAreHeadersInitialized = false;
        mIsLoadingItems = true;
        mClearOnNextQueryComplete = true;
        mHistoryProvider.queryHistory(mQueryText);
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        // This adapter should only ever be attached to one RecyclerView.
        assert mRecyclerView == null;

        mRecyclerView = recyclerView;
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
        mRecyclerView = null;
    }

    /**
     * Load more browsing history items. Returns early if more items are already being loaded or
     * there are no more items to load.
     */
    public void loadMoreItems() {
        if (!canLoadMoreItems()) return;

        mIsLoadingItems = true;
        addFooter();
        notifyDataSetChanged();
        mHistoryProvider.queryHistoryContinuation();
    }

    /**
     * @return Whether more items can be loaded right now.
     */
    public boolean canLoadMoreItems() {
        return !mIsLoadingItems && mHasMorePotentialItems;
    }

    /**
     * Called to perform a search.
     * @param query The text to search for.
     */
    public void search(String query) {
        mQueryText = query;
        mIsSearching = true;
        mClearOnNextQueryComplete = true;
        mHistoryProvider.queryHistory(mQueryText);
    }

    /**
     * Called when a search is ended.
     */
    public void onEndSearch() {
        mQueryText = EMPTY_QUERY;
        mIsSearching = false;

        // Re-initialize the data in the adapter.
        initialize();
    }

    /**
     * Adds the HistoryItem to the list of items being removed and removes it from the adapter. The
     * removal will not be committed until #removeItems() is called.
     * @param item The item to mark for removal.
     */
    public void markItemForRemoval(HistoryItem item) {
        removeItem(item);
        mHistoryProvider.markItemForRemoval(item);
    }

    /**
     * Removes all items that have been marked for removal through #markItemForRemoval().
     */
    public void removeItems() {
        mHistoryProvider.removeItems();
    }

    /**
     * Should be called when the user's sign in state changes.
     */
    public void onSignInStateChange() {
        for (HistoryItemView itemView : mItemViews) {
            itemView.onSignInStateChange();
        }
        initialize();
        updateClearBrowsingDataButtonVisibility();
    }

    /**
     * See {@link SelectionObserver}.
     */
    public void onSelectionStateChange(boolean selectionEnabled) {
        if (mClearBrowsingDataButton != null) {
            mClearBrowsingDataButton.setEnabled(!selectionEnabled);
        }
        for (HistoryItemView item : mItemViews) {
            item.setRemoveButtonVisible(!selectionEnabled);
        }
    }

    @Override
    protected ViewHolder createViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(parent.getContext()).inflate(
                R.layout.history_item_view, parent, false);
        SelectableItemViewHolder<HistoryItem> viewHolder =
                new SelectableItemViewHolder<>(v, mSelectionDelegate);
        HistoryItemView itemView = (HistoryItemView) viewHolder.itemView;
        itemView.setRemoveButtonVisible(!mSelectionDelegate.isSelectionEnabled());
        mItemViews.add(itemView);
        return viewHolder;
    }

    @Override
    protected void bindViewHolderForTimedItem(ViewHolder current, TimedItem timedItem) {
        final HistoryItem item = (HistoryItem) timedItem;
        @SuppressWarnings("unchecked")
        SelectableItemViewHolder<HistoryItem> holder =
                (SelectableItemViewHolder<HistoryItem>) current;
        holder.displayItem(item);
        ((HistoryItemView) holder.itemView).setHistoryManager(mHistoryManager);
    }

    @Override
    protected int getTimedItemViewResId() {
        return R.layout.date_view;
    }

    @SuppressWarnings("unchecked")
    @Override
    public void onQueryHistoryComplete(List<HistoryItem> items, boolean hasMorePotentialMatches) {
        // Return early if the results are returned after the activity/native page is
        // destroyed to avoid unnecessary work.
        if (mIsDestroyed) return;

        if (mClearOnNextQueryComplete) {
            clear(true);
            mClearOnNextQueryComplete = false;
        }

        if (!mAreHeadersInitialized && items.size() > 0 && !mIsSearching) {
            setHeaders();
            mAreHeadersInitialized = true;
        }

        removeFooter();

        loadItems(items);

        mIsLoadingItems = false;
        mHasMorePotentialItems = hasMorePotentialMatches;
    }

    @Override
    public void onHistoryDeleted() {
        mSelectionDelegate.clearSelection();
        // TODO(twellington): Account for items that have been paged in due to infinite scroll.
        //                    This currently removes all items and re-issues a query.
        initialize();
    }

    @Override
    public void hasOtherFormsOfBrowsingData(boolean hasOtherForms) {
        mHasOtherFormsOfBrowsingData = hasOtherForms;
        setPrivacyDisclaimer();
    }

    @Override
    protected BasicViewHolder createFooter(ViewGroup parent) {
        return new BasicViewHolder(
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.indeterminate_progress_view, parent, false));
    }

    /**
     * Initialize clear browsing data and privacy disclaimer header views and generate header
     * items for them.
     */
    void generateHeaderItems() {
        ViewGroup privacyDisclaimerContainer =
                (ViewGroup) View.inflate(mHistoryManager.getSelectableListLayout().getContext(),
                        R.layout.history_privacy_disclaimer_header, null);

        TextView privacyDisclaimerTextView =
                privacyDisclaimerContainer.findViewById(R.id.privacy_disclaimer);
        privacyDisclaimerTextView.setMovementMethod(LinkMovementMethod.getInstance());
        privacyDisclaimerTextView.setText(getPrivacyDisclaimerText());
        mPrivacyDisclaimerBottomSpace =
                privacyDisclaimerContainer.findViewById(R.id.privacy_disclaimer_bottom_space);

        ViewGroup clearBrowsingDataButtonContainer =
                (ViewGroup) View.inflate(mHistoryManager.getSelectableListLayout().getContext(),
                        R.layout.history_clear_browsing_data_header, null);

        mClearBrowsingDataButton = (Button) clearBrowsingDataButtonContainer.findViewById(
                R.id.clear_browsing_data_button);
        mClearBrowsingDataButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mHistoryManager.openClearBrowsingDataPreference();
            }
        });

        mPrivacyDisclaimerHeaderItem = new HeaderItem(0, privacyDisclaimerContainer);
        mClearBrowsingDataButtonHeaderItem = new HeaderItem(1, clearBrowsingDataButtonContainer);
        updateClearBrowsingDataButtonVisibility();
        setPrivacyDisclaimer();
    }

    /**
     * Pass header items to {@link #setHeaders(HeaderItem...)} as parameters.
     */
    private void setHeaders() {
        ArrayList<HeaderItem> args = new ArrayList<>();
        if (mPrivacyDisclaimersVisible) args.add(mPrivacyDisclaimerHeaderItem);
        if (mClearBrowsingDataButtonVisible) args.add(mClearBrowsingDataButtonHeaderItem);
        setHeaders(args.toArray(new HeaderItem[args.size()]));
    }

    /**
     * Create a {@SpannableString} for privacy disclaimer.
     * @return The {@SpannableString} with the privacy disclaimer string resource and url.
     */
    private SpannableString getPrivacyDisclaimerText() {
        final Resources resources = ContextUtils.getApplicationContext().getResources();
        NoUnderlineClickableSpan link = new NoUnderlineClickableSpan((view) -> {
            mHistoryManager.openUrl(UrlConstants.MY_ACTIVITY_URL_IN_HISTORY, null, true);
        });
        return SpanApplier.applySpans(
                resources.getString(R.string.android_history_other_forms_of_history),
                new SpanApplier.SpanInfo("<link>", "</link>", link));
    }

    /**
     * @return True if any privacy disclaimer should be visible, false otherwise.
     */
    boolean hasPrivacyDisclaimers() {
        return !mHistoryManager.isIncognito() && mHasOtherFormsOfBrowsingData;
    }

    /**
     * Set text of privacy disclaimer and visibility of its container.
     */
    void setPrivacyDisclaimer() {
        boolean shouldShowPrivacyDisclaimers =
                hasPrivacyDisclaimers() && mHistoryManager.shouldShowInfoHeaderIfAvailable();

        // Prevent from refreshing the recycler view if header visibility is not changed.
        if (mPrivacyDisclaimersVisible == shouldShowPrivacyDisclaimers) return;
        mPrivacyDisclaimersVisible = shouldShowPrivacyDisclaimers;
        if (mAreHeadersInitialized) setHeaders();
    }

    private void updateClearBrowsingDataButtonVisibility() {
        // If the history header is not showing (e.g. when there is no browsing history),
        // mClearBrowsingDataButton will be null.
        if (mClearBrowsingDataButton == null) return;
        boolean shouldShowButton =
                PrefServiceBridge.getInstance().getBoolean(Pref.ALLOW_DELETING_BROWSER_HISTORY);
        if (mClearBrowsingDataButtonVisible == shouldShowButton) return;
        mClearBrowsingDataButtonVisible = shouldShowButton;
        mPrivacyDisclaimerBottomSpace.setVisibility(shouldShowButton ? View.GONE : View.VISIBLE);
        if (mAreHeadersInitialized) setHeaders();
    }

    @VisibleForTesting
    ItemGroup getFirstGroupForTests() {
        return getGroupAt(0).first;
    }

    @VisibleForTesting
    void setClearBrowsingDataButtonVisibilityForTest(boolean isVisible) {
        if (mClearBrowsingDataButtonVisible == isVisible) return;
        mClearBrowsingDataButtonVisible = isVisible;

        setHeaders();
    }

    @VisibleForTesting
    public ArrayList<HistoryItemView> getItemViewsForTests() {
        return mItemViews;
    }

    @VisibleForTesting
    void generateHeaderItemsForTest() {
        mPrivacyDisclaimerHeaderItem = new HeaderItem(0, null);
        mClearBrowsingDataButtonHeaderItem = new HeaderItem(1, null);
        mClearBrowsingDataButtonVisible = true;
    }

    @VisibleForTesting
    boolean arePrivacyDisclaimersVisible() {
        return mPrivacyDisclaimersVisible;
    }

    @VisibleForTesting
    boolean isClearBrowsingDataButtonVisible() {
        return mClearBrowsingDataButtonVisible;
    }
}
