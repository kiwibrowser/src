// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.support.annotation.IntDef;
import android.support.annotation.StringRes;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsNavigationDelegate;
import org.chromium.ui.base.WindowAndroid.OnCloseContextMenuListener;
import org.chromium.ui.mojom.WindowOpenDisposition;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Map;
import java.util.TreeMap;

/**
 * Takes care of creating, closing a context menu and triaging the item clicks.
 */
public class ContextMenuManager implements OnCloseContextMenuListener {
    @IntDef({ID_OPEN_IN_NEW_WINDOW, ID_OPEN_IN_NEW_TAB, ID_OPEN_IN_INCOGNITO_TAB,
            ID_SAVE_FOR_OFFLINE, ID_REMOVE, ID_LEARN_MORE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ContextMenuItemId {}

    // The order of the items will be based on the value of their ID. So if new items are added,
    // the value of the existing ones should be modified so they stay in order.
    public static final int ID_OPEN_IN_NEW_WINDOW = 0;
    public static final int ID_OPEN_IN_NEW_TAB = 1;
    public static final int ID_OPEN_IN_INCOGNITO_TAB = 2;
    public static final int ID_SAVE_FOR_OFFLINE = 3;
    public static final int ID_REMOVE = 4;
    public static final int ID_LEARN_MORE = 5;

    private final SuggestionsNavigationDelegate mNavigationDelegate;
    private final TouchEnabledDelegate mTouchEnabledDelegate;
    private final Runnable mCloseContextMenuCallback;
    private boolean mContextMenuOpen;

    /** Defines callback to configure the context menu and respond to user interaction. */
    public interface Delegate {
        /** Opens the current item the way specified by {@code windowDisposition}. */
        void openItem(int windowDisposition);

        /** Remove the current item. */
        void removeItem();

        /**
         * @return the URL of the current item for saving offline, or null if the item can't be
         *         saved offline.
         */
        String getUrl();

        /** @return whether the given menu item is supported. */
        boolean isItemSupported(@ContextMenuItemId int menuItemId);

        /** Called when a context menu has been created. */
        void onContextMenuCreated();
    }

    /**
     * Delegate used by the {@link ContextMenuManager} to disable touch events on the outer view
     * while the context menu is open.
     */
    public interface TouchEnabledDelegate { void setTouchEnabled(boolean enabled); }

    public ContextMenuManager(SuggestionsNavigationDelegate navigationDelegate,
            TouchEnabledDelegate touchEnabledDelegate, Runnable closeContextMenuCallback) {
        mNavigationDelegate = navigationDelegate;
        mTouchEnabledDelegate = touchEnabledDelegate;
        mCloseContextMenuCallback = closeContextMenuCallback;
    }

    /**
     * Populates the context menu.
     *
     * @param menu The menu to populate.
     * @param associatedView The view that requested a context menu.
     * @param delegate Delegate that defines the configuration of the menu and what to do when items
     *            are tapped.
     */
    public void createContextMenu(ContextMenu menu, View associatedView, Delegate delegate) {
        OnMenuItemClickListener listener = new ItemClickListener(delegate, mNavigationDelegate);
        boolean hasItems = false;

        for (@ContextMenuItemId int itemId : MenuItemLabelMatcher.STRING_MAP.keySet()) {
            if (!shouldShowItem(itemId, delegate)) continue;

            @StringRes
            int itemString = MenuItemLabelMatcher.STRING_MAP.get(itemId);
            menu.add(Menu.NONE, itemId, Menu.NONE, itemString).setOnMenuItemClickListener(listener);
            hasItems = true;
        }

        // No item added. We won't show the menu, so we can skip the rest.
        if (!hasItems) return;

        delegate.onContextMenuCreated();

        associatedView.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override
            public void onViewAttachedToWindow(View view) {}

            @Override
            public void onViewDetachedFromWindow(View view) {
                closeContextMenu();
                view.removeOnAttachStateChangeListener(this);
            }
        });

        // Touch events must be disabled on the outer view while the context menu is open. This is
        // to prevent the user long pressing to get the context menu then on the same press
        // scrolling or swiping to dismiss an item (eg. https://crbug.com/638854,
        // https://crbug.com/638555, https://crbug.com/636296).
        mTouchEnabledDelegate.setTouchEnabled(false);
        mContextMenuOpen = true;

        RecordUserAction.record("Suggestions.ContextMenu.Shown");
    }

