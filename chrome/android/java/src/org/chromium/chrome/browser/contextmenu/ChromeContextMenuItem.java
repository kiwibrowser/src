// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.annotation.IdRes;
import android.support.annotation.StringRes;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.DefaultBrowserInfo;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;

/**
 * List of all predefined Context Menu Items available in Chrome.
 */
public enum ChromeContextMenuItem implements ContextMenuItem {
    // Custom Tab Group
    OPEN_IN_NEW_CHROME_TAB(
            R.string.contextmenu_open_in_new_chrome_tab, R.id.contextmenu_open_in_new_chrome_tab),
    OPEN_IN_CHROME_INCOGNITO_TAB(R.string.contextmenu_open_in_chrome_incognito_tab,
            R.id.contextmenu_open_in_chrome_incognito_tab),
    OPEN_IN_BROWSER_ID(0, R.id.contextmenu_open_in_browser_id),

    // Link Group
    OPEN_IN_OTHER_WINDOW(
            R.string.contextmenu_open_in_other_window, R.id.contextmenu_open_in_other_window),
    OPEN_IN_NEW_TAB(R.string.contextmenu_open_in_new_tab, R.id.contextmenu_open_in_new_tab),
    OPEN_IN_INCOGNITO_TAB(
            R.string.contextmenu_open_in_incognito_tab, R.id.contextmenu_open_in_incognito_tab),
    COPY_LINK_ADDRESS(R.string.contextmenu_copy_link_address, R.id.contextmenu_copy_link_address),
    COPY_LINK_TEXT(R.string.contextmenu_copy_link_text, R.id.contextmenu_copy_link_text),
    SAVE_LINK_AS(R.string.contextmenu_save_link, R.id.contextmenu_save_link_as),

    // Image Group
    LOAD_ORIGINAL_IMAGE(
            R.string.contextmenu_load_original_image, R.id.contextmenu_load_original_image),
    SAVE_IMAGE(R.string.contextmenu_save_image, R.id.contextmenu_save_image),
    OPEN_IMAGE(R.string.contextmenu_open_image, R.id.contextmenu_open_image),
    OPEN_IMAGE_IN_NEW_TAB(
            R.string.contextmenu_open_image_in_new_tab, R.id.contextmenu_open_image_in_new_tab),
    SEARCH_BY_IMAGE(R.string.contextmenu_search_web_for_image, R.id.contextmenu_search_by_image),

    // Message Group
    CALL(R.string.contextmenu_call, R.id.contextmenu_call),
    SEND_MESSAGE(R.string.contextmenu_send_message, R.id.contextmenu_send_message),
    ADD_TO_CONTACTS(R.string.contextmenu_add_to_contacts, R.id.contextmenu_add_to_contacts),
    COPY(R.string.contextmenu_copy, R.id.contextmenu_copy),

    // Video Group
    SAVE_VIDEO(R.string.contextmenu_save_video, R.id.contextmenu_save_video),

    // Other
    OPEN_IN_CHROME(R.string.menu_open_in_chrome, R.id.contextmenu_open_in_chrome),

    // Browser Action Items
    BROWSER_ACTIONS_OPEN_IN_BACKGROUND(
            R.string.browser_actions_open_in_background, R.id.browser_actions_open_in_background),
    BROWSER_ACTIONS_OPEN_IN_INCOGNITO_TAB(R.string.browser_actions_open_in_incognito_tab,
            R.id.browser_actions_open_in_incognito_tab),
    BROWSER_ACTION_SAVE_LINK_AS(
            R.string.browser_actions_save_link_as, R.id.browser_actions_save_link_as),
    BROWSER_ACTIONS_COPY_ADDRESS(
            R.string.browser_actions_copy_address, R.id.browser_actions_copy_address);

    @StringRes
    private final int mStringId;
    @IdRes
    private final int mMenuId;

    /**
     * A representation of a Context Menu Item. Each item should have a string and an id associated
     * with it.
     * @param stringId The string that describes the action of the item.
     * @param menuId The id found in ids.xml.
     */
    ChromeContextMenuItem(@StringRes int stringId, @IdRes int menuId) {
        mStringId = stringId;
        mMenuId = menuId;
    }

    /**
     * Transforms the id of the item into a string. It manages special cases that need minor
     * changes due to templating.
     * @param context Requires to get the string resource related to the item.
     * @return Returns a string for the menu item.
     */
    @Override
    public String getTitle(Context context) {
        if (this == ChromeContextMenuItem.SEARCH_BY_IMAGE) {
            return context.getString(R.string.contextmenu_search_web_for_image,
                    TemplateUrlService.getInstance()
                            .getDefaultSearchEngineTemplateUrl()
                            .getShortName());
        } else if (this == OPEN_IN_BROWSER_ID) {
            return DefaultBrowserInfo.getTitleOpenInDefaultBrowser(false);
        } else if (mStringId == 0) {
            return "";
        }

        return context.getString(mStringId);
    }

    @Override
    public void getDrawableAsync(Context context, Callback<Drawable> callback) {
        callback.onResult(null);
    }

    @Override
    public int getMenuId() {
        return mMenuId;
    }
}
