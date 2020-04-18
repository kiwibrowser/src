// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.app.Activity;
import android.util.Pair;

import org.chromium.base.Callback;

import java.util.List;

/**
 * A representation of the ContextMenu UI. Given a list of items it should populate and display a
 * context menu.
 */
public interface ContextMenuUi {
    /**
     * Shows the Context Menu in Chrome.
     * @param activity Used to inflate the context menu.
     * @param params The current parameters for the the context menu.
     * @param items The list of items that need to be displayed in the context menu items. This is
     *              taken from the return value of {@link ContextMenuPopulator#buildContextMenu(
     *              ContextMenu, Context, ContextMenuParams)}.
     * @param onItemClicked When the user has pressed an item the menuId associated with the item
     *                      is sent back through {@link Callback#onResult(Object)}. The ids that
     *                      could be called are in ids.xml.
     * @param onMenuShown After the menu is displayed this method should be called to present a
     *                    full menu.
     * @param onMenuClosed When the menu is closed, this method is called to do any possible final
     *                     clean up.
     */
    void displayMenu(Activity activity, ContextMenuParams params,
            List<Pair<Integer, List<ContextMenuItem>>> items, Callback<Integer> onItemClicked,
            Runnable onMenuShown, Runnable onMenuClosed);
}
