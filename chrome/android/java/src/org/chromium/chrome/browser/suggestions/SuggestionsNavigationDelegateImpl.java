// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.annotation.Nullable;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.blink_public.web.WebReferrerPolicy;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.bookmarks.BookmarkUtils;
import org.chromium.chrome.browser.device.DeviceClassManager;
import org.chromium.chrome.browser.download.DownloadMetrics;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.offlinepages.DownloadUiActionFlags;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.document.TabDelegate;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.common.Referrer;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.mojom.WindowOpenDisposition;
import org.chromium.ui.widget.Toast;

/**
 * {@link SuggestionsUiDelegate} implementation.
 */
public class SuggestionsNavigationDelegateImpl implements SuggestionsNavigationDelegate {
    private static final String CHROME_CONTENT_SUGGESTIONS_REFERRER =
            "https://www.googleapis.com/auth/chrome-content-suggestions";
    private static final String CHROME_CONTEXTUAL_SUGGESTIONS_REFERRER =
            "https://goto.google.com/explore-on-content-viewer";
    private static final String NEW_TAB_URL_HELP =
            "https://support.google.com/chrome/?p=new_tab";

    private final ChromeActivity mActivity;
    private final Profile mProfile;

    private final NativePageHost mHost;
    private final TabModelSelector mTabModelSelector;

    public SuggestionsNavigationDelegateImpl(ChromeActivity activity, Profile profile,
            NativePageHost host, TabModelSelector tabModelSelector) {
        mActivity = activity;
        mProfile = profile;
        mHost = host;
        mTabModelSelector = tabModelSelector;
    }

    @Override
    public boolean isOpenInNewWindowEnabled() {
        return MultiWindowUtils.getInstance().isOpenInOtherWindowSupported(mActivity);
    }

    @Override
    public boolean isOpenInIncognitoEnabled() {
        return PrefServiceBridge.getInstance().isIncognitoModeEnabled();
    }

    @Override
    public void navigateToBookmarks() {
        RecordUserAction.record("MobileNTPSwitchToBookmarks");
        BookmarkUtils.showBookmarkManager(mActivity);
    }

    @Override
    public void navigateToRecentTabs() {
        RecordUserAction.record("MobileNTPSwitchToOpenTabs");
        mHost.loadUrl(new LoadUrlParams(UrlConstants.RECENT_TABS_URL), /* incognito = */ false);
    }

    @Override
    public void navigateToDownloadManager() {
        RecordUserAction.record("MobileNTPSwitchToDownloadManager");
        DownloadUtils.showDownloadManager(mActivity, mHost.getActiveTab());
    }

    @Override
    public void navigateToHelpPage() {
        NewTabPageUma.recordAction(NewTabPageUma.ACTION_CLICKED_LEARN_MORE);
        // TODO(dgn): Use the standard Help UI rather than a random link to online help?
        openUrl(WindowOpenDisposition.CURRENT_TAB,
                new LoadUrlParams(NEW_TAB_URL_HELP, PageTransition.AUTO_BOOKMARK));
    }

    @Override
    public void navigateToSuggestionUrl(int windowOpenDisposition, String url) {
        LoadUrlParams loadUrlParams = new LoadUrlParams(url, PageTransition.AUTO_BOOKMARK);
        openUrl(windowOpenDisposition, loadUrlParams);
    }

