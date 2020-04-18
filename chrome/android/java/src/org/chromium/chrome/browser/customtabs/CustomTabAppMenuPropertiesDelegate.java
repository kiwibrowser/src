// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_MEDIA_VIEWER;
import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_MINIMAL_UI_WEBAPP;
import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_OFFLINE_PAGE;
import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_PAYMENT_REQUEST;
import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_READER_MODE;

import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuItem;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.DefaultBrowserInfo;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.appmenu.AppMenuPropertiesDelegate;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CustomTabsUiType;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.tab.Tab;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * App menu properties delegate for {@link CustomTabActivity}.
 */
public class CustomTabAppMenuPropertiesDelegate extends AppMenuPropertiesDelegate {
    private final @CustomTabsUiType int mUiType;
    private final boolean mShowShare;
    private final boolean mShowStar;
    private final boolean mShowDownload;
    private final boolean mIsOpenedByChrome;

    private final List<String> mMenuEntries;
    private final Map<MenuItem, Integer> mItemToIndexMap = new HashMap<MenuItem, Integer>();

    private boolean mIsCustomEntryAdded;

    /**
     * Creates an {@link CustomTabAppMenuPropertiesDelegate} instance.
     */
    public CustomTabAppMenuPropertiesDelegate(final ChromeActivity activity,
            @CustomTabsUiType final int uiType, List<String> menuEntries, boolean isOpenedByChrome,
            boolean showShare, boolean showStar, boolean showDownload) {
        super(activity);
        mUiType = uiType;
        mMenuEntries = menuEntries;
        mIsOpenedByChrome = isOpenedByChrome;
        mShowShare = showShare;
        mShowStar = showStar;
        mShowDownload = showDownload;
    }

