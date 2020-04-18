// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_DEFAULT;
import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.CUSTOM_TABS_UI_TYPE_INFO_PAGE;
import static org.chromium.chrome.browser.webapps.WebappActivity.ACTIVITY_TYPE_OTHER;
import static org.chromium.chrome.browser.webapps.WebappActivity.ACTIVITY_TYPE_WEBAPK;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.StrictMode;
import android.provider.Browser;
import android.support.annotation.Nullable;
import android.support.customtabs.CustomTabsIntent;
import android.support.customtabs.CustomTabsSessionToken;
import android.support.v4.app.ActivityOptionsCompat;
import android.text.TextUtils;
import android.util.Pair;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.RemoteViews;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.IntentHandler.ExternalAppId;
import org.chromium.chrome.browser.KeyboardShortcuts;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.ServiceTabLauncher;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.WarmupManager;
import org.chromium.chrome.browser.WebContentsFactory;
import org.chromium.chrome.browser.appmenu.AppMenuPropertiesDelegate;
import org.chromium.chrome.browser.browserservices.BrowserSessionContentHandler;
import org.chromium.chrome.browser.browserservices.BrowserSessionContentUtils;
import org.chromium.chrome.browser.compositor.layouts.LayoutManager;
import org.chromium.chrome.browser.datausage.DataUseTabUIManager;
import org.chromium.chrome.browser.externalnav.ExternalNavigationDelegateImpl;
import org.chromium.chrome.browser.firstrun.FirstRunSignInProcessor;
import org.chromium.chrome.browser.fullscreen.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.gsa.GSAState;
import org.chromium.chrome.browser.metrics.PageLoadMetrics;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.page_info.PageInfoController;
import org.chromium.chrome.browser.payments.ServiceWorkerPaymentAppBridge;
import org.chromium.chrome.browser.rappor.RapporServiceBridge;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.tabmodel.AsyncTabParams;
import org.chromium.chrome.browser.tabmodel.AsyncTabParamsManager;
import org.chromium.chrome.browser.tabmodel.ChromeTabCreator;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorImpl;
import org.chromium.chrome.browser.tabmodel.TabReparentingParams;
import org.chromium.chrome.browser.toolbar.ToolbarControlContainer;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.chrome.browser.webapps.WebappCustomTabTimeSpentLogger;
import org.chromium.components.dom_distiller.core.DomDistillerUrlUtils;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationEntry;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * The activity for custom tabs. It will be launched on top of a client's task.
 */
public class CustomTabActivity extends ChromeActivity {
    private static final String TAG = "CustomTabActivity";
    private static final String LAST_URL_PREF = "pref_last_custom_tab_url";

    // For CustomTabs.WebContentsStateOnLaunch, see histograms.xml. Append only.
    private static final int WEBCONTENTS_STATE_NO_WEBCONTENTS = 0;
    private static final int WEBCONTENTS_STATE_PRERENDERED_WEBCONTENTS = 1;
    private static final int WEBCONTENTS_STATE_SPARE_WEBCONTENTS = 2;
    private static final int WEBCONTENTS_STATE_TRANSFERRED_WEBCONTENTS = 3;
    private static final int WEBCONTENTS_STATE_MAX = 4;

    // For CustomTabs.ConnectionStatusOnReturn, see histograms.xml. Append only.
    private static final int CONNECTION_STATUS_DISCONNECTED = 0;
    private static final int CONNECTION_STATUS_DISCONNECTED_KEEP_ALIVE = 1;
    private static final int CONNECTION_STATUS_CONNECTED = 2;
    private static final int CONNECTION_STATUS_CONNECTED_KEEP_ALIVE = 3;
    private static final int CONNECTION_STATUS_MAX = 4;

    private CustomTabIntentDataProvider mIntentDataProvider;
    private CustomTabsSessionToken mSession;
    private BrowserSessionContentHandler mBrowserSessionContentHandler;
    private Tab mMainTab;
    private CustomTabBottomBarDelegate mBottomBarDelegate;
    private CustomTabTabPersistencePolicy mTabPersistencePolicy;

    // This is to give the right package name while using the client's resources during an
    // overridePendingTransition call.
    // TODO(ianwen, yusufo): Figure out a solution to extract external resources without having to
    // change the package name.
    private boolean mShouldOverridePackage;

    private boolean mHasCreatedTabEarly;
    private boolean mIsInitialResume = true;
    // Whether there is any speculative page loading associated with the session.
    private boolean mHasSpeculated;
    private CustomTabObserver mTabObserver;
    private CustomTabNavigationEventObserver mTabNavigationEventObserver;

    private String mSpeculatedUrl;

    private boolean mUsingHiddenTab;

    private boolean mIsClosing;
    private boolean mIsKeepAlive;

    // This boolean is used to do a hack in navigation history for hidden tab loads with
    // unmatching fragments.
    private boolean mIsFirstLoad;

    private final CustomTabsConnection mConnection = CustomTabsConnection.getInstance();

    private WebappCustomTabTimeSpentLogger mWebappTimeSpentLogger;

    private static class PageLoadMetricsObserver implements PageLoadMetrics.Observer {
        private final CustomTabsConnection mConnection;
        private final CustomTabsSessionToken mSession;
        private final Tab mTab;
        private final CustomTabObserver mCustomTabObserver;

        public PageLoadMetricsObserver(CustomTabsConnection connection,
                CustomTabsSessionToken session, Tab tab, CustomTabObserver tabObserver) {
            mConnection = connection;
            mSession = session;
            mTab = tab;
            mCustomTabObserver = tabObserver;
        }

        @Override
        public void onNewNavigation(WebContents webContents, long navigationId) {}

        @Override
        public void onNetworkQualityEstimate(WebContents webContents, long navigationId,
                int effectiveConnectionType, long httpRttMs, long transportRttMs) {
            if (webContents != mTab.getWebContents()) return;

            Bundle args = new Bundle();
            args.putLong(PageLoadMetrics.EFFECTIVE_CONNECTION_TYPE, effectiveConnectionType);
            args.putLong(PageLoadMetrics.HTTP_RTT, httpRttMs);
            args.putLong(PageLoadMetrics.TRANSPORT_RTT, transportRttMs);
            args.putBoolean(CustomTabsConnection.DATA_REDUCTION_ENABLED,
                    DataReductionProxySettings.getInstance().isDataReductionProxyEnabled());
            mConnection.notifyPageLoadMetrics(mSession, args);
        }

