// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.annotation.IdRes;

import org.chromium.base.Callback;

/**
 * An interface to get information of context menu.
 */
public interface ContextMenuItem {
    /**
     * Gets the {@link IdRes} menuId of a context menu.
     */
    @IdRes
    int getMenuId();

    /**
     * Gets the title of a context menu item.
     * @param context The context required to get the title from resources.
     * @return The title of the menu item.
     */
    String getTitle(Context context);

    /**
     * Gets the {@link Drawable} icon of a context menu item asynchronously.
     * @param context The context required to get the icon from resources.
     * @param callback The {@link Callback} used to show the icon when it is ready.
     */
    void getDrawableAsync(Context context, Callback<Drawable> callback);
}
