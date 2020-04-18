// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.app.Activity;
import android.content.ComponentName;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Build;
import android.util.Pair;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.share.ShareParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.MenuSourceType;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.WindowAndroid.OnCloseContextMenuListener;

import java.util.List;

import javax.annotation.Nullable;

/**
 * A helper class that handles generating context menus for {@link WebContents}s.
 */
public class ContextMenuHelper implements OnCreateContextMenuListener {
    private static final int MAX_SHARE_DIMEN_PX = 2048;

    private final WebContents mWebContents;
    private long mNativeContextMenuHelper;

    private ContextMenuPopulator mPopulator;
    private ContextMenuParams mCurrentContextMenuParams;
    private Activity mActivity;
    private Callback<Integer> mCallback;
    private Runnable mOnMenuShown;
    private Runnable mOnMenuClosed;

    private ContextMenuHelper(long nativeContextMenuHelper, WebContents webContents) {
        mNativeContextMenuHelper = nativeContextMenuHelper;
        mWebContents = webContents;
    }

    @CalledByNative
    private static ContextMenuHelper create(long nativeContextMenuHelper, WebContents webContents) {
        return new ContextMenuHelper(nativeContextMenuHelper, webContents);
    }

    @CalledByNative
    private void destroy() {
        if (mPopulator != null) mPopulator.onDestroy();
        mNativeContextMenuHelper = 0;
    }

    /**
     * @return The activity that corresponds to the context menu helper.
     */
    protected Activity getActivity() {
        return mActivity;
    }

    /**
     * @param populator A {@link ContextMenuPopulator} that is responsible for managing and showing
     *                  context menus.
     */
    @CalledByNative
    private void setPopulator(ContextMenuPopulator populator) {
        if (mPopulator != null) mPopulator.onDestroy();
        mPopulator = populator;
    }

