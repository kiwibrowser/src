// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;

import org.chromium.base.Log;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.OfflinePageOrigin;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.share.ShareParams;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.util.IntentUtils;

/**
 * A delegate responsible for taking actions based on browser action context menu selections.
 */
public class BrowserActionsContextMenuItemDelegate {
    private static final String TAG = "BrowserActionsItem";

    private final Activity mActivity;
    private final String mSourcePackageName;

    /**
     * Builds a {@link BrowserActionsContextMenuItemDelegate} instance.
     * @param activity The activity displays the context menu.
     * @param sourcePackageName The package name of the app which requests the Browser Actions.
     */
    public BrowserActionsContextMenuItemDelegate(Activity activity, String sourcePackageName) {
        mActivity = activity;
        mSourcePackageName = sourcePackageName;
    }

    /**
     * Called when the {@code text} should be saved to the clipboard.
     * @param text The text to save to the clipboard.
     */
    public void onSaveToClipboard(String text) {
        ClipboardManager clipboardManager =
                (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData data = ClipData.newPlainText("url", text);
        clipboardManager.setPrimaryClip(data);
    }

    /**
     * Called when the {@code linkUrl} should be opened in Chrome incognito tab.
     * @param linkUrl The url to open.
     */
    public void onOpenInIncognitoTab(String linkUrl) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(linkUrl));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setPackage(mActivity.getPackageName());
        intent.putExtra(LaunchIntentDispatcher.EXTRA_IS_ALLOWED_TO_RETURN_TO_PARENT, false);
        intent.putExtra(IntentHandler.EXTRA_OPEN_NEW_INCOGNITO_TAB, true);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, mActivity.getPackageName());
        IntentHandler.addTrustedIntentExtras(intent);
        IntentHandler.setTabLaunchType(intent, TabLaunchType.FROM_EXTERNAL_APP);
        IntentUtils.safeStartActivity(mActivity, intent);
    }

    /**
     * Called when the {@code linkUrl} should be opened in Chrome in the background.
     * @param linkUrl The url to open.
     */
    public void onOpenInBackground(String linkUrl) {
        BrowserActionsService.openLinkInBackground(linkUrl, mSourcePackageName);
    }

    /**
     * Called when a custom item of Browser action menu is selected.
     * @param action The PendingIntent action to be launched.
     */
    public void onCustomItemSelected(PendingIntent action) {
        try {
            action.send();
        } catch (CanceledException e) {
            Log.e(TAG, "Browser Action in Chrome failed to send pending intent.");
        }
    }

    /**
     * Called when the page of the {@code linkUrl} should be downloaded.
     * @param linkUrl The url of the page to download.
     */
    public void startDownload(String linkUrl) {
        OfflinePageBridge offlinePageBridge =
                OfflinePageBridge.getForProfile(Profile.getLastUsedProfile().getOriginalProfile());
        OfflinePageOrigin origin = new OfflinePageOrigin(mActivity, mSourcePackageName);
        // TODO(ltian): Support single file download here. crbug.com/754807.
        offlinePageBridge.savePageLater(
                linkUrl, OfflinePageBridge.BROWSER_ACTIONS_NAMESPACE, true, origin);
    }

    /**
     * Called when the {@code linkUrl} should be shared.
     * @param shareDirectly Whether to share directly with the previous app shared with.
     * @param linkUrl The url to share.
     * @param shouldCloseActivity Whether to close activity after sharing.
     */
    public void share(Boolean shareDirectly, String linkUrl, boolean shouldCloseActivity) {
        Runnable onShareDialogDismissed = shouldCloseActivity ? mActivity::finish : null;
        ShareParams params = new ShareParams.Builder(mActivity, linkUrl, linkUrl)
                                     .setShareDirectly(shareDirectly)
                                     .setSaveLastUsed(!shareDirectly)
                                     .setSourcePackageName(mSourcePackageName)
                                     .setIsExternalUrl(true)
                                     .setOnDialogDismissed(onShareDialogDismissed)
                                     .build();
        ShareHelper.share(params);
    }
}