    @Override
    public void prepareMenu(Menu menu) {
        Tab currentTab = mActivity.getActivityTab();
        if (currentTab != null) {
            MenuItem forwardMenuItem = menu.findItem(R.id.forward_menu_id);
            forwardMenuItem.setEnabled(currentTab.canGoForward());

            mReloadMenuItem = menu.findItem(R.id.reload_menu_id);
            mReloadMenuItem.setIcon(R.drawable.btn_reload_stop);
            loadingStateChanged(currentTab.isLoading());

            MenuItem shareItem = menu.findItem(R.id.share_row_menu_id);
            shareItem.setVisible(mShowShare);
            shareItem.setEnabled(mShowShare);
            if (mShowShare) {
                ShareHelper.configureDirectShareMenuItem(
                        mActivity, menu.findItem(R.id.direct_share_menu_id));
            }

            boolean openInChromeItemVisible = true;
            boolean bookmarkItemVisible = mShowStar;
            boolean downloadItemVisible = mShowDownload;
            boolean addToHomeScreenVisible = true;
            boolean requestDesktopSiteVisible = true;

            if (mUiType == CUSTOM_TABS_UI_TYPE_MEDIA_VIEWER) {
                // Most of the menu items don't make sense when viewing media.
                menu.findItem(R.id.icon_row_menu_id).setVisible(false);
                menu.findItem(R.id.find_in_page_id).setVisible(false);
                bookmarkItemVisible = false; // Set to skip initialization.
                downloadItemVisible = false; // Set to skip initialization.
                openInChromeItemVisible = false;
                requestDesktopSiteVisible = false;
                addToHomeScreenVisible = false;
            } else if (mUiType == CUSTOM_TABS_UI_TYPE_PAYMENT_REQUEST) {
                // Only the icon row and 'find in page' are shown for opening payment request UI
                // from Chrome.
                openInChromeItemVisible = false;
                requestDesktopSiteVisible = false;
                addToHomeScreenVisible = false;
                downloadItemVisible = false;
                bookmarkItemVisible = false;
            } else if (mUiType == CUSTOM_TABS_UI_TYPE_READER_MODE) {
                // Only 'find in page' and the reader mode preference are shown for Reader Mode UI.
                menu.findItem(R.id.icon_row_menu_id).setVisible(false);
                bookmarkItemVisible = false; // Set to skip initialization.
                downloadItemVisible = false; // Set to skip initialization.
                openInChromeItemVisible = false;
                requestDesktopSiteVisible = false;
                addToHomeScreenVisible = false;

                menu.findItem(R.id.reader_mode_prefs_id).setVisible(true);
            } else if (mUiType == CUSTOM_TABS_UI_TYPE_MINIMAL_UI_WEBAPP) {
                requestDesktopSiteVisible = false;
                addToHomeScreenVisible = false;
                downloadItemVisible = false;
                bookmarkItemVisible = false;
            } else if (mUiType == CUSTOM_TABS_UI_TYPE_OFFLINE_PAGE) {
                openInChromeItemVisible = false;
                bookmarkItemVisible = true;
                downloadItemVisible = false;
                addToHomeScreenVisible = false;
                requestDesktopSiteVisible = true;
            }

            if (!FirstRunStatus.getFirstRunFlowComplete()) {
                openInChromeItemVisible = false;
                bookmarkItemVisible = false;
                downloadItemVisible = false;
                addToHomeScreenVisible = false;
            }

            String url = currentTab.getUrl();
            boolean isChromeScheme = url.startsWith(UrlConstants.CHROME_URL_PREFIX)
                    || url.startsWith(UrlConstants.CHROME_NATIVE_URL_PREFIX);
            if (isChromeScheme || TextUtils.isEmpty(url)) {
                addToHomeScreenVisible = false;
            }

            MenuItem downloadItem = menu.findItem(R.id.offline_page_id);
            if (downloadItemVisible) {
                downloadItem.setEnabled(DownloadUtils.isAllowedToDownloadPage(currentTab));
            } else {
                downloadItem.setVisible(false);
            }

            MenuItem bookmarkItem = menu.findItem(R.id.bookmark_this_page_id);
            if (bookmarkItemVisible) {
                updateBookmarkMenuItem(bookmarkItem, currentTab);
            } else {
                bookmarkItem.setVisible(false);
            }

            MenuItem openInChromeItem = menu.findItem(R.id.open_in_browser_id);
            if (openInChromeItemVisible) {
                openInChromeItem.setTitle(
                        DefaultBrowserInfo.getTitleOpenInDefaultBrowser(mIsOpenedByChrome));
            } else {
                openInChromeItem.setVisible(false);
            }

            // Add custom menu items. Make sure they are only added once.
            if (!mIsCustomEntryAdded) {
                mIsCustomEntryAdded = true;
                for (int i = 0; i < mMenuEntries.size(); i++) {
                    MenuItem item = menu.add(0, 0, 1, mMenuEntries.get(i));
                    mItemToIndexMap.put(item, i);
                }
            }

            updateRequestDesktopSiteMenuItem(menu, currentTab, requestDesktopSiteVisible);
            prepareAddToHomescreenMenuItem(menu, currentTab, addToHomeScreenVisible);
        }
    }

    /**
     * @return The index that the given menu item should appear in the result of
     *         {@link CustomTabIntentDataProvider#getMenuTitles()}. Returns -1 if item not found.
     */
    public int getIndexOfMenuItem(MenuItem menuItem) {
        if (!mItemToIndexMap.containsKey(menuItem)) {
            return -1;
        }
        return mItemToIndexMap.get(menuItem).intValue();
    }

    @Override
    public int getFooterResourceId() {
        // Avoid showing the branded menu footer for media and offline pages.
        if (mUiType == CUSTOM_TABS_UI_TYPE_MEDIA_VIEWER
                || mUiType == CUSTOM_TABS_UI_TYPE_OFFLINE_PAGE) {
            return 0;
        }
        return R.layout.powered_by_chrome_footer;
    }

    /**
     * Get the {@link MenuItem} object associated with the given title. If multiple menu items have
     * the same title, a random one will be returned. This method is for testing purpose _only_.
     */
    @VisibleForTesting
    MenuItem getMenuItemForTitle(String title) {
        for (MenuItem item : mItemToIndexMap.keySet()) {
            if (item.getTitle().equals(title)) return item;
        }
        return null;
    }
}
