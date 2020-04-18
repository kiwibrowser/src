// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.favicon.IconType;
import org.chromium.chrome.browser.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.RoundedIconGenerator;
import org.chromium.components.bookmarks.BookmarkId;

/**
 * A row view that shows bookmark info in the bookmarks UI.
 */
public class BookmarkItemRow extends BookmarkRow implements LargeIconCallback {

    private String mUrl;
    private RoundedIconGenerator mIconGenerator;
    private final int mMinIconSize;
    private final int mDisplayedIconSize;

    /**
     * Constructor for inflating from XML.
     */
    public BookmarkItemRow(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMinIconSize = (int) getResources().getDimension(R.dimen.default_favicon_min_size);
        mDisplayedIconSize = getResources().getDimensionPixelSize(R.dimen.default_favicon_size);
        mIconGenerator = ViewUtils.createDefaultRoundedIconGenerator(
                FeatureUtilities.isChromeModernDesignEnabled());
    }

    // BookmarkRow implementation.

    @Override
    public void onClick() {
        int launchLocation = -1;
        switch (mDelegate.getCurrentState()) {
            case BookmarkUIState.STATE_FOLDER:
                launchLocation = BookmarkLaunchLocation.FOLDER;
                break;
            case BookmarkUIState.STATE_SEARCHING:
                launchLocation = BookmarkLaunchLocation.SEARCH;
                break;
            case BookmarkUIState.STATE_LOADING:
                assert false :
                        "The main content shouldn't be inflated if it's still loading";
                break;
            default:
                assert false : "State not valid";
                break;
        }
        mDelegate.openBookmark(mBookmarkId, launchLocation);
    }

    @Override
    BookmarkItem setBookmarkId(BookmarkId bookmarkId) {
        BookmarkItem item = super.setBookmarkId(bookmarkId);
        mUrl = item.getUrl();
        mIconView.setImageDrawable(null);
        mTitleView.setText(item.getTitle());
        mDescriptionView.setText(item.getUrlForDisplay());
        mDelegate.getLargeIconBridge().getLargeIconForUrl(mUrl, mMinIconSize, this);
        return item;
    }

    // LargeIconCallback implementation.

    @Override
    public void onLargeIconAvailable(Bitmap icon, int fallbackColor, boolean isFallbackColorDefault,
            @IconType int iconType) {
        if (icon == null) {
            mIconGenerator.setBackgroundColor(fallbackColor);
            icon = mIconGenerator.generateIconForUrl(mUrl);
            setIconDrawable(new BitmapDrawable(getResources(), icon));
        } else {
            setIconDrawable(ViewUtils.createRoundedBitmapDrawable(
                    Bitmap.createScaledBitmap(icon, mDisplayedIconSize, mDisplayedIconSize, false),
                    ViewUtils.DEFAULT_FAVICON_CORNER_RADIUS));
        }
    }
}
