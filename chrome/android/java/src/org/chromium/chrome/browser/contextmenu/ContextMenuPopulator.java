// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.util.Pair;
import android.view.ContextMenu;

import java.util.List;

/**
 * A delegate responsible for populating context menus and processing results from
 * {@link ContextMenuHelper}.
 */
public interface ContextMenuPopulator {
    /**
     *  Called when this ContextMenuPopulator is about to be destroyed.
     */
    public void onDestroy();

    /**
     * Should be used to populate {@code menu} with the correct context menu items.
     * @param menu    The menu to populate.
     * @param context A {@link Context} instance.
     * @param params  The parameters that represent what should be shown in the context menu.
     * @return A list separate by groups. Each "group" will contain items related to said group as
     *         well as an integer that is a string resource for the group. Image items will have
     *         items that belong to that are related to that group and the string resource for the
     *         group will likely say "IMAGE". If the link pressed is contains multiple items (like
     *         an image link) the list will have both an image list and a link list.
     */
    public List<Pair<Integer, List<ContextMenuItem>>> buildContextMenu(
            ContextMenu menu, Context context, ContextMenuParams params);

    /**
     * Called when a context menu item has been selected.
     * @param helper The {@link ContextMenuHelper} driving the menu operations.
     * @param params The parameters that represent what is being shown in the context menu.
     * @param itemId The id of the selected menu item.
     * @return       Whether or not the selection was handled.
     */
    public boolean onItemSelected(ContextMenuHelper helper, ContextMenuParams params, int itemId);
}