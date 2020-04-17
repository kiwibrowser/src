// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;
import android.support.annotation.IntDef;
import android.support.annotation.LayoutRes;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.ViewGroup;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkModelObserver;
import org.chromium.chrome.browser.bookmarks.BookmarkManager.ItemsAdapter;
import org.chromium.chrome.browser.signin.PersonalizedSigninPromoView;
import org.chromium.chrome.browser.widget.dragreorder.DragReorderableListAdapter;
import org.chromium.components.bookmarks.BookmarkId;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * BaseAdapter for {@link RecyclerView}. It manages bookmarks to list there.
 */
class ReorderBookmarkItemsAdapter extends DragReorderableListAdapter<BookmarkItem>
        implements BookmarkUIObserver, ItemsAdapter {
    /**
     * Specifies the view types that the bookmark delegate screen can contain.
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ViewType.PERSONALIZED_SIGNIN_PROMO, ViewType.SYNC_PROMO, ViewType.FOLDER,
            ViewType.BOOKMARK})
    private @interface ViewType {
        int INVALID_PROMO = -1;
        int PERSONALIZED_SIGNIN_PROMO = 0;
        int SYNC_PROMO = 1;
        int FOLDER = 2;
        int BOOKMARK = 3;
    }

    private static final int MAXIMUM_NUMBER_OF_SEARCH_RESULTS = 500;
    private static final String EMPTY_QUERY = null;

    private final List<BookmarkId> mTopLevelFolders = new ArrayList<>();

    // There can only be one promo header at a time. This takes on one of the values:
    // ViewType.PERSONALIZED_SIGNIN_PROMO, ViewType.SYNC_PROMO, or ViewType.INVALID_PROMO
    private int mPromoHeaderType = ViewType.INVALID_PROMO;
    private BookmarkDelegate mDelegate;
    private BookmarkPromoHeader mPromoHeaderManager;
    private String mSearchText;
    private BookmarkId mCurrentFolder;

    private BookmarkModelObserver mBookmarkModelObserver = new BookmarkModelObserver() {
        @Override
        public void bookmarkNodeChanged(BookmarkItem node) {
            assert mDelegate != null;
            int position = getPositionForBookmark(node.getId());
            if (position >= 0) notifyItemChanged(position);
        }

        @Override
        public void bookmarkNodeRemoved(BookmarkItem parent, int oldIndex, BookmarkItem node,
                boolean isDoingExtensiveChanges) {
            assert mDelegate != null;

            if (mDelegate.getCurrentState() == BookmarkUIState.STATE_SEARCHING
                    && TextUtils.equals(mSearchText, EMPTY_QUERY)) {
                mDelegate.closeSearchUI();
            }

            if (node.isFolder()) {
                mDelegate.notifyStateChange(ReorderBookmarkItemsAdapter.this);
            } else {
                int deletedPosition = getPositionForBookmark(node.getId());
                if (deletedPosition >= 0) {
                    removeItem(deletedPosition);
                }
            }
        }

        @Override
        public void bookmarkModelChanged() {
            assert mDelegate != null;
            mDelegate.notifyStateChange(ReorderBookmarkItemsAdapter.this);

            if (mDelegate.getCurrentState() == BookmarkUIState.STATE_SEARCHING
                    && !TextUtils.equals(mSearchText, EMPTY_QUERY)) {
                search(mSearchText);
            }
        }
    };

    ReorderBookmarkItemsAdapter(Context context) {
        super(context);
    }

    /**
     * @return The position of the given bookmark in adapter. Will return -1 if not found.
     */
    private int getPositionForBookmark(BookmarkId bookmark) {
        assert bookmark != null;
        int position = -1;
        for (int i = 0; i < getItemCount(); i++) {
            if (bookmark.equals(getIdByPosition(i))) {
                position = i;
                break;
            }
        }
        return position;
    }

    private void setBookmarks(List<BookmarkId> bookmarks) {
        // Update header in order to determine whether we have a Promo Header.
        // Must be done before adding in all of our elements.
        updateHeader();
        mElements.clear();
        if (hasPromoHeader()) {
            mElements.add(null);
        }
        for (BookmarkId bId : bookmarks) {
            mElements.add(mDelegate.getModel().getBookmarkById(bId));
        }
        notifyDataSetChanged();
    }

    private void removeItem(int position) {
        mElements.remove(position);
        notifyItemRemoved(position);
    }

    // DragReorderableListAdapter implementation.
    @Override
    public @ViewType int getItemViewType(int position) {
        BookmarkItem item = getItemByPosition(position);
        if (item.isFolder()) {
            return ViewType.FOLDER;
        } else {
            return ViewType.BOOKMARK;
        }
    }

    private ViewHolder createViewHolderHelper(ViewGroup parent, @LayoutRes int layoutId) {
        // create the row associated with this adapter
        ViewGroup row = (ViewGroup) LayoutInflater.from(parent.getContext())
                                .inflate(layoutId, parent, false);

        // ViewHolder is abstract and it cannot be instantiated directly.
        ViewHolder holder = new ViewHolder(row) {};
        ((BookmarkRow) holder.itemView).onDelegateInitialized(mDelegate);
        return holder;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, @ViewType int viewType) {
        assert mDelegate != null;

        switch (viewType) {
            case ViewType.PERSONALIZED_SIGNIN_PROMO:
                return mPromoHeaderManager.createPersonalizedSigninPromoHolder(parent);
            case ViewType.SYNC_PROMO:
                return mPromoHeaderManager.createSyncPromoHolder(parent);
            case ViewType.FOLDER:
                return createViewHolderHelper(parent, R.layout.bookmark_folder_row);
            case ViewType.BOOKMARK:
                return createViewHolderHelper(parent, R.layout.bookmark_item_row);
            default:
                assert false;
                return null;
        }
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        if (holder.getItemViewType() == ViewType.PERSONALIZED_SIGNIN_PROMO) {
            PersonalizedSigninPromoView view = (PersonalizedSigninPromoView) holder.itemView;
            mPromoHeaderManager.setupPersonalizedSigninPromo(view);
        } else if (!(holder.getItemViewType() == ViewType.SYNC_PROMO)) {
            BookmarkRow row = ((BookmarkRow) holder.itemView);
            row.setBookmarkId(getIdByPosition(position));
            row.setDragHandleOnTouchListener((v, event) -> {
                if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                    mItemTouchHelper.startDrag(holder);
                }
                // this callback consumed the click action (don't activate menu)
                return true;
            });
        }
    }

    @Override
    public void onViewRecycled(ViewHolder holder) {
        switch (holder.getItemViewType()) {
            case ViewType.PERSONALIZED_SIGNIN_PROMO:
                mPromoHeaderManager.detachPersonalizePromoView();
                break;
            default:
                // Other view holders don't have special recycling code.
        }
    }

    /**
     * Sets the delegate to use to handle UI actions related to this adapter.
     *
     * @param delegate A {@link BookmarkDelegate} instance to handle all backend interaction.
     */
    @Override
    public void onBookmarkDelegateInitialized(BookmarkDelegate delegate) {
        mDelegate = delegate;
        mDelegate.addUIObserver(this);
        mDelegate.getModel().addObserver(mBookmarkModelObserver);
        // This must be registered directly with the selection delegate.
        // addUIObserver (see above) is not sufficient.
        // TODO(jhimawan): figure out why this is the case.
        mDelegate.getSelectionDelegate().addObserver(ReorderBookmarkItemsAdapter.this);

        Runnable promoHeaderChangeAction = () -> {
            assert mDelegate != null;
            if (mDelegate.getCurrentState() != BookmarkUIState.STATE_FOLDER) {
                return;
            }

            boolean wasShowingPromo = hasPromoHeader();
            updateHeader();
            boolean willShowPromo = hasPromoHeader();

            if (!wasShowingPromo && willShowPromo) {
                notifyItemInserted(0);
            } else if (wasShowingPromo && willShowPromo) {
                notifyItemChanged(0);
            } else if (wasShowingPromo && !willShowPromo) {
                notifyItemRemoved(0);
            }
        };

        mPromoHeaderManager = new BookmarkPromoHeader(mContext, promoHeaderChangeAction);
        populateTopLevelFoldersList();

        mElements = new ArrayList<>();
        setDragStateDelegate(delegate.getDragStateDelegate());
        notifyDataSetChanged();
    }

    // BookmarkUIObserver implementations.
    @Override
    public void onDestroy() {
        mDelegate.removeUIObserver(this);
        mDelegate.getModel().removeObserver(mBookmarkModelObserver);
        mDelegate.getSelectionDelegate().removeObserver(this);
        mDelegate = null;
        mPromoHeaderManager.destroy();
    }

    @Override
    public void onFolderStateSet(BookmarkId folder) {
        assert mDelegate != null;

        mSearchText = EMPTY_QUERY;
        mCurrentFolder = folder;

        enableDrag();

        if (folder.equals(mDelegate.getModel().getRootFolderId())) {
            setBookmarks(mTopLevelFolders);
        } else {
            setBookmarks(mDelegate.getModel().getChildIDs(folder, true, true));
        }
    }

    @Override
    public void onSearchStateSet() {
        disableDrag();
        updateHeader();
        notifyDataSetChanged();
    }

    @Override
    public void onSelectionStateChange(List<BookmarkId> selectedBookmarks) {
        notifyDataSetChanged();
    }

    /**
     * Refresh the list of bookmarks within the currently visible folder.
     */
    @Override
    public void refresh() {
        // TODO(crbug.com/160194): Clean up after bookmark reordering launches.
        // Tell the RecyclerView to update its elements.
        notifyDataSetChanged();
    }

    /**
     * Synchronously searches for the given query.
     *
     * @param query The query text to search for.
     */
    @Override
    public void search(String query) {
        mSearchText = query.toString().trim();
        List<BookmarkId> result =
                mDelegate.getModel().searchBookmarks(mSearchText, MAXIMUM_NUMBER_OF_SEARCH_RESULTS);
        setBookmarks(result);
    }

    private void updateHeader() {
        if (mDelegate == null) return;

        int currentUIState = mDelegate.getCurrentState();
        if (currentUIState == BookmarkUIState.STATE_LOADING) return;

        // Reset the promo header and get rid of the Promo Header placeholder inside of mElements.
        if (hasPromoHeader()) {
            mElements.remove(0);
            mPromoHeaderType = -1;
        }

        if (currentUIState == BookmarkUIState.STATE_SEARCHING) return;

        assert currentUIState == BookmarkUIState.STATE_FOLDER : "Unexpected UI state";

        switch (mPromoHeaderManager.getPromoState()) {
            case BookmarkPromoHeader.PromoState.PROMO_NONE:
                return;
            case BookmarkPromoHeader.PromoState.PROMO_SIGNIN_PERSONALIZED:
                mPromoHeaderType = ViewType.PERSONALIZED_SIGNIN_PROMO;
                return;
            case BookmarkPromoHeader.PromoState.PROMO_SYNC:
                mPromoHeaderType = ViewType.SYNC_PROMO;
                return;
            default:
                assert false : "Unexpected value for promo state!";
        }
    }

    private void populateTopLevelFoldersList() {
        BookmarkId desktopNodeId = mDelegate.getModel().getDesktopFolderId();
        BookmarkId mobileNodeId = mDelegate.getModel().getMobileFolderId();
        BookmarkId othersNodeId = mDelegate.getModel().getOtherFolderId();

        if (mDelegate.getModel().isFolderVisible(mobileNodeId)) {
            mTopLevelFolders.add(mobileNodeId);
        }
        if (mDelegate.getModel().isFolderVisible(desktopNodeId)) {
            mTopLevelFolders.add(desktopNodeId);
        }
        if (mDelegate.getModel().isFolderVisible(othersNodeId)) {
            mTopLevelFolders.add(othersNodeId);
        }

        // Add any top-level managed and partner bookmark folders that are children of the root
        // folder.
        List<BookmarkId> managedAndPartnerFolderIds =
                mDelegate.getModel().getTopLevelFolderIDs(true, false);
        BookmarkId rootFolder = mDelegate.getModel().getRootFolderId();
        for (BookmarkId bookmarkId : managedAndPartnerFolderIds) {
            BookmarkId parent = mDelegate.getModel().getBookmarkById(bookmarkId).getParentId();
            if (parent.equals(rootFolder)) mTopLevelFolders.add(bookmarkId);
        }
    }

    @VisibleForTesting
    public BookmarkDelegate getDelegateForTesting() {
        return mDelegate;
    }

    @Override
    protected void setOrder(List<BookmarkItem> bookmarkItems) {
        assert mCurrentFolder != mTopLevelFolders : "Cannot reorder top-level folders!";
        assert mDelegate.getCurrentState()
                == BookmarkUIState.STATE_FOLDER : "Can only reorder items from folder mode!";

        // Check for a promo header.
        int startIndex = hasPromoHeader() ? 1 : 0;
        // Check for partner bookmarks folder.
        int endIndex = mElements.size() - 1;
        if (!mElements.get(endIndex).isEditable()) {
            endIndex--;
        }
        // Get the new order for the IDs.
        long[] newOrder = new long[endIndex - startIndex + 1];
        for (int i = startIndex; i <= endIndex; i++) {
            newOrder[i - startIndex] = bookmarkItems.get(i).getId().getId();
        }
        mDelegate.getModel().reorderBookmarks(mCurrentFolder, newOrder);
    }

    private boolean isOrderable(BookmarkItem bItem) {
        return bItem != null && bItem.isMovable();
    }

    @Override
    protected boolean isActivelyDraggable(ViewHolder viewHolder) {
        return isPassivelyDraggable(viewHolder)
                && ((BookmarkRow) viewHolder.itemView).isItemSelected();
    }

    @Override
    protected boolean isPassivelyDraggable(ViewHolder viewHolder) {
        BookmarkItem bItem = getItemByHolder(viewHolder);
        return isOrderable(bItem);
    }

    @VisibleForTesting
    BookmarkId getIdByPosition(int position) {
        BookmarkItem bItem = getItemByPosition(position);
        if (bItem == null) return null;
        return bItem.getId();
    }

    private boolean hasPromoHeader() {
        return mPromoHeaderType != ViewType.INVALID_PROMO;
    }
}
