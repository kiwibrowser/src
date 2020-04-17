// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.SnackbarActivity;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.components.bookmarks.BookmarkId;

import android.app.Activity;
import android.app.Application;
import android.content.Context;

import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.ActivityWindowAndroid;

/**
 * The activity that displays the bookmark UI on the phone. It keeps a {@link BookmarkManager}
 * inside of it and creates a snackbar manager. This activity should only be shown on phones; on
 * tablet the bookmark UI is shown inside of a tab (see {@link BookmarkPage}).
 */
public class BookmarkActivity extends SnackbarActivity {

    private BookmarkManager mBookmarkManager;
    private ActivityWindowAndroid mWindowAndroid;
    static final int EDIT_BOOKMARK_REQUEST_CODE = 14;
    public static final String INTENT_VISIT_BOOKMARK_ID = "BookmarkEditActivity.VisitBookmarkId";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBookmarkManager = new BookmarkManager(this, true, getSnackbarManager());
        String url = getIntent().getDataString();
        if (TextUtils.isEmpty(url)) url = UrlConstants.BOOKMARKS_URL;
        mBookmarkManager.updateForUrl(url);
        setContentView(mBookmarkManager.getView());

        mWindowAndroid = new ActivityWindowAndroid(this, true);
        mWindowAndroid.restoreInstanceState(savedInstanceState);
        mBookmarkManager.setWindow(mWindowAndroid);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        mWindowAndroid.saveInstanceState(outState);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mBookmarkManager.onDestroyed();
    }

    @Override
    public void onBackPressed() {
        if (!mBookmarkManager.onBackPressed()) super.onBackPressed();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mWindowAndroid.onActivityResult(requestCode, resultCode, data);
        if (requestCode == EDIT_BOOKMARK_REQUEST_CODE && resultCode == RESULT_OK) {
            BookmarkId bookmarkId = BookmarkId.getBookmarkIdFromString(data.getStringExtra(
                    INTENT_VISIT_BOOKMARK_ID));
            mBookmarkManager.openBookmark(bookmarkId, BookmarkLaunchLocation.BOOKMARK_EDITOR);
        }
    }

    /**
     * @return The {@link BookmarkManager} for testing purposes.
     */
    @VisibleForTesting
    public BookmarkManager getManagerForTesting() {
        return mBookmarkManager;
    }
}
