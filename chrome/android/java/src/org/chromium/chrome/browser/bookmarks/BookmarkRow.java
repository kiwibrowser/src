// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.widget.ListMenuButton;
import org.chromium.chrome.browser.widget.ListMenuButton.Item;
import org.chromium.chrome.browser.widget.selection.SelectableItemView;
import org.chromium.components.bookmarks.BookmarkId;

import java.util.List;

/**
 * Common logic for bookmark and folder rows.
 */
abstract class BookmarkRow extends SelectableItemView<BookmarkId>
        implements BookmarkUIObserver, ListMenuButton.Delegate {
    protected ListMenuButton mMoreIcon;

    protected BookmarkDelegate mDelegate;
    protected BookmarkId mBookmarkId;
    private boolean mIsAttachedToWindow;

    /**
     * Constructor for inflating from XML.
     */
    public BookmarkRow(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Updates this row for the given {@link BookmarkId}.
     * @return The {@link BookmarkItem} corresponding the given {@link BookmarkId}.
     */
    BookmarkItem setBookmarkId(BookmarkId bookmarkId) {
        mBookmarkId = bookmarkId;
        BookmarkItem bookmarkItem = mDelegate.getModel().getBookmarkById(bookmarkId);
        mMoreIcon.dismiss();

        mMoreIcon.setContentDescriptionContext(bookmarkItem.getTitle());
        mMoreIcon.setVisibility(bookmarkItem.isEditable() ? VISIBLE : GONE);
        setChecked(mDelegate.getSelectionDelegate().isItemSelected(bookmarkId));

        super.setItem(bookmarkId);
        return bookmarkItem;
    }

    /**
     * Sets the delegate to use to handle UI actions related to this view.
     * @param delegate A {@link BookmarkDelegate} instance to handle all backend interaction.
     */
    public void onBookmarkDelegateInitialized(BookmarkDelegate delegate) {
        super.setSelectionDelegate(delegate.getSelectionDelegate());
        mDelegate = delegate;
        if (mIsAttachedToWindow) initialize();
    }

    private void initialize() {
        mDelegate.addUIObserver(this);
        updateSelectionState();
    }

    private void cleanup() {
        mMoreIcon.dismiss();
        if (mDelegate != null) mDelegate.removeUIObserver(this);
    }

    private void updateSelectionState() {
        mMoreIcon.setClickable(!mDelegate.getSelectionDelegate().isSelectionEnabled());
    }

    // PopupMenuItem.Delegate implementation.
    @Override
    public Item[] getItems() {
        boolean canMove = false;
        if (mDelegate != null && mDelegate.getModel() != null) {
            BookmarkItem bookmarkItem = mDelegate.getModel().getBookmarkById(mBookmarkId);
            if (bookmarkItem != null) canMove = bookmarkItem.isMovable();
        }
        return new Item[] {new Item(getContext(), R.string.bookmark_item_select, true),
                new Item(getContext(), R.string.bookmark_item_edit, true),
                new Item(getContext(), R.string.bookmark_item_move, canMove),
                new Item(getContext(), R.string.bookmark_item_delete, true)};
    }

    @Override
    public void onItemSelected(Item item) {
        if (item.getTextId() == R.string.bookmark_item_select) {
            setChecked(mDelegate.getSelectionDelegate().toggleSelectionForItem(mBookmarkId));
            RecordUserAction.record("Android.BookmarkPage.SelectFromMenu");
        } else if (item.getTextId() == R.string.bookmark_item_edit) {
            BookmarkItem bookmarkItem = mDelegate.getModel().getBookmarkById(mBookmarkId);
            if (bookmarkItem.isFolder()) {
                BookmarkAddEditFolderActivity.startEditFolderActivity(
                        getContext(), bookmarkItem.getId());
            } else {
                BookmarkUtils.startEditActivity(getContext(), bookmarkItem.getId());
            }
        } else if (item.getTextId() == R.string.bookmark_item_move) {
            BookmarkFolderSelectActivity.startFolderSelectActivity(getContext(), mBookmarkId);
        } else if (item.getTextId() == R.string.bookmark_item_delete) {
            if (mDelegate != null && mDelegate.getModel() != null) {
                mDelegate.getModel().deleteBookmarks(mBookmarkId);
                RecordUserAction.record("Android.BookmarkPage.RemoveItem");
            }
        }
    }

    // FrameLayout implementation.
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mMoreIcon = (ListMenuButton) findViewById(R.id.more);
        mMoreIcon.setDelegate(this);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mIsAttachedToWindow = true;
        if (mDelegate != null) {
            initialize();
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mIsAttachedToWindow = false;
        cleanup();
    }

    // SelectableItem implementation.
    @Override
    public void onSelectionStateChange(List<BookmarkId> selectedBookmarks) {
        super.onSelectionStateChange(selectedBookmarks);
        updateSelectionState();
    }

    // BookmarkUIObserver implementation.
    @Override
    public void onDestroy() {
        cleanup();
    }

    @Override
    public void onFolderStateSet(BookmarkId folder) {}

    @Override
    public void onSearchStateSet() {}
}