    @Override
    public void onContextMenuClosed() {
        if (!mContextMenuOpen) return;
        mTouchEnabledDelegate.setTouchEnabled(true);
        mContextMenuOpen = false;
    }

    /** Closes the context menu, if open. */
    public void closeContextMenu() {
        mCloseContextMenuCallback.run();
    }

    private boolean shouldShowItem(@ContextMenuItemId int itemId, Delegate delegate) {
        if (!delegate.isItemSupported(itemId)) return false;

        switch (itemId) {
            case ID_OPEN_IN_NEW_WINDOW:
                return mNavigationDelegate.isOpenInNewWindowEnabled();
            case ID_OPEN_IN_NEW_TAB:
                return true;
            case ID_OPEN_IN_INCOGNITO_TAB:
                return mNavigationDelegate.isOpenInIncognitoEnabled();
            case ID_SAVE_FOR_OFFLINE: {
                String itemUrl = delegate.getUrl();
                return itemUrl != null && OfflinePageBridge.canSavePage(itemUrl);
            }
            case ID_REMOVE:
                return true;
            case ID_LEARN_MORE:
                // With Scroll to Load enabled, the Learn More link is moved into the Context Menu.
                return SuggestionsConfig.scrollToLoad();

            default:
                assert false;
                return false;
        }
    }

    private static class MenuItemLabelMatcher {
        private static final Map<Integer, Integer> STRING_MAP = new TreeMap<Integer, Integer>() {
            {
                put(ID_OPEN_IN_NEW_WINDOW, R.string.contextmenu_open_in_other_window);
                put(ID_OPEN_IN_NEW_TAB, R.string.contextmenu_open_in_new_tab);
                put(ID_OPEN_IN_INCOGNITO_TAB, R.string.contextmenu_open_in_incognito_tab);
                put(ID_SAVE_FOR_OFFLINE, R.string.contextmenu_save_link);
                put(ID_REMOVE, R.string.remove);
                put(ID_LEARN_MORE, R.string.learn_more);
            }
        };
    }

    private static class ItemClickListener implements OnMenuItemClickListener {
        private final Delegate mDelegate;
        private final SuggestionsNavigationDelegate mNavigationDelegate;

        ItemClickListener(Delegate delegate, SuggestionsNavigationDelegate navigationDelegate) {
            mDelegate = delegate;
            mNavigationDelegate = navigationDelegate;
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            switch (item.getItemId()) {
                case ID_OPEN_IN_NEW_WINDOW:
                    mDelegate.openItem(WindowOpenDisposition.NEW_WINDOW);
                    RecordUserAction.record("Suggestions.ContextMenu.OpenItemInNewWindow");
                    return true;
                case ID_OPEN_IN_NEW_TAB:
                    mDelegate.openItem(WindowOpenDisposition.NEW_BACKGROUND_TAB);
                    RecordUserAction.record("Suggestions.ContextMenu.OpenItemInNewTab");
                    return true;
                case ID_OPEN_IN_INCOGNITO_TAB:
                    mDelegate.openItem(WindowOpenDisposition.OFF_THE_RECORD);
                    RecordUserAction.record("Suggestions.ContextMenu.OpenItemInIncognitoTab");
                    return true;
                case ID_SAVE_FOR_OFFLINE:
                    mDelegate.openItem(WindowOpenDisposition.SAVE_TO_DISK);
                    RecordUserAction.record("Suggestions.ContextMenu.DownloadItem");
                    return true;
                case ID_REMOVE:
                    mDelegate.removeItem();
                    RecordUserAction.record("Suggestions.ContextMenu.RemoveItem");
                    return true;
                case ID_LEARN_MORE:
                    mNavigationDelegate.navigateToHelpPage();
                    RecordUserAction.record("Suggestions.ContextMenu.LearnMore");
                    return true;
                default:
                    return false;
            }
        }
    }
}