        @Override
        public void onFirstContentfulPaint(WebContents webContents, long navigationId,
                long navigationStartTick, long firstContentfulPaintMs) {
            if (webContents != mTab.getWebContents()) return;

            mConnection.notifySinglePageLoadMetric(mSession, PageLoadMetrics.FIRST_CONTENTFUL_PAINT,
                    navigationStartTick, firstContentfulPaintMs);
        }

        @Override
        public void onFirstMeaningfulPaint(WebContents webContents, long navigationId,
                long navigationStartTick, long firstContentfulPaintMs) {
            if (webContents != mTab.getWebContents()) return;

            mCustomTabObserver.onFirstMeaningfulPaint(mTab);
        }

        @Override
        public void onLoadEventStart(WebContents webContents, long navigationId,
                long navigationStartTick, long loadEventStartMs) {
            if (webContents != mTab.getWebContents()) return;
            mConnection.notifySinglePageLoadMetric(mSession, PageLoadMetrics.LOAD_EVENT_START,
                    navigationStartTick, loadEventStartMs);
        }

        @Override
        public void onLoadedMainResource(WebContents webContents, long navigationId,
                long dnsStartMs, long dnsEndMs, long connectStartMs, long connectEndMs,
                long requestStartMs, long sendStartMs, long sendEndMs) {
            if (webContents != mTab.getWebContents()) return;

            Bundle args = new Bundle();
            args.putLong(PageLoadMetrics.DOMAIN_LOOKUP_START, dnsStartMs);
            args.putLong(PageLoadMetrics.DOMAIN_LOOKUP_END, dnsEndMs);
            args.putLong(PageLoadMetrics.CONNECT_START, connectStartMs);
            args.putLong(PageLoadMetrics.CONNECT_END, connectEndMs);
            args.putLong(PageLoadMetrics.REQUEST_START, requestStartMs);
            args.putLong(PageLoadMetrics.SEND_START, sendStartMs);
            args.putLong(PageLoadMetrics.SEND_END, sendEndMs);
            mConnection.notifyPageLoadMetrics(mSession, args);
        }
    }

    private static class CustomTabCreator extends ChromeTabCreator {
        private final boolean mSupportsUrlBarHiding;
        private final boolean mIsOpenedByChrome;
        private final BrowserStateBrowserControlsVisibilityDelegate mVisibilityDelegate;

        public CustomTabCreator(
                ChromeActivity activity, WindowAndroid nativeWindow, boolean incognito,
                boolean supportsUrlBarHiding, boolean isOpenedByChrome) {
            super(activity, nativeWindow, incognito);
            mSupportsUrlBarHiding = supportsUrlBarHiding;
            mIsOpenedByChrome = isOpenedByChrome;
            mVisibilityDelegate = activity.getFullscreenManager().getBrowserVisibilityDelegate();
        }

        @Override
        public TabDelegateFactory createDefaultTabDelegateFactory() {
            return new CustomTabDelegateFactory(
                    mSupportsUrlBarHiding, mIsOpenedByChrome, mVisibilityDelegate);
        }
    }

    private PageLoadMetricsObserver mMetricsObserver;

    // Only the normal tab model is observed because there is no incognito tabmodel in Custom Tabs.
    private TabModelObserver mTabModelObserver = new EmptyTabModelObserver() {
        @Override
        public void didAddTab(Tab tab, TabLaunchType type) {
            // Ensure that the PageLoadMetrics observer is attached in all cases, if in
            // the future we do not go through initializeMainTab. ObserverList.addObserver
            // will deduplicate additions, so it is safe to add both here as well as in
            // initializeMainTab().
            PageLoadMetrics.addObserver(mMetricsObserver);
            tab.addObserver(mTabObserver);
            tab.addObserver(mTabNavigationEventObserver);
        }

        @Override
        public void didCloseTab(int tabId, boolean incognito) {
            PageLoadMetrics.removeObserver(mMetricsObserver);
            // Finish the activity after we intent out.
            if (getTabModelSelector().getCurrentModel().getCount() == 0) finishAndClose(false);
        }

        @Override
        public void tabRemoved(Tab tab) {
            tab.removeObserver(mTabObserver);
            tab.removeObserver(mTabNavigationEventObserver);
            PageLoadMetrics.removeObserver(mMetricsObserver);
        }
    };

    @Override
    protected Drawable getBackgroundDrawable() {
        int initialBackgroundColor = mIntentDataProvider.getInitialBackgroundColor();
        if (mIntentDataProvider.isTrustedIntent() && initialBackgroundColor != Color.TRANSPARENT) {
            return new ColorDrawable(initialBackgroundColor);
        } else {
            return super.getBackgroundDrawable();
        }
    }

    @Override
    public boolean isCustomTab() {
        return true;
    }

    @Override
    protected void recordIntentToCreationTime(long timeMs) {
        super.recordIntentToCreationTime(timeMs);

        RecordHistogram.recordTimesHistogram(
                "MobileStartup.IntentToCreationTime.CustomTabs", timeMs, TimeUnit.MILLISECONDS);
    }

    @Override
    public void onStart() {
        super.onStart();
        mIsClosing = false;
        mIsKeepAlive = mConnection.keepAliveForSession(
                mIntentDataProvider.getSession(), mIntentDataProvider.getKeepAliveServiceIntent());
    }

    @Override
    public void onStop() {
        super.onStop();
        mConnection.dontKeepAliveForSession(mIntentDataProvider.getSession());
        mIsKeepAlive = false;
    }

