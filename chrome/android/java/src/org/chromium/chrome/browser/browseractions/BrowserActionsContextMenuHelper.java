// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.support.annotation.IntDef;
import android.support.customtabs.browseractions.BrowserActionItem;
import android.support.customtabs.browseractions.BrowserActionsIntent;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v7.content.res.AppCompatResources;
import android.util.Pair;
import android.util.SparseArray;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.View.OnCreateContextMenuListener;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.CachedMetrics.EnumeratedHistogramSample;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuItem;
import org.chromium.chrome.browser.contextmenu.ContextMenuItem;
import org.chromium.chrome.browser.contextmenu.ContextMenuParams;
import org.chromium.chrome.browser.contextmenu.ContextMenuUi;
import org.chromium.chrome.browser.contextmenu.PlatformContextMenuUi;
import org.chromium.chrome.browser.contextmenu.ShareContextMenuItem;
import org.chromium.chrome.browser.contextmenu.TabularContextMenuUi;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.ui.base.WindowAndroid.OnCloseContextMenuListener;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A helper class that handles generating context menus for Browser Actions.
 */
public class BrowserActionsContextMenuHelper implements OnCreateContextMenuListener,
                                                        OnCloseContextMenuListener,
                                                        OnAttachStateChangeListener {
    /** Notified about events happening for Browser Actions tests. */
    public interface BrowserActionsTestDelegate {
        /** Called when menu is shown. */
        void onBrowserActionsMenuShown();

        /** Called when {@link BrowserActionActivity#finishNativeInitialization} is done. */
        void onFinishNativeInitialization();

        /** Called when Browser Actions start opening a tab in background */
        void onOpenTabInBackgroundStart();

        /** Called when Browser Actions start downloading a url */
        void onDownloadStart();

        /** Initializes data needed for testing. */
        void initialize(SparseArray<PendingIntent> customActions,
                List<Pair<Integer, List<ContextMenuItem>>> items,
                ProgressDialog progressDialog);
    }

    /**
     * Defines the actions shown on the Browser Actions context menu.
     * Note: these values must match the BrowserActionsMenuOption enum in enums.xml.
     * And the values are persisted to logs so they cannot be renumbered or reused.
     */
    @IntDef({ACTION_OPEN_IN_NEW_CHROME_TAB, ACTION_OPEN_IN_INCOGNITO_TAB, ACTION_DOWNLOAD_PAGE,
            ACTION_COPY_LINK, ACTION_SHARE, ACTION_APP_PROVIDED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface BrowserActionsActionId {}
    private static final int ACTION_OPEN_IN_NEW_CHROME_TAB = 0;
    private static final int ACTION_OPEN_IN_INCOGNITO_TAB = 1;
    private static final int ACTION_DOWNLOAD_PAGE = 2;
    private static final int ACTION_COPY_LINK = 3;
    private static final int ACTION_SHARE = 4;
    // Actions for selecting custom items.
    private static final int ACTION_APP_PROVIDED = 5;
    private static final int NUM_ACTIONS = 6;

    static final List<Integer> CUSTOM_BROWSER_ACTIONS_ID_GROUP =
            Arrays.asList(R.id.browser_actions_custom_item_one,
                    R.id.browser_actions_custom_item_two, R.id.browser_actions_custom_item_three,
                    R.id.browser_actions_custom_item_four, R.id.browser_actions_custom_item_five);

    private static final String TAG = "cr_BrowserActions";
    private static final boolean IS_NEW_UI_ENABLED = true;

    // Items list that could be included in the Browser Actions context menu for type {@code LINK}.
    private final List<? extends ContextMenuItem> mBrowserActionsLinkGroup;

    // Map each custom item's id with its PendingIntent action.
    private final SparseArray<PendingIntent> mCustomItemActionMap = new SparseArray<>();

    private final ContextMenuParams mCurrentContextMenuParams;
    private final BrowserActionsContextMenuItemDelegate mMenuItemDelegate;
    private final Activity mActivity;
    private final Callback<Integer> mItemSelectedCallback;
    private final Runnable mOnMenuShown;
    private final Runnable mOnMenuClosed;
    private final Runnable mOnMenuShownListener;
    private final Callback<Boolean> mOnShareClickedRunnable;
    private final PendingIntent mOnBrowserActionSelectedCallback;
    private final EnumeratedHistogramSample mActionHistogram;

    private final List<Pair<Integer, List<ContextMenuItem>>> mItems;

    private final ProgressDialog mProgressDialog;

    private BrowserActionsTestDelegate mTestDelegate;
    private int mPendingItemId;
    private boolean mIsNativeInitialized;
    private boolean mShouldFinishActivity;

    public BrowserActionsContextMenuHelper(Activity activity, ContextMenuParams params,
            List<BrowserActionItem> customItems, String sourcePackageName,
            PendingIntent onBrowserActionSelectedCallback, final Runnable listener) {
        mActivity = activity;
        mCurrentContextMenuParams = params;
        mOnMenuShownListener = listener;
        mShouldFinishActivity = true;

        mOnMenuShown = new Runnable() {
            @Override
            public void run() {
                mOnMenuShownListener.run();
                if (mTestDelegate != null) {
                    mTestDelegate.onBrowserActionsMenuShown();
                }
            }
        };
        mOnMenuClosed = new Runnable() {
            @Override
            public void run() {
                if (mPendingItemId == 0 && mShouldFinishActivity) {
                    mActivity.finish();
                }
            }
        };
        mItemSelectedCallback = new Callback<Integer>() {
            @Override
            public void onResult(Integer result) {
                onItemSelected(result, true);
            }
        };
        mOnShareClickedRunnable = new Callback<Boolean>() {
            @Override
            public void onResult(Boolean isShareLink) {
                mMenuItemDelegate.share(true, mCurrentContextMenuParams.getLinkUrl(), false);
            }
        };
        ShareContextMenuItem shareItem = new ShareContextMenuItem(R.drawable.ic_share_white_24dp,
                R.string.browser_actions_share, R.id.browser_actions_share, true);
        shareItem.setCreatorPackageName(sourcePackageName);
        if (FirstRunStatus.getFirstRunFlowComplete()) {
            mBrowserActionsLinkGroup =
                    Arrays.asList(ChromeContextMenuItem.BROWSER_ACTIONS_OPEN_IN_BACKGROUND,
                            ChromeContextMenuItem.BROWSER_ACTIONS_OPEN_IN_INCOGNITO_TAB,
                            ChromeContextMenuItem.BROWSER_ACTION_SAVE_LINK_AS,
                            ChromeContextMenuItem.BROWSER_ACTIONS_COPY_ADDRESS, shareItem);
        } else {
            mBrowserActionsLinkGroup =
                    Arrays.asList(ChromeContextMenuItem.BROWSER_ACTIONS_COPY_ADDRESS, shareItem);
        }
        mMenuItemDelegate = new BrowserActionsContextMenuItemDelegate(mActivity, sourcePackageName);
        mOnBrowserActionSelectedCallback = onBrowserActionSelectedCallback;
        mProgressDialog = new ProgressDialog(mActivity);
        mActionHistogram =
                new EnumeratedHistogramSample("BrowserActions.SelectedOption", NUM_ACTIONS);

        mItems = buildContextMenuItems(customItems, sourcePackageName);
    }

    /**
     * Sets the {@link BrowserActionsTestDelegate} for testing.
     * @param testDelegate The delegate used to notified Browser Actions events.
     */
    @VisibleForTesting
    void setTestDelegateForTesting(BrowserActionsTestDelegate testDelegate) {
        mTestDelegate = testDelegate;
        mTestDelegate.initialize(mCustomItemActionMap, mItems, mProgressDialog);
    }

    /**
     * Builds items for Browser Actions context menu.
     */
    private List<Pair<Integer, List<ContextMenuItem>>> buildContextMenuItems(
            List<BrowserActionItem> customItems, String sourcePackageName) {
        List<Pair<Integer, List<ContextMenuItem>>> menuItems = new ArrayList<>();
        List<ContextMenuItem> items = new ArrayList<>();
        items.addAll(mBrowserActionsLinkGroup);
        addBrowserActionItems(items, customItems, sourcePackageName);

        menuItems.add(new Pair<>(R.string.contextmenu_link_title, items));
        return menuItems;
    }

    /**
     * Adds custom items to the context menu list and populates custom item action map.
     * @param items List of {@link ContextMenuItem} to display the context menu.
     * @param customItems List of {@link BrowserActionItem} for custom items.
     * @param sourcePackageName The package name of the requested app.
     */
    private void addBrowserActionItems(List<ContextMenuItem> items,
            List<BrowserActionItem> customItems, String sourcePackageName) {
        PackageManager pm = ContextUtils.getApplicationContext().getPackageManager();
        Resources resources = null;
        try {
            resources = pm.getResourcesForApplication(sourcePackageName);
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Fail to find the resources", e);
        }
        for (int i = 0; i < customItems.size() && i < BrowserActionsIntent.MAX_CUSTOM_ITEMS; i++) {
            Drawable drawable = null;
            if (resources != null && customItems.get(i).getIconId() != 0) {
                try {
                    drawable = ResourcesCompat.getDrawable(
                            resources, customItems.get(i).getIconId(), null);
                } catch (NotFoundException e1) {
                    try {
                        Context context = mActivity.createPackageContext(sourcePackageName,
                                Context.CONTEXT_IGNORE_SECURITY | Context.CONTEXT_INCLUDE_CODE);
                        drawable = AppCompatResources.getDrawable(
                                context, customItems.get(i).getIconId());
                    } catch (NameNotFoundException e2) {
                        Log.e(TAG, "Cannot find the package name %s", sourcePackageName, e2);
                    } catch (NotFoundException e3) {
                        Log.e(TAG, "Cannot get Drawable for %s", customItems.get(i).getTitle(), e3);
                    }
                }
            }
            items.add(new BrowserActionsCustomContextMenuItem(
                    CUSTOM_BROWSER_ACTIONS_ID_GROUP.get(i), customItems.get(i).getTitle(), drawable,
                    customItems.get(i).getIconUri()));
            mCustomItemActionMap.put(
                    CUSTOM_BROWSER_ACTIONS_ID_GROUP.get(i), customItems.get(i).getAction());
        }
    }

    boolean onItemSelected(int itemId, boolean recordMetrics) {
        if (itemId == R.id.browser_actions_open_in_background) {
            if (mIsNativeInitialized) {
                handleOpenInBackground();
            } else {
                mPendingItemId = itemId;
                waitNativeInitialized();
            }
        } else if (itemId == R.id.browser_actions_open_in_incognito_tab) {
            mMenuItemDelegate.onOpenInIncognitoTab(mCurrentContextMenuParams.getLinkUrl());
            notifyBrowserActionSelected(BrowserActionsIntent.ITEM_OPEN_IN_INCOGNITO);
        } else if (itemId == R.id.browser_actions_save_link_as) {
            if (mIsNativeInitialized) {
                handleDownload();
            } else {
                mPendingItemId = itemId;
                waitNativeInitialized();
            }
        } else if (itemId == R.id.browser_actions_copy_address) {
            mMenuItemDelegate.onSaveToClipboard(mCurrentContextMenuParams.getLinkUrl());
            notifyBrowserActionSelected(BrowserActionsIntent.ITEM_COPY);
        } else if (itemId == R.id.browser_actions_share) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP_MR1) {
                mShouldFinishActivity = false;
            }
            mMenuItemDelegate.share(
                    false, mCurrentContextMenuParams.getLinkUrl(), !mShouldFinishActivity);
            notifyBrowserActionSelected(BrowserActionsIntent.ITEM_SHARE);
        } else if (mCustomItemActionMap.indexOfKey(itemId) >= 0) {
            mMenuItemDelegate.onCustomItemSelected(mCustomItemActionMap.get(itemId));
        }
        if (recordMetrics) recordBrowserActionsSelection(itemId);
        return true;
    }

    private void recordBrowserActionsSelection(int itemId) {
        @BrowserActionsActionId
        final int actionId;
        if (itemId == R.id.browser_actions_open_in_background) {
            actionId = ACTION_OPEN_IN_NEW_CHROME_TAB;
        } else if (itemId == R.id.browser_actions_open_in_incognito_tab) {
            actionId = ACTION_OPEN_IN_INCOGNITO_TAB;
        } else if (itemId == R.id.browser_actions_save_link_as) {
            actionId = ACTION_DOWNLOAD_PAGE;
        } else if (itemId == R.id.browser_actions_copy_address) {
            actionId = ACTION_COPY_LINK;
        } else if (itemId == R.id.browser_actions_share) {
            actionId = ACTION_SHARE;
        } else {
            actionId = ACTION_APP_PROVIDED;
        }
        mActionHistogram.record(actionId);
    }

    private void notifyBrowserActionSelected(int menuId) {
        if (mOnBrowserActionSelectedCallback == null) return;
        Intent additionalData = new Intent();
        additionalData.setData(Uri.parse(String.valueOf(menuId)));
        try {
            mOnBrowserActionSelectedCallback.send(mActivity, 0, additionalData, null, null);
        } catch (CanceledException e) {
            Log.e(TAG, "Browser Actions failed to send default items' pending intent.");
        }
    }

    /**
     * Display a progress dialog to wait for native libraries initialized.
     */
    private void waitNativeInitialized() {
        mProgressDialog.setMessage(
                mActivity.getString(R.string.browser_actions_loading_native_message));
        mProgressDialog.show();
    }

    private void dismissProgressDialog() {
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
        }
    }

    /**
     * Displays the Browser Actions context menu.
     * @param view The view to show the context menu if old UI is used.
     */
    public void displayBrowserActionsMenu(final View view) {
        if (IS_NEW_UI_ENABLED) {
            ContextMenuUi menuUi = new TabularContextMenuUi(mOnShareClickedRunnable);
            menuUi.displayMenu(mActivity, mCurrentContextMenuParams, mItems, mItemSelectedCallback,
                    mOnMenuShown, mOnMenuClosed);
        } else {
            view.setOnCreateContextMenuListener(BrowserActionsContextMenuHelper.this);
            assert view.getWindowToken() == null;
            view.addOnAttachStateChangeListener(this);
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        ContextMenuUi menuUi = new PlatformContextMenuUi(menu);
        menuUi.displayMenu(mActivity, mCurrentContextMenuParams, mItems, mItemSelectedCallback,
                mOnMenuShown, mOnMenuClosed);
    }

    @Override
    public void onContextMenuClosed() {
        mOnMenuClosed.run();
    }

    @Override
    public void onViewAttachedToWindow(View view) {
        if (view.showContextMenu()) {
            mOnMenuShown.run();
        }
    }

    @Override
    public void onViewDetachedFromWindow(View v) {}

    /**
     * Finishes all pending actions which requires Chrome native libraries.
     */
    public void onNativeInitialized() {
        mIsNativeInitialized = true;
        RecordUserAction.record("BrowserActions.MenuOpened");
        if (mTestDelegate != null) {
            mTestDelegate.onFinishNativeInitialization();
        }
        if (mPendingItemId != 0) {
            dismissProgressDialog();
            onItemSelected(mPendingItemId, false);
            mPendingItemId = 0;
            if (mShouldFinishActivity) mActivity.finish();
        }
    }

    private void handleOpenInBackground() {
        mMenuItemDelegate.onOpenInBackground(mCurrentContextMenuParams.getLinkUrl());
        if (mTestDelegate != null) {
            mTestDelegate.onOpenTabInBackgroundStart();
        }
        notifyBrowserActionSelected(BrowserActionsIntent.ITEM_OPEN_IN_NEW_TAB);
    }

    private void handleDownload() {
        mMenuItemDelegate.startDownload(mCurrentContextMenuParams.getLinkUrl());
        if (mTestDelegate != null) {
            mTestDelegate.onDownloadStart();
        }
        notifyBrowserActionSelected(BrowserActionsIntent.ITEM_DOWNLOAD);
    }
}
