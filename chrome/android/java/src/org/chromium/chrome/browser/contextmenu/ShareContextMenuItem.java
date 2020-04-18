// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.StringRes;
import android.util.Pair;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.widget.TintedDrawable;

/**
 * List of all predefined Context Menu Items for share in Chrome.
 */
public class ShareContextMenuItem implements ContextMenuItem {
    @DrawableRes
    private final int mIconId;
    @StringRes
    private final int mStringId;
    @IdRes
    private final int mMenuId;
    private final boolean mIsShareLink;
    private String mCreatorPackageName;

    /**
     * A representation of a Context Menu Item. Each item should have a string and an id associated
     * with it.
     * @param iconId The icon that appears in {@link TabularContextMenuUi} to represent each item.
     * @param stringId The string that describes the action of the item.
     * @param menuId The id found in ids.xml.
     * @param isShareLink Whether the menu is for sharing a link.
     */
    public ShareContextMenuItem(@DrawableRes int iconId, @StringRes int stringId, @IdRes int menuId,
            boolean isShareLink) {
        mIconId = iconId;
        mStringId = stringId;
        mMenuId = menuId;
        mIsShareLink = isShareLink;
    }

    /**
     * Set the package name of the app who requests for share. If Null, it is requested by Chrome.
     */
    public void setCreatorPackageName(String creatorPackageName) {
        mCreatorPackageName = creatorPackageName;
    }

    @Override
    public String getTitle(Context context) {
        return context.getString(mStringId);
    }

    @Override
    public void getDrawableAsync(Context context, Callback<Drawable> callback) {
        Drawable drawable = TintedDrawable.constructTintedDrawable(
                context.getResources(), mIconId, R.color.dark_mode_tint);
        callback.onResult(drawable);
    }

    @Override
    public int getMenuId() {
        return mMenuId;
    }

    /**
     * Return whether this menu is for sharing a link.
     */
    public boolean isShareLink() {
        return mIsShareLink;
    }

    /**
     * Return the icon and name of the most recently shared app by certain app.
     */
    public Pair<Drawable, CharSequence> getShareInfo() {
        Intent shareIntent = mIsShareLink ? ShareHelper.getShareLinkAppCompatibilityIntent()
                                          : ShareHelper.getShareImageIntent(null);
        return ShareHelper.getShareableIconAndName(shareIntent, mCreatorPackageName);
    }
}