    @Override
    public void preInflationStartup() {
        // Parse the data from the Intent before calling super to allow the Intent to customize
        // the Activity parameters, including the background of the page.
        mIntentDataProvider = new CustomTabIntentDataProvider(getIntent(), this);

        super.preInflationStartup();
        mSession = mIntentDataProvider.getSession();
        supportRequestWindowFeature(Window.FEATURE_ACTION_MODE_OVERLAY);
        mSpeculatedUrl = mConnection.getSpeculatedUrl(mSession);
        mHasSpeculated = !TextUtils.isEmpty(mSpeculatedUrl);
        if (getSavedInstanceState() == null && CustomTabsConnection.hasWarmUpBeenFinished()) {
            initializeTabModels();
            mMainTab = getHiddenTab();
            if (mMainTab == null) mMainTab = createMainTab();
            mIsFirstLoad = true;
            loadUrlInTab(mMainTab, new LoadUrlParams(getUrlToLoad()),
                    IntentHandler.getTimestampFromIntent(getIntent()));
            mHasCreatedTabEarly = true;
        }
    }

    @Override
    public boolean shouldAllocateChildConnection() {
        return !mHasCreatedTabEarly && !mHasSpeculated
                && !WarmupManager.getInstance().hasSpareWebContents();
    }

    @Override
    public void postInflationStartup() {
        super.postInflationStartup();

        getToolbarManager().setCloseButtonDrawable(mIntentDataProvider.getCloseButtonDrawable());
        getToolbarManager().setShowTitle(mIntentDataProvider.getTitleVisibilityState()
                == CustomTabsIntent.SHOW_PAGE_TITLE);
        if (mConnection.shouldHideDomainForSession(mSession)) {
            getToolbarManager().setUrlBarHidden(true);
        }
        int toolbarColor = mIntentDataProvider.getToolbarColor();
        getToolbarManager().updatePrimaryColor(toolbarColor, false);
        if (!mIntentDataProvider.isOpenedByChrome()) {
            getToolbarManager().setShouldUpdateToolbarPrimaryColor(false);
        }
        if (toolbarColor != ApiCompatibilityUtils.getColor(
                getResources(), R.color.default_primary_color)) {
            ApiCompatibilityUtils.setStatusBarColor(getWindow(),
                    ColorUtils.getDarkenedColorForStatusBar(toolbarColor));
        }
        // Properly attach tab's infobar to the view hierarchy, as the main tab might have been
        // initialized prior to inflation.
        if (mMainTab != null) {
            ViewGroup bottomContainer = (ViewGroup) findViewById(R.id.bottom_container);
            mMainTab.getInfoBarContainer().setParentView(bottomContainer);
        }

        // Setting task title and icon to be null will preserve the client app's title and icon.
        ApiCompatibilityUtils.setTaskDescription(this, null, null, toolbarColor);
        showCustomButtonsOnToolbar();
        mBottomBarDelegate = new CustomTabBottomBarDelegate(this, mIntentDataProvider,
                getFullscreenManager());
        mBottomBarDelegate.showBottomBarIfNecessary();
    }

    @Override
    protected TabModelSelector createTabModelSelector() {
        mTabPersistencePolicy = new CustomTabTabPersistencePolicy(
                getTaskId(), getSavedInstanceState() != null);

        return new TabModelSelectorImpl(this, this, mTabPersistencePolicy, false, false);
    }

    @Override
    protected Pair<CustomTabCreator, CustomTabCreator> createTabCreators() {
        return Pair.create(
                new CustomTabCreator(
                        this, getWindowAndroid(), false,
                        mIntentDataProvider.shouldEnableUrlBarHiding(),
                        mIntentDataProvider.isOpenedByChrome()),
                new CustomTabCreator(
                        this, getWindowAndroid(), true,
                        mIntentDataProvider.shouldEnableUrlBarHiding(),
                        mIntentDataProvider.isOpenedByChrome()));
    }