    /**
     * Starts showing a context menu for {@code view} based on {@code params}.
     * @param params The {@link ContextMenuParams} that indicate what menu items to show.
     * @param view container view for the menu.
     * @param topContentOffsetPx the offset of the content from the top.
     */
    @CalledByNative
    private void showContextMenu(
            final ContextMenuParams params, View view, float topContentOffsetPx) {
        if (params.isFile()) return;
        final WindowAndroid windowAndroid = mWebContents.getTopLevelNativeWindow();

        if (view == null || view.getVisibility() != View.VISIBLE || view.getParent() == null
                || windowAndroid == null || windowAndroid.getActivity().get() == null
                || mPopulator == null) {
            return;
        }

        mCurrentContextMenuParams = params;
        mActivity = windowAndroid.getActivity().get();
        mCallback = new Callback<Integer>() {
            @Override
            public void onResult(Integer result) {
                mPopulator.onItemSelected(
                        ContextMenuHelper.this, mCurrentContextMenuParams, result);
            }
        };
        mOnMenuShown = new Runnable() {
            @Override
            public void run() {
                RecordHistogram.recordBooleanHistogram("ContextMenu.Shown", mWebContents != null);
            }
        };
        mOnMenuClosed = new Runnable() {
            @Override
            public void run() {
                if (mNativeContextMenuHelper == 0) return;
                nativeOnContextMenuClosed(mNativeContextMenuHelper);
            }
        };

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.CUSTOM_CONTEXT_MENU)
                && params.getSourceType() != MenuSourceType.MENU_SOURCE_MOUSE) {
            List<Pair<Integer, List<ContextMenuItem>>> items =
                    mPopulator.buildContextMenu(null, mActivity, mCurrentContextMenuParams);
            if (items.isEmpty()) {
                ThreadUtils.postOnUiThread(mOnMenuClosed);
                return;
            }

            final TabularContextMenuUi menuUi = new TabularContextMenuUi(new Callback<Boolean>() {
                @Override
                public void onResult(Boolean isShareLink) {
                    if (isShareLink) {
                        ShareParams shareParams =
                                new ShareParams.Builder(mActivity, params.getUrl(), params.getUrl())
                                        .setShareDirectly(true)
                                        .setSaveLastUsed(false)
                                        .build();
                        ShareHelper.share(shareParams);
                    } else {
                        shareImageDirectly(ShareHelper.getLastShareComponentName(null));
                    }
                }
            });
            menuUi.setTopContentOffsetY(topContentOffsetPx);
            menuUi.displayMenu(mActivity, mCurrentContextMenuParams, items, mCallback, mOnMenuShown,
                    mOnMenuClosed);
            if (mCurrentContextMenuParams.isImage()) {
                getThumbnail(menuUi, new Callback<Bitmap>() {
                    @Override
                    public void onResult(Bitmap result) {
                        menuUi.onImageThumbnailRetrieved(result);
                    }
                });
            }
            return;
        }

        // The Platform Context Menu requires the listener within this helper since this helper and
        // provides context menu for us to show.
        view.setOnCreateContextMenuListener(this);
        boolean wasContextMenuShown = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
                && params.getSourceType() == MenuSourceType.MENU_SOURCE_MOUSE) {
            final float density = view.getResources().getDisplayMetrics().density;
            final float touchPointXPx = params.getTriggeringTouchXDp() * density;
            final float touchPointYPx =
                    (params.getTriggeringTouchYDp() * density) + topContentOffsetPx;
            wasContextMenuShown = view.showContextMenu(touchPointXPx, touchPointYPx);
        } else {
            wasContextMenuShown = view.showContextMenu();
        }
        if (wasContextMenuShown) {
            mOnMenuShown.run();
            windowAndroid.addContextMenuCloseListener(new OnCloseContextMenuListener() {
                @Override
                public void onContextMenuClosed() {
                    mOnMenuClosed.run();
                    windowAndroid.removeContextMenuCloseListener(this);
                }
            });
        }
    }

    /**
     * Starts a download based on the current {@link ContextMenuParams}.
     * @param isLink Whether or not the download target is a link.
     */
    public void startContextMenuDownload(boolean isLink, boolean isDataReductionProxyEnabled) {
        if (mNativeContextMenuHelper != 0) {
            nativeOnStartDownload(mNativeContextMenuHelper, isLink, isDataReductionProxyEnabled);
        }
    }

    /**
     * Trigger an image search for the current image that triggered the context menu.
     */
    public void searchForImage() {
        if (mNativeContextMenuHelper == 0) return;
        nativeSearchForImage(mNativeContextMenuHelper);
    }

    /**
     * Share the image that triggered the current context menu.
     */
    public void shareImage() {
        shareImageDirectly(null);
    }

    /**
     * Share image triggered with the current context menu directly with a specific app.
     * @param name The {@link ComponentName} of the app to share the image directly with.
     */
    public void shareImageDirectly(@Nullable final ComponentName name) {
        if (mNativeContextMenuHelper == 0) return;
        Callback<byte[]> callback = new Callback<byte[]>() {
            @Override
            public void onResult(byte[] result) {
                WindowAndroid windowAndroid = mWebContents.getTopLevelNativeWindow();

                Activity activity = windowAndroid.getActivity().get();
                if (activity == null) return;

                ShareHelper.shareImage(activity, result, name);
            }
        };
        nativeRetrieveImageForShare(
                mNativeContextMenuHelper, callback, MAX_SHARE_DIMEN_PX, MAX_SHARE_DIMEN_PX);
    }

    /**
     * Gets the thumbnail of the current image that triggered the context menu.
     * @param callback Called once the the thumbnail is received.
     */
    private void getThumbnail(TabularContextMenuUi menuUi, final Callback<Bitmap> callback) {
        if (mNativeContextMenuHelper == 0) return;

        Resources res = mActivity.getResources();

        int maxWidthPx = menuUi.getMaxThumbnailWidthPx(res);
        int maxHeightPx = menuUi.getMaxThumbnailHeightPx(res);

        Callback<Bitmap> bitmapCallback = new Callback<Bitmap>() {
            @Override
            public void onResult(Bitmap result) {
                callback.onResult(result);
            }

        };
        nativeRetrieveImageForContextMenu(
                mNativeContextMenuHelper, bitmapCallback, maxWidthPx, maxHeightPx);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        List<Pair<Integer, List<ContextMenuItem>>> items =
                mPopulator.buildContextMenu(menu, v.getContext(), mCurrentContextMenuParams);

        if (items.isEmpty()) {
            ThreadUtils.postOnUiThread(mOnMenuClosed);
            return;
        }
        ContextMenuUi menuUi = new PlatformContextMenuUi(menu);
        menuUi.displayMenu(mActivity, mCurrentContextMenuParams, items, mCallback, mOnMenuShown,
                mOnMenuClosed);
    }

    /**
     * @return The {@link ContextMenuPopulator} responsible for populating the context menu.
     */
    @VisibleForTesting
    public ContextMenuPopulator getPopulator() {
        return mPopulator;
    }

    private native void nativeOnStartDownload(
            long nativeContextMenuHelper, boolean isLink, boolean isDataReductionProxyEnabled);
    private native void nativeSearchForImage(long nativeContextMenuHelper);
    private native void nativeRetrieveImageForShare(long nativeContextMenuHelper,
            Callback<byte[]> callback, int maxWidthPx, int maxHeightPx);
    private native void nativeRetrieveImageForContextMenu(long nativeContextMenuHelper,
            Callback<Bitmap> callback, int maxWidthPx, int maxHeightPx);
    private native void nativeOnContextMenuClosed(long nativeContextMenuHelper);
}