    @Override
    public void openSnippet(final int windowOpenDisposition, final SnippetArticle article) {
        NewTabPageUma.recordAction(NewTabPageUma.ACTION_OPENED_SNIPPET);

        if (article.isAssetDownload()) {
            assert windowOpenDisposition == WindowOpenDisposition.CURRENT_TAB
                    || windowOpenDisposition == WindowOpenDisposition.NEW_WINDOW
                    || windowOpenDisposition == WindowOpenDisposition.NEW_BACKGROUND_TAB;
            DownloadUtils.openFile(article.getAssetDownloadFile(),
                    article.getAssetDownloadMimeType(), article.getAssetDownloadGuid(), false, null,
                    null, DownloadMetrics.NEW_TAP_PAGE);
            return;
        }

        // We explicitly open an offline page only for offline page downloads or for prefetched
        // offline pages when Data Reduction Proxy is enabled. For all other
        // sections the URL is opened and it is up to Offline Pages whether to open its offline
        // page (e.g. when offline).
        if ((article.isDownload() && !article.isAssetDownload())
                || (DataReductionProxySettings.getInstance().isDataReductionProxyEnabled()
                           && article.isPrefetched())) {
            assert article.getOfflinePageOfflineId() != null;
            assert windowOpenDisposition == WindowOpenDisposition.CURRENT_TAB
                    || windowOpenDisposition == WindowOpenDisposition.NEW_WINDOW
                    || windowOpenDisposition == WindowOpenDisposition.NEW_BACKGROUND_TAB;
            OfflinePageUtils.getLoadUrlParamsForOpeningOfflineVersion(
                    article.mUrl, article.getOfflinePageOfflineId(), (loadUrlParams) -> {
                        // Extra headers are not read in loadUrl, but verbatim headers are.
                        loadUrlParams.setVerbatimHeaders(loadUrlParams.getExtraHeadersString());
                        openDownloadSuggestion(windowOpenDisposition, article, loadUrlParams);
                    });

            return;
        }

        LoadUrlParams loadUrlParams =
                new LoadUrlParams(article.mUrl, PageTransition.AUTO_BOOKMARK);

        // For article suggestions, we set the referrer. This is exploited
        // to filter out these history entries for NTP tiles.
        // TODO(mastiz): Extend this with support for other categories.
        if (article.mCategory == KnownCategories.ARTICLES) {
            loadUrlParams.setReferrer(new Referrer(CHROME_CONTENT_SUGGESTIONS_REFERRER,
                    WebReferrerPolicy.ALWAYS));
        }

        // Set appropriate referrer for contextual suggestions to distinguish them from navigation
        // from a page.
        if (article.mCategory == KnownCategories.CONTEXTUAL) {
            loadUrlParams.setReferrer(
                    new Referrer(CHROME_CONTEXTUAL_SUGGESTIONS_REFERRER, WebReferrerPolicy.ALWAYS));
        }

        Tab loadingTab = openUrl(windowOpenDisposition, loadUrlParams);
        if (loadingTab != null) SuggestionsMetrics.recordVisit(loadingTab, article);
    }

    private void openDownloadSuggestion(
            int windowOpenDisposition, SnippetArticle article, LoadUrlParams loadUrlParams) {
        Tab loadingTab = openUrl(windowOpenDisposition, loadUrlParams);
        if (loadingTab != null) SuggestionsMetrics.recordVisit(loadingTab, article);
    }

    @Override
    @Nullable
    public Tab openUrl(int windowOpenDisposition, LoadUrlParams loadUrlParams) {
        Tab loadingTab = null;

        switch (windowOpenDisposition) {
            case WindowOpenDisposition.CURRENT_TAB:
                mHost.loadUrl(loadUrlParams, mTabModelSelector.isIncognitoSelected());
                loadingTab = mHost.getActiveTab();
                break;
            case WindowOpenDisposition.NEW_BACKGROUND_TAB:
                loadingTab = openUrlInNewTab(loadUrlParams);
                break;
            case WindowOpenDisposition.OFF_THE_RECORD:
                mHost.loadUrl(loadUrlParams, true);
                break;
            case WindowOpenDisposition.NEW_WINDOW:
                openUrlInNewWindow(loadUrlParams);
                break;
            case WindowOpenDisposition.SAVE_TO_DISK:
                saveUrlForOffline(loadUrlParams.getUrl());
                break;
            default:
                assert false;
        }

        return loadingTab;
    }

    private void openUrlInNewWindow(LoadUrlParams loadUrlParams) {
        TabDelegate tabDelegate = new TabDelegate(false);
        tabDelegate.createTabInOtherWindow(loadUrlParams, mActivity, mHost.getParentId());
    }

    private Tab openUrlInNewTab(LoadUrlParams loadUrlParams) {
        Tab tab = mTabModelSelector.openNewTab(loadUrlParams,
                TabLaunchType.FROM_LONGPRESS_BACKGROUND, mHost.getActiveTab(),
                /* incognito = */ false);

        // If animations are disabled in the DeviceClassManager, a toast is already displayed for
        // all tabs opened in the background.
        // TODO(twellington): Replace this with an animation.
        if (mActivity.getBottomSheet() != null && DeviceClassManager.enableAnimations()) {
            Toast.makeText(mActivity, R.string.open_in_new_tab_toast, Toast.LENGTH_SHORT).show();
        }

        return tab;
    }

    private void saveUrlForOffline(String url) {
        OfflinePageBridge offlinePageBridge =
                SuggestionsDependencyFactory.getInstance().getOfflinePageBridge(mProfile);
        if (mHost.getActiveTab() != null) {
            offlinePageBridge.scheduleDownload(mHost.getActiveTab().getWebContents(),
                    OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE, url, DownloadUiActionFlags.ALL);
        } else {
            offlinePageBridge.savePageLater(
                    url, OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE, true /* userRequested */);
        }
    }
}