    @Override
    public void finishNativeInitialization() {
        if (!mIntentDataProvider.isInfoPage()) FirstRunSignInProcessor.start(this);

        // If extra headers have been passed, cancel any current speculation, as
        // speculation doesn't support extra headers.
        if (IntentHandler.getExtraHeadersFromIntent(getIntent()) != null) {
            mConnection.cancelSpeculation(mSession);
        }

        getTabModelSelector()
                .getModel(mIntentDataProvider.isIncognito())
                .addObserver(mTabModelObserver);

        boolean successfulStateRestore = false;
        // Attempt to restore the previous tab state if applicable.
        if (getSavedInstanceState() != null) {
            assert mMainTab == null;
            getTabModelSelector().loadState(true);
            getTabModelSelector().restoreTabs(true);
            mMainTab = getTabModelSelector().getCurrentTab();
            successfulStateRestore = mMainTab != null;
            if (successfulStateRestore) initializeMainTab(mMainTab);
        }

        // If no tab was restored, create a new tab.
        if (!successfulStateRestore) {
            if (mHasCreatedTabEarly) {
                // When the tab is created early, we don't have the TabContentManager connected,
                // since compositor related controllers were not initialized at that point.
                mMainTab.attachTabContentManager(getTabContentManager());
            } else {
                mMainTab = createMainTab();
            }
            getTabModelSelector()
                    .getModel(mIntentDataProvider.isIncognito())
                    .addTab(mMainTab, 0, mMainTab.getLaunchType());
        }

        // This cannot be done before because we want to do the reparenting only
        // when we have compositor related controllers.
        if (mUsingHiddenTab) {
            TabReparentingParams params =
                    (TabReparentingParams) AsyncTabParamsManager.remove(mMainTab.getId());
            mMainTab.attachAndFinishReparenting(this,
                    new CustomTabDelegateFactory(mIntentDataProvider.shouldEnableUrlBarHiding(),
                            mIntentDataProvider.isOpenedByChrome(),
                            getFullscreenManager().getBrowserVisibilityDelegate()),
                    (params == null ? null : params.getFinalizeCallback()));
        }

        LayoutManager layoutDriver = new LayoutManager(getCompositorViewHolder());
        initializeCompositorContent(layoutDriver, findViewById(R.id.url_bar),
                (ViewGroup) findViewById(android.R.id.content),
                (ToolbarControlContainer) findViewById(R.id.control_container));
        getToolbarManager().initializeWithNative(getTabModelSelector(),
                getFullscreenManager().getBrowserVisibilityDelegate(), getFindToolbarManager(),
                null, layoutDriver, null, null, null, new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        RecordUserAction.record("CustomTabs.CloseButtonClicked");
                        if (mIntentDataProvider.shouldEnableEmbeddedMediaExperience()) {
                            RecordUserAction.record("CustomTabs.CloseButtonClicked.DownloadsUI");
                        }
                        recordClientConnectionStatus();
                        finishAndClose(false);
                    }
                }, null);

        mBrowserSessionContentHandler = new BrowserSessionContentHandler() {
            @Override
            public void loadUrlAndTrackFromTimestamp(LoadUrlParams params, long timestamp) {
                if (!TextUtils.isEmpty(params.getUrl())) {
                    params.setUrl(DataReductionProxySettings.getInstance()
                            .maybeRewriteWebliteUrl(params.getUrl()));
                }
                loadUrlInTab(getActivityTab(), params, timestamp);
            }

            @Override
            public CustomTabsSessionToken getSession() {
                return mSession;
            }

            @Override
            public boolean shouldIgnoreIntent(Intent intent) {
                return mIntentHandler.shouldIgnoreIntent(intent);
            }

            @Override
            public boolean updateCustomButton(int id, Bitmap bitmap, String description) {
                CustomButtonParams params = mIntentDataProvider.getButtonParamsForId(id);
                if (params == null) {
                    Log.w(TAG, "Custom toolbar button with ID %d not found", id);
                    return false;
                }

                params.update(bitmap, description);
                if (params.showOnToolbar()) {
                    if (!CustomButtonParams.doesIconFitToolbar(CustomTabActivity.this, bitmap)) {
                        return false;
                    }
                    int index = mIntentDataProvider.getCustomToolbarButtonIndexForId(id);
                    assert index != -1;
                    getToolbarManager().updateCustomActionButton(
                            index, params.getIcon(getResources()), description);
                } else {
                    if (mBottomBarDelegate != null) {
                        mBottomBarDelegate.updateBottomBarButtons(params);
                    }
                }
                return true;
            }

            @Override
            public boolean updateRemoteViews(RemoteViews remoteViews, int[] clickableIDs,
                    PendingIntent pendingIntent) {
                if (mBottomBarDelegate == null) return false;
                return mBottomBarDelegate.updateRemoteViews(remoteViews, clickableIDs,
                        pendingIntent);
            }

            @Override
            @Nullable
            public String getCurrentUrl() {
                return getActivityTab() == null ? null : getActivityTab().getUrl();
            }

            @Override
            @Nullable
            public String getPendingUrl() {
                if (getActivityTab() == null) return null;
                if (getActivityTab().getWebContents() == null) return null;

                NavigationEntry entry = getActivityTab().getWebContents().getNavigationController()
                        .getPendingEntry();
                return entry != null ? entry.getUrl() : null;
            }
        };
        recordClientPackageName();
        mConnection.showSignInToastIfNecessary(mSession, getIntent());
        String url = getUrlToLoad();
        String packageName = mConnection.getClientPackageNameForSession(mSession);
        if (TextUtils.isEmpty(packageName)) {
            packageName = mConnection.extractCreatorPackage(getIntent());
        }
        DataUseTabUIManager.onCustomTabInitialNavigation(mMainTab, packageName, url);

        if (!mHasCreatedTabEarly && !successfulStateRestore && !mMainTab.isLoading()) {
            loadUrlInTab(mMainTab, new LoadUrlParams(url),
                    IntentHandler.getTimestampFromIntent(getIntent()));
        }

        // Put Sync in the correct state by calling tab state initialized. crbug.com/581811.
        getTabModelSelector().markTabStateInitialized();

        // Notify ServiceTabLauncher if this is an asynchronous tab launch.
        if (getIntent().hasExtra(ServiceTabLauncher.LAUNCH_REQUEST_ID_EXTRA)) {
            ServiceTabLauncher.onWebContentsForRequestAvailable(
                    getIntent().getIntExtra(ServiceTabLauncher.LAUNCH_REQUEST_ID_EXTRA, 0),
                    getActivityTab().getWebContents());
        }
        super.finishNativeInitialization();
    }

    /**
     * Encapsulates CustomTabsConnection#takeHiddenTab()
     * with additional initialization logic.
     */
    private Tab getHiddenTab() {
        String url = getUrlToLoad();
        String referrerUrl = mConnection.getReferrer(mSession, getIntent());
        Tab tab = mConnection.takeHiddenTab(mSession, url, referrerUrl);
        mUsingHiddenTab = tab != null;
        if (!mUsingHiddenTab) return null;
        RecordHistogram.recordEnumeratedHistogram("CustomTabs.WebContentsStateOnLaunch",
                WEBCONTENTS_STATE_PRERENDERED_WEBCONTENTS, WEBCONTENTS_STATE_MAX);
        tab.setAppAssociatedWith(mConnection.getClientPackageNameForSession(mSession));
        if (mIntentDataProvider.shouldEnableEmbeddedMediaExperience()) {
            tab.enableEmbeddedMediaExperience(true);
        }
        initializeMainTab(tab);
        return tab;
    }

    private Tab createMainTab() {
        WebContents webContents = takeWebContents();

        int assignedTabId = IntentUtils.safeGetIntExtra(
                getIntent(), IntentHandler.EXTRA_TAB_ID, Tab.INVALID_TAB_ID);
        int parentTabId = IntentUtils.safeGetIntExtra(
                getIntent(), IntentHandler.EXTRA_PARENT_TAB_ID, Tab.INVALID_TAB_ID);
        Tab tab = new Tab(assignedTabId, parentTabId, mIntentDataProvider.isIncognito(),
                getWindowAndroid(), TabLaunchType.FROM_EXTERNAL_APP, null, null);
        if (getIntent().getIntExtra(
                    CustomTabIntentDataProvider.EXTRA_BROWSER_LAUNCH_SOURCE, ACTIVITY_TYPE_OTHER)
                == ACTIVITY_TYPE_WEBAPK) {
            String webapkPackageName = getIntent().getStringExtra(Browser.EXTRA_APPLICATION_ID);
            tab.setAppAssociatedWith(webapkPackageName);
        } else {
            tab.setAppAssociatedWith(mConnection.getClientPackageNameForSession(mSession));
        }
        tab.initialize(
                webContents, getTabContentManager(),
                new CustomTabDelegateFactory(
                        mIntentDataProvider.shouldEnableUrlBarHiding(),
                        mIntentDataProvider.isOpenedByChrome(),
                        getFullscreenManager().getBrowserVisibilityDelegate()),
                false, false);

        if (mIntentDataProvider.shouldEnableEmbeddedMediaExperience()) {
            tab.enableEmbeddedMediaExperience(true);
        }

        initializeMainTab(tab);
        return tab;
    }

    private WebContents takeWebContents() {
        int webContentsStateOnLaunch = WEBCONTENTS_STATE_PRERENDERED_WEBCONTENTS;

        WebContents webContents = takeAsyncWebContents();
        if (webContents != null) {
            webContentsStateOnLaunch = WEBCONTENTS_STATE_TRANSFERRED_WEBCONTENTS;
            webContents.resumeLoadingCreatedWebContents();
        } else {
            webContents = WarmupManager.getInstance().takeSpareWebContents(
                    mIntentDataProvider.isIncognito(), false);
            if (webContents != null) {
                webContentsStateOnLaunch = WEBCONTENTS_STATE_SPARE_WEBCONTENTS;
            } else {
                webContents = WebContentsFactory.createWebContentsWithWarmRenderer(
                        mIntentDataProvider.isIncognito(), false);
                webContentsStateOnLaunch = WEBCONTENTS_STATE_NO_WEBCONTENTS;
            }
        }

        RecordHistogram.recordEnumeratedHistogram("CustomTabs.WebContentsStateOnLaunch",
                webContentsStateOnLaunch, WEBCONTENTS_STATE_MAX);

        mConnection.resetPostMessageHandlerForSession(mSession, webContents);

        return webContents;
    }

    private WebContents takeAsyncWebContents() {
        int assignedTabId = IntentUtils.safeGetIntExtra(
                getIntent(), IntentHandler.EXTRA_TAB_ID, Tab.INVALID_TAB_ID);
        AsyncTabParams asyncParams = AsyncTabParamsManager.remove(assignedTabId);
        if (asyncParams == null) return null;
        return asyncParams.getWebContents();
    }

    private void initializeMainTab(Tab tab) {
        tab.getTabRedirectHandler().updateIntent(getIntent());
        tab.getView().requestFocus();
        mTabObserver = new CustomTabObserver(
                getApplication(), mSession, mIntentDataProvider.isOpenedByChrome());
        mTabNavigationEventObserver = new CustomTabNavigationEventObserver(mSession);

        mMetricsObserver = new PageLoadMetricsObserver(mConnection, mSession, tab, mTabObserver);
        // Immediately add the observer to PageLoadMetrics to catch early events that may
        // be generated in the middle of tab initialization.
        PageLoadMetrics.addObserver(mMetricsObserver);
        tab.addObserver(mTabObserver);
        tab.addObserver(mTabNavigationEventObserver);

        // Let ServiceWorkerPaymentAppBridge observe the opened tab for payment request.
        if (mIntentDataProvider.isForPaymentRequest()) {
            ServiceWorkerPaymentAppBridge.addTabObserverForPaymentRequestTab(tab);
        }

        prepareTabBackground(tab);
    }

    @Override
    public void initializeCompositor() {
        super.initializeCompositor();
        getTabModelSelector().onNativeLibraryReady(getTabContentManager());
        mBottomBarDelegate.addContextualSearchObserver();
    }

    private void recordClientPackageName() {
        String clientName = mConnection.getClientPackageNameForSession(mSession);
        if (TextUtils.isEmpty(clientName)) clientName = mIntentDataProvider.getClientPackageName();
        final String packageName = clientName;
        if (TextUtils.isEmpty(packageName) || packageName.contains(getPackageName())) return;
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                RapporServiceBridge.sampleString(
                        "CustomTabs.ServiceClient.PackageName", packageName);
                if (GSAState.isGsaPackageName(packageName)) return;
                RapporServiceBridge.sampleString(
                        "CustomTabs.ServiceClient.PackageNameThirdParty", packageName);
            }
        });
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();
        BrowserSessionContentUtils.setActiveContentHandler(mBrowserSessionContentHandler);
        if (mHasCreatedTabEarly && !mMainTab.isLoading()) postDeferredStartupIfNeeded();
    }

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();

        if (getSavedInstanceState() != null || !mIsInitialResume) {
            if (mIntentDataProvider.isOpenedByChrome()) {
                RecordUserAction.record("ChromeGeneratedCustomTab.StartedReopened");
            } else {
                RecordUserAction.record("CustomTabs.StartedReopened");
            }
        } else {
            SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
            String lastUrl = preferences.getString(LAST_URL_PREF, null);
            if (lastUrl != null && lastUrl.equals(getUrlToLoad())) {
                RecordUserAction.record("CustomTabsMenuOpenSameUrl");
            } else {
                preferences.edit().putString(LAST_URL_PREF, getUrlToLoad()).apply();
            }

            if (mIntentDataProvider.isOpenedByChrome()) {
                RecordUserAction.record("ChromeGeneratedCustomTab.StartedInitially");
            } else {
                ExternalAppId externalId = IntentHandler.determineExternalIntentSource(getIntent());
                RecordHistogram.recordEnumeratedHistogram("CustomTabs.ClientAppId",
                        externalId.ordinal(), ExternalAppId.INDEX_BOUNDARY.ordinal());

                RecordUserAction.record("CustomTabs.StartedInitially");
            }
        }
        mIsInitialResume = false;
        mWebappTimeSpentLogger = WebappCustomTabTimeSpentLogger.createInstanceAndStartTimer(
                getIntent().getIntExtra(CustomTabIntentDataProvider.EXTRA_BROWSER_LAUNCH_SOURCE,
                        ACTIVITY_TYPE_OTHER));
    }

    @Override
    public void onPauseWithNative() {
        super.onPauseWithNative();
        if (mWebappTimeSpentLogger != null) {
            mWebappTimeSpentLogger.onPause();
        }
    }

    @Override
    public void onStopWithNative() {
        super.onStopWithNative();
        BrowserSessionContentUtils.setActiveContentHandler(null);
        if (mIsClosing) {
            getTabModelSelector().closeAllTabs(true);
            mTabPersistencePolicy.deleteMetadataStateFileAsync();
        } else {
            getTabModelSelector().saveState();
        }
    }

    /**
     * Loads the current tab with the given load params while taking client
     * referrer and extra headers into account.
     */
    private void loadUrlInTab(final Tab tab, final LoadUrlParams params, long timeStamp) {
        Intent intent = getIntent();
        String url = getUrlToLoad();

        // Caching isFirstLoad value to deal with multiple return points.
        boolean isFirstLoad = mIsFirstLoad;
        mIsFirstLoad = false;

        // The following block is a hack that deals with urls preloaded with
        // the wrong fragment. Does an extra pageload and replaces history.
        if (mHasSpeculated && isFirstLoad
                && UrlUtilities.urlsFragmentsDiffer(mSpeculatedUrl, url)) {
            params.setShouldReplaceCurrentEntry(true);
        }

        mTabObserver.trackNextPageLoadFromTimestamp(tab, timeStamp);

        // Manually generating metrics in case the hidden tab has completely finished loading.
        if (mUsingHiddenTab && !tab.isLoading() && !tab.isShowingErrorPage()) {
            mTabObserver.onPageLoadStarted(tab, params.getUrl());
            mTabObserver.onPageLoadFinished(tab);
            mTabNavigationEventObserver.onPageLoadStarted(tab, params.getUrl());
            mTabNavigationEventObserver.onPageLoadFinished(tab);
        }

        // No actual load to do if tab already has the exact correct url.
        if (TextUtils.equals(mSpeculatedUrl, params.getUrl()) && mUsingHiddenTab && isFirstLoad) {
            return;
        }

        IntentHandler.addReferrerAndHeaders(params, intent);
        if (params.getReferrer() == null) {
            params.setReferrer(mConnection.getReferrerForSession(mSession));
        }
        // See ChromeTabCreator#getTransitionType(). This marks the navigation chain as starting
        // from an external intent (unless otherwise specified by an extra in the intent).
        params.setTransitionType(IntentHandler.getTransitionTypeFromIntent(intent,
                PageTransition.LINK | PageTransition.FROM_API));
        tab.loadUrl(params);
    }

    @Override
    public void createContextualSearchTab(String searchUrl) {
        if (getActivityTab() == null) return;
        getActivityTab().loadUrl(new LoadUrlParams(searchUrl));
    }

    @Override
    public TabModelSelectorImpl getTabModelSelector() {
        return (TabModelSelectorImpl) super.getTabModelSelector();
    }

    @Override
    public Tab getActivityTab() {
        Tab tab = super.getActivityTab();
        if (tab == null) tab = mMainTab;
        return tab;
    }

    @Override
    protected AppMenuPropertiesDelegate createAppMenuPropertiesDelegate() {
        return new CustomTabAppMenuPropertiesDelegate(this,
                mIntentDataProvider.getUiType(),
                mIntentDataProvider.getMenuTitles(),
                mIntentDataProvider.isOpenedByChrome(),
                mIntentDataProvider.shouldShowShareMenuItem(),
                mIntentDataProvider.shouldShowStarButton(),
                mIntentDataProvider.shouldShowDownloadButton());
    }

    @Override
    protected int getAppMenuLayoutId() {
        return R.menu.custom_tabs_menu;
    }

    @Override
    protected int getControlContainerLayoutId() {
        return R.layout.custom_tabs_control_container;
    }

    @Override
    protected int getToolbarLayoutId() {
        return R.layout.custom_tabs_toolbar;
    }

    @Override
    public int getControlContainerHeightResource() {
        return R.dimen.custom_tabs_control_container_height;
    }

    @Override
    public String getPackageName() {
        if (mShouldOverridePackage) return mIntentDataProvider.getClientPackageName();
        return super.getPackageName();
    }

    @Override
    public void finish() {
        // Prevent the menu window from leaking.
        if (getAppMenuHandler() != null) getAppMenuHandler().hideAppMenu();

        super.finish();
        if (mIntentDataProvider != null && mIntentDataProvider.shouldAnimateOnFinish()) {
            mShouldOverridePackage = true;
            overridePendingTransition(mIntentDataProvider.getAnimationEnterRes(),
                    mIntentDataProvider.getAnimationExitRes());
            mShouldOverridePackage = false;
        } else if (mIntentDataProvider != null && mIntentDataProvider.isOpenedByChrome()) {
            overridePendingTransition(R.anim.no_anim, R.anim.activity_close_exit);
        }
    }

    /**
     * Finishes the activity and removes the reference from the Android recents.
     *
     * @param reparenting true iff the activity finishes due to tab reparenting.
     */
    public final void finishAndClose(boolean reparenting) {
        if (mIsClosing) return;
        mIsClosing = true;

        // Notify the window is closing so as to abort invoking payment app early.
        if (mIntentDataProvider.isForPaymentRequest()
                && getActivityTab().getWebContents() != null) {
            ServiceWorkerPaymentAppBridge.onClosingPaymentAppWindow(
                    getActivityTab().getWebContents());
        }

        if (!reparenting) {
            // Closing the activity destroys the renderer as well. Re-create a spare renderer some
            // time after, so that we have one ready for the next tab open. This does not increase
            // memory consumption, as the current renderer goes away. We create a renderer as a lot
            // of users open several Custom Tabs in a row. The delay is there to avoid jank in the
            // transition animation when closing the tab.
            ThreadUtils.postOnUiThreadDelayed(new Runnable() {
                @Override
                public void run() {
                    WarmupManager.getInstance().createSpareWebContents();
                }
            }, 500);
        }

        handleFinishAndClose();
    }

    /**
     * Internal implementation that finishes the activity and removes the references from Android
     * recents.
     */
    protected void handleFinishAndClose() {
        // When on top of another app, finish is all that is required.
        finish();
    }

    @Override
    protected boolean handleBackPressed() {
        if (!LibraryLoader.isInitialized()) return false;

        RecordUserAction.record("CustomTabs.SystemBack");
        if (getActivityTab() == null) return false;

        if (exitFullscreenIfShowing()) return true;

        if (!getToolbarManager().back()) {
            if (getCurrentTabModel().getCount() > 1) {
                getCurrentTabModel().closeTab(getActivityTab(), false, false, false);
            } else {
                recordClientConnectionStatus();
                finishAndClose(false);
            }
        }
        return true;
    }

    private void recordClientConnectionStatus() {
        String packageName =
                (getActivityTab() == null) ? null : getActivityTab().getAppAssociatedWith();
        if (packageName == null) return; // No associated package

        boolean isConnected = packageName.equals(
                CustomTabsConnection.getInstance().getClientPackageNameForSession(mSession));
        int status = -1;
        if (isConnected) {
            if (mIsKeepAlive) {
                status = CONNECTION_STATUS_CONNECTED_KEEP_ALIVE;
            } else {
                status = CONNECTION_STATUS_CONNECTED;
            }
        } else {
            if (mIsKeepAlive) {
                status = CONNECTION_STATUS_DISCONNECTED_KEEP_ALIVE;
            } else {
                status = CONNECTION_STATUS_DISCONNECTED;
            }
        }
        assert status >= 0;

        if (GSAState.isGsaPackageName(packageName)) {
            RecordHistogram.recordEnumeratedHistogram(
                    "CustomTabs.ConnectionStatusOnReturn.GSA", status, CONNECTION_STATUS_MAX);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "CustomTabs.ConnectionStatusOnReturn.NonGSA", status, CONNECTION_STATUS_MAX);
        }
    }

    /**
     * Configures the custom button on toolbar. Does nothing if invalid data is provided by clients.
     */
    private void showCustomButtonsOnToolbar() {
        final List<CustomButtonParams> paramList = mIntentDataProvider.getCustomButtonsOnToolbar();
        for (CustomButtonParams params : paramList) {
            getToolbarManager().addCustomActionButton(
                    params.getIcon(getResources()), params.getDescription(), v -> {
                        if (getActivityTab() == null) return;
                        mIntentDataProvider.sendButtonPendingIntentWithUrlAndTitle(
                                getApplicationContext(), params, getActivityTab().getUrl(),
                                getActivityTab().getTitle());
                        RecordUserAction.record("CustomTabsCustomActionButtonClick");
                        if (mIntentDataProvider.shouldEnableEmbeddedMediaExperience()
                                && TextUtils.equals(
                                           params.getDescription(), getString(R.string.share))) {
                            RecordUserAction.record(
                                    "CustomTabsCustomActionButtonClick.DownloadsUI.Share");
                        }
                    });
        }
    }

    @Override
    public boolean shouldShowAppMenu() {
        if (getActivityTab() == null || !getToolbarManager().isInitialized()) return false;

        return super.shouldShowAppMenu();
    }

    @Override
    protected void showAppMenuForKeyboardEvent() {
        if (!shouldShowAppMenu()) return;
        super.showAppMenuForKeyboardEvent();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int menuIndex = getAppMenuPropertiesDelegate().getIndexOfMenuItem(item);
        if (menuIndex >= 0) {
            mIntentDataProvider.clickMenuItemWithUrlAndTitle(
                    this, menuIndex, getActivityTab().getUrl(), getActivityTab().getTitle());
            RecordUserAction.record("CustomTabsMenuCustomMenuItem");
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        Boolean result = KeyboardShortcuts.dispatchKeyEvent(event, this,
                getToolbarManager().isInitialized());
        return result != null ? result : super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!getToolbarManager().isInitialized()) {
            return super.onKeyDown(keyCode, event);
        }
        return KeyboardShortcuts.onKeyDown(event, this, true, false)
                || super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        // Disable creating new tabs, bookmark, history, print, help, focus_url, etc.
        if (id == R.id.focus_url_bar || id == R.id.all_bookmarks_menu_id
                || id == R.id.help_id || id == R.id.recent_tabs_menu_id
                || id == R.id.new_incognito_tab_menu_id || id == R.id.new_tab_menu_id
                || id == R.id.open_history_menu_id) {
            return true;
        } else if (id == R.id.bookmark_this_page_id) {
            addOrEditBookmark(getActivityTab());
            RecordUserAction.record("MobileMenuAddToBookmarks");
            return true;
        } else if (id == R.id.open_in_browser_id) {
            if (openCurrentUrlInBrowser(false)) {
                RecordUserAction.record("CustomTabsMenuOpenInChrome");
                mConnection.notifyOpenInBrowser(mSession);
            }
            return true;
        } else if (id == R.id.info_menu_id) {
            if (getTabModelSelector().getCurrentTab() == null) return false;
            PageInfoController.show(this, getTabModelSelector().getCurrentTab(),
                    getToolbarManager().getContentPublisher(), PageInfoController.OPENED_FROM_MENU);
            return true;
        }
        return super.onMenuOrKeyboardAction(id, fromMenu);
    }

    @Override
    protected void setStatusBarColor(Tab tab, int color) {
        // Intentionally do nothing as CustomTabActivity explicitly sets status bar color.  Except
        // for Custom Tabs opened by Chrome.
        if (mIntentDataProvider.isOpenedByChrome()) super.setStatusBarColor(tab, color);
    }

    /**
     * @return The {@link AppMenuPropertiesDelegate} associated with this activity. For test
     *         purposes only.
     */
    @VisibleForTesting
    @Override
    public CustomTabAppMenuPropertiesDelegate getAppMenuPropertiesDelegate() {
        return (CustomTabAppMenuPropertiesDelegate) super.getAppMenuPropertiesDelegate();
    }

    @Override
    public void onCheckForUpdate(boolean updateAvailable) {
    }

    /**
     * @return The {@link CustomTabIntentDataProvider} for this {@link CustomTabActivity}. For test
     *         purposes only.
     */
    @VisibleForTesting
    CustomTabIntentDataProvider getIntentDataProvider() {
        return mIntentDataProvider;
    }

    /**
     * @return The tab persistence policy for this activity.
     */
    @VisibleForTesting
    CustomTabTabPersistencePolicy getTabPersistencePolicyForTest() {
        return mTabPersistencePolicy;
    }

    /**
     * Opens the URL currently being displayed in the Custom Tab in the regular browser.
     * @param forceReparenting Whether tab reparenting should be forced for testing.
     *
     * @return Whether or not the tab was sent over successfully.
     */
    boolean openCurrentUrlInBrowser(boolean forceReparenting) {
        Tab tab = getActivityTab();
        if (tab == null) return false;

        String url = tab.getUrl();
        if (DomDistillerUrlUtils.isDistilledPage(url)) {
            url = DomDistillerUrlUtils.getOriginalUrlFromDistillerUrl(url);
        }
        if (TextUtils.isEmpty(url)) url = getUrlToLoad();
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(LaunchIntentDispatcher.EXTRA_IS_ALLOWED_TO_RETURN_TO_PARENT, false);

        boolean willChromeHandleIntent = getIntentDataProvider().isOpenedByChrome();
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        try {
            willChromeHandleIntent |= ExternalNavigationDelegateImpl
                    .willChromeHandleIntent(intent, true);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        Bundle startActivityOptions = ActivityOptionsCompat.makeCustomAnimation(
                this, R.anim.abc_fade_in, R.anim.abc_fade_out).toBundle();
        if (willChromeHandleIntent || forceReparenting) {
            Runnable finalizeCallback = new Runnable() {
                @Override
                public void run() {
                    finishAndClose(true);
                }
            };

            mMainTab = null;
            // mHasCreatedTabEarly == true => mMainTab != null in the rest of the code.
            mHasCreatedTabEarly = false;
            mConnection.resetPostMessageHandlerForSession(mSession, null);
            tab.detachAndStartReparenting(intent, startActivityOptions, finalizeCallback);
        } else {
            // Temporarily allowing disk access while fixing. TODO: http://crbug.com/581860
            StrictMode.allowThreadDiskWrites();
            try {
                if (mIntentDataProvider.isInfoPage()) {
                    IntentHandler.startChromeLauncherActivityForTrustedIntent(intent);
                } else {
                    startActivity(intent, startActivityOptions);
                }
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
        }
        return true;
    }

    /**
     * @return The URL that should be used from this intent. If it is a WebLite url, it may be
     *         overridden if the Data Reduction Proxy is using Lo-Fi previews.
     */
    private String getUrlToLoad() {
        String url = IntentHandler.getUrlFromIntent(getIntent());

        // Intents fired for media viewers have an additional file:// URI passed along so that the
        // tab can display the actual filename to the user when it is loaded.
        if (mIntentDataProvider.isMediaViewer()) {
            String mediaViewerUrl = mIntentDataProvider.getMediaViewerUrl();
            if (!TextUtils.isEmpty(mediaViewerUrl)) {
                Uri mediaViewerUri = Uri.parse(mediaViewerUrl);
                if (UrlConstants.FILE_SCHEME.equals(mediaViewerUri.getScheme())) {
                    url = mediaViewerUrl;
                }
            }
        }

        if (!TextUtils.isEmpty(url)) {
            url = DataReductionProxySettings.getInstance().maybeRewriteWebliteUrl(url);
        }

        return url;
    }

    /** Sets the initial background color for the Tab, shown before the page content is ready. */
    private void prepareTabBackground(final Tab tab) {
        if (!IntentHandler.isIntentChromeOrFirstParty(getIntent())) return;

        int backgroundColor = mIntentDataProvider.getInitialBackgroundColor();
        if (backgroundColor == Color.TRANSPARENT) return;

        // Set the background color.
        tab.getView().setBackgroundColor(backgroundColor);

        // Unset the background when the page has rendered.
        EmptyTabObserver mediaObserver = new EmptyTabObserver() {
            @Override
            public void didFirstVisuallyNonEmptyPaint(final Tab tab) {
                tab.removeObserver(this);

                // Blink has rendered the page by this point, but we need to wait for the compositor
                // frame swap to avoid flash of white content.
                getCompositorViewHolder().getCompositorView().surfaceRedrawNeededAsync(() -> {
                    if (!tab.isInitialized() || isActivityDestroyed()) return;
                    tab.getView().setBackgroundResource(0);
                });
            }
        };

        tab.addObserver(mediaObserver);
    }

    @Override
    protected void initializeToolbar() {
        super.initializeToolbar();
        if (mIntentDataProvider.isMediaViewer()) getToolbarManager().disableShadow();
    }

    /**
     * Show the web page with CustomTabActivity, without any navigation control.
     * Used in showing the terms of services page or help pages for Chrome.
     * @param context The current activity context.
     * @param url The url of the web page.
     */
    public static void showInfoPage(Context context, String url) {
        // TODO(xingliu): The title text will be the html document title, figure out if we want to
        // use Chrome strings here as EmbedContentViewActivity does.
        CustomTabsIntent customTabIntent = new CustomTabsIntent.Builder()
                .setShowTitle(true)
                .setToolbarColor(ApiCompatibilityUtils.getColor(
                        context.getResources(),
                        R.color.dark_action_bar_color))
                .build();
        customTabIntent.intent.setData(Uri.parse(url));

        Intent intent = LaunchIntentDispatcher.createCustomTabActivityIntent(
                context, customTabIntent.intent);
        intent.setPackage(context.getPackageName());
        intent.putExtra(CustomTabIntentDataProvider.EXTRA_UI_TYPE, CUSTOM_TABS_UI_TYPE_INFO_PAGE);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        if (!(context instanceof Activity)) intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        IntentHandler.addTrustedIntentExtras(intent);

        context.startActivity(intent);
    }

    @Override
    protected boolean requiresFirstRunToBeCompleted(Intent intent) {
        // Custom Tabs can be used to open Chrome help pages before the ToS has been accepted.
        if (IntentHandler.isIntentChromeOrFirstParty(intent)
                && IntentUtils.safeGetIntExtra(intent, CustomTabIntentDataProvider.EXTRA_UI_TYPE,
                           CUSTOM_TABS_UI_TYPE_DEFAULT)
                        == CUSTOM_TABS_UI_TYPE_INFO_PAGE) {
            return false;
        }

        return super.requiresFirstRunToBeCompleted(intent);
    }

    @Override
    public boolean canShowTrustedCdnPublisherUrl() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.SHOW_TRUSTED_PUBLISHER_URL)) {
            return false;
        }

        String publisherUrlPackage = mConnection.getTrustedCdnPublisherUrlPackage();
        return publisherUrlPackage != null
                && publisherUrlPackage.equals(mConnection.getClientPackageNameForSession(mSession));
    }
}
