// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.PendingIntent;
import android.content.Intent;
import android.graphics.Point;
import android.net.Uri;
import android.os.Bundle;
import android.support.customtabs.browseractions.BrowserActionItem;
import android.support.customtabs.browseractions.BrowserActionsIntent;
import android.text.TextUtils;
import android.view.Menu;
import android.view.View;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.contextmenu.ContextMenuParams;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.gsa.GSAState;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.rappor.RapporServiceBridge;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.content_public.common.Referrer;
import org.chromium.ui.base.MenuSourceType;

import java.util.ArrayList;
import java.util.List;

/**
 * A transparent {@link AsyncInitializationActivity} that displays the Browser Actions context menu.
 */
public class BrowserActionActivity extends AsyncInitializationActivity {
    private static final String TAG = "cr_BrowserActions";

    private int mType;
    private Uri mUri;
    @VisibleForTesting
    String mUntrustedCreatorPackageName;
    @VisibleForTesting
    List<BrowserActionItem> mActions = new ArrayList<>();
    private PendingIntent mOnBrowserActionSelectedCallback;
    private BrowserActionsContextMenuHelper mHelper;

    @Override
    protected void setContentView() {
        View view = new View(this);
        setContentView(view);
        openContextMenu(view);
    }

    @Override
    protected boolean isStartedUpCorrectly(Intent intent) {
        if (intent == null
                || !BrowserActionsIntent.ACTION_BROWSER_ACTIONS_OPEN.equals(intent.getAction())) {
            return false;
        }
        mUri = Uri.parse(IntentHandler.getUrlFromIntent(intent));
        mType = IntentUtils.safeGetIntExtra(
                intent, BrowserActionsIntent.EXTRA_TYPE, BrowserActionsIntent.URL_TYPE_NONE);
        mUntrustedCreatorPackageName = BrowserActionsIntent.getUntrustedCreatorPackageName(intent);
        mOnBrowserActionSelectedCallback = IntentUtils.safeGetParcelableExtra(
                intent, BrowserActionsIntent.EXTRA_SELECTED_ACTION_PENDING_INTENT);
        ArrayList<Bundle> bundles = IntentUtils.getParcelableArrayListExtra(
                intent, BrowserActionsIntent.EXTRA_MENU_ITEMS);
        if (bundles != null) {
            mActions = BrowserActionsIntent.parseBrowserActionItems(bundles);
        }
        if (mUri == null) {
            Log.e(TAG, "Missing url");
            return false;
        } else if (!UrlConstants.HTTP_SCHEME.equals(mUri.getScheme())
                && !UrlConstants.HTTPS_SCHEME.equals(mUri.getScheme())) {
            Log.e(TAG, "Url should only be HTTP or HTTPS scheme");
            return false;
        } else if (mUntrustedCreatorPackageName == null) {
            Log.e(TAG, "Missing creator's pacakge name");
            return false;
        } else if (!TextUtils.equals(mUntrustedCreatorPackageName, getPackageName())
                && (intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0) {
            Log.e(TAG, "Intent should not be started with FLAG_ACTIVITY_NEW_TASK");
            return false;
        } else if ((intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_DOCUMENT) != 0) {
            Log.e(TAG, "Intent should not be started with FLAG_ACTIVITY_NEW_DOCUMENT");
            return false;
        } else {
            return true;
        }
    }

    /**
     * Opens a Browser Actions context menu based on the parsed data.
     */
    @Override
    public void openContextMenu(View view) {
        ContextMenuParams params = createContextMenuParams();
        Runnable listener = new Runnable() {
            @Override
            public void run() {
                if (FirstRunStatus.getFirstRunFlowComplete()) {
                    startDelayedNativeInitialization();
                }
            }
        };
        mHelper = new BrowserActionsContextMenuHelper(this, params, mActions,
                mUntrustedCreatorPackageName, mOnBrowserActionSelectedCallback, listener);
        mHelper.displayBrowserActionsMenu(view);
        return;
    }

    @Override
    @VisibleForTesting
    protected boolean isStartupDelayed() {
        return super.isStartupDelayed();
    }

    /**
     * @return The {@link BrowserActionsContextMenuHelper} for testing.
     */
    @VisibleForTesting
    BrowserActionsContextMenuHelper getHelperForTesting() {
        return mHelper;
    }

    /**
     * Creates a {@link ContextMenuParams} with given Uri and type.
     * @return The ContextMenuParams used to construct context menu.
     */
    private ContextMenuParams createContextMenuParams() {
        Referrer referrer =
                IntentHandler.constructValidReferrerForAuthority(mUntrustedCreatorPackageName);

        Point displaySize = new Point();
        getWindowManager().getDefaultDisplay().getSize(displaySize);
        float density = getResources().getDisplayMetrics().density;
        int touchX = (int) ((displaySize.x / 2f) / density);
        int touchY = (int) ((displaySize.y / 2f) / density);

        return new ContextMenuParams(mType, mUri.toString(), mUri.toString(), mUri.toString(),
                mUri.toString(), mUri.toString(), mUri.toString(), false /* imageWasFetchedLoFi */,
                referrer, false /* canSaveMedia */, touchX, touchY,
                MenuSourceType.MENU_SOURCE_TOUCH);
    }

    @Override
    protected boolean shouldDelayBrowserStartup() {
        return true;
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void onContextMenuClosed(Menu menu) {
        super.onContextMenuClosed(menu);
        if (mHelper != null) {
            mHelper.onContextMenuClosed();
        }
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        recordClientPackageName();
        mHelper.onNativeInitialized();
    }

    private void recordClientPackageName() {
        if (TextUtils.isEmpty(mUntrustedCreatorPackageName)) return;
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                RapporServiceBridge.sampleString(
                        "BrowserActions.ServiceClient.PackageName", mUntrustedCreatorPackageName);
                if (GSAState.isGsaPackageName(mUntrustedCreatorPackageName)) return;
                RapporServiceBridge.sampleString(
                        "BrowserActions.ServiceClient.PackageNameThirdParty",
                        mUntrustedCreatorPackageName);
            }
        });
    }

    @Override
    protected boolean requiresFirstRunToBeCompleted(Intent intent) {
        return false;
    }
}
