// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.framehost;

import android.graphics.Bitmap;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationEntry;
import org.chromium.content_public.browser.NavigationHistory;
import org.chromium.content_public.common.ResourceRequestBody;

/**
 * The NavigationControllerImpl Java wrapper to allow communicating with the native
 * NavigationControllerImpl object.
 */
@JNINamespace("content")
// TODO(tedchoc): Remove the package restriction once this class moves to a non-public content
//                package whose visibility will be enforced via DEPS.
/* package */ class NavigationControllerImpl implements NavigationController {

    private long mNativeNavigationControllerAndroid;

    private NavigationControllerImpl(long nativeNavigationControllerAndroid) {
        mNativeNavigationControllerAndroid = nativeNavigationControllerAndroid;
    }

    @CalledByNative
    private static NavigationControllerImpl create(long nativeNavigationControllerAndroid) {
        return new NavigationControllerImpl(nativeNavigationControllerAndroid);
    }

    @CalledByNative
    private void destroy() {
        mNativeNavigationControllerAndroid = 0;
    }

    @Override
    public boolean canGoBack() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoBack(mNativeNavigationControllerAndroid);
    }

    @Override
    public boolean canGoForward() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoForward(mNativeNavigationControllerAndroid);
    }

    @Override
    @VisibleForTesting
    public boolean canGoToOffset(int offset) {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoToOffset(mNativeNavigationControllerAndroid, offset);
    }

    @Override
    public void goToOffset(int offset) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoToOffset(mNativeNavigationControllerAndroid, offset);
        }
    }

    @Override
    public void goToNavigationIndex(int index) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoToNavigationIndex(mNativeNavigationControllerAndroid, index);
        }
    }

    @Override
    public void goBack() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoBack(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void goForward() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoForward(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public boolean isInitialNavigation() {
        return mNativeNavigationControllerAndroid != 0
                && nativeIsInitialNavigation(mNativeNavigationControllerAndroid);
    }

    @Override
    public void loadIfNecessary() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeLoadIfNecessary(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public boolean needsReload() {
        return mNativeNavigationControllerAndroid != 0
                && nativeNeedsReload(mNativeNavigationControllerAndroid);
    }

    @Override
    public void setNeedsReload() {
        nativeSetNeedsReload(mNativeNavigationControllerAndroid);
    }

    @Override
    public void reload(boolean checkForRepost) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeReload(mNativeNavigationControllerAndroid, checkForRepost);
        }
    }

    @Override
    public void reloadBypassingCache(boolean checkForRepost) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeReloadBypassingCache(mNativeNavigationControllerAndroid, checkForRepost);
        }
    }

    @Override
    public void cancelPendingReload() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeCancelPendingReload(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void continuePendingReload() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeContinuePendingReload(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void loadUrl(LoadUrlParams params) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeLoadUrl(mNativeNavigationControllerAndroid, params.getUrl(),
                    params.getLoadUrlType(), params.getTransitionType(),
                    params.getReferrer() != null ? params.getReferrer().getUrl() : null,
                    params.getReferrer() != null ? params.getReferrer().getPolicy() : 0,
                    params.getUserAgentOverrideOption(), params.getExtraHeadersString(),
                    params.getPostData(), params.getBaseUrl(), params.getVirtualUrlForDataUrl(),
                    params.getDataUrlAsString(), params.getCanLoadLocalResources(),
                    params.getIsRendererInitiated(), params.getShouldReplaceCurrentEntry());
        }
    }

    @Override
    @VisibleForTesting
    public void clearHistory() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeClearHistory(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public NavigationHistory getNavigationHistory() {
        if (mNativeNavigationControllerAndroid == 0) return null;
        NavigationHistory history = new NavigationHistory();
        int currentIndex = nativeGetNavigationHistory(
                   mNativeNavigationControllerAndroid, history);
        history.setCurrentEntryIndex(currentIndex);
        return history;
    }

    @Override
    public NavigationHistory getDirectedNavigationHistory(boolean isForward, int itemLimit) {
        if (mNativeNavigationControllerAndroid == 0) return null;
        NavigationHistory history = new NavigationHistory();
        nativeGetDirectedNavigationHistory(mNativeNavigationControllerAndroid,
                   history, isForward, itemLimit);
        return history;
    }

    @Override
    public String getOriginalUrlForVisibleNavigationEntry() {
        if (mNativeNavigationControllerAndroid == 0) return null;
        return nativeGetOriginalUrlForVisibleNavigationEntry(mNativeNavigationControllerAndroid);
    }

    @Override
    public void clearSslPreferences() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeClearSslPreferences(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public boolean getUseDesktopUserAgent() {
        if (mNativeNavigationControllerAndroid == 0) return false;
        return nativeGetUseDesktopUserAgent(mNativeNavigationControllerAndroid);
    }

    @Override
    public void setUseDesktopUserAgent(boolean override, boolean reloadOnChange) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeSetUseDesktopUserAgent(mNativeNavigationControllerAndroid,
                     override, reloadOnChange);
        }
    }

    @Override
    public NavigationEntry getEntryAtIndex(int index) {
        if (mNativeNavigationControllerAndroid != 0) {
            return nativeGetEntryAtIndex(mNativeNavigationControllerAndroid, index);
        }

        return null;
    }

    @Override
    public NavigationEntry getPendingEntry() {
        if (mNativeNavigationControllerAndroid != 0) {
            return nativeGetPendingEntry(mNativeNavigationControllerAndroid);
        }

        return null;
    }

    @Override
    public int getLastCommittedEntryIndex() {
        if (mNativeNavigationControllerAndroid != 0) {
            return nativeGetLastCommittedEntryIndex(mNativeNavigationControllerAndroid);
        }
        return -1;
    }

    @Override
    public boolean removeEntryAtIndex(int index) {
        if (mNativeNavigationControllerAndroid != 0) {
            return nativeRemoveEntryAtIndex(mNativeNavigationControllerAndroid, index);
        }
        return false;
    }

    @Override
    public boolean canCopyStateOver() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanCopyStateOver(mNativeNavigationControllerAndroid);
    }

    @Override
    public boolean canPruneAllButLastCommitted() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanPruneAllButLastCommitted(mNativeNavigationControllerAndroid);
    }

    @Override
    public void copyStateFrom(NavigationController source, boolean needsReload) {
        if (mNativeNavigationControllerAndroid == 0) return;
        NavigationControllerImpl sourceImpl = (NavigationControllerImpl) source;
        if (sourceImpl.mNativeNavigationControllerAndroid == 0) return;
        nativeCopyStateFrom(mNativeNavigationControllerAndroid,
                sourceImpl.mNativeNavigationControllerAndroid, needsReload);
    }

    @Override
    public void copyStateFromAndPrune(NavigationController source, boolean replaceEntry) {
        if (mNativeNavigationControllerAndroid == 0) return;
        NavigationControllerImpl sourceImpl = (NavigationControllerImpl) source;
        if (sourceImpl.mNativeNavigationControllerAndroid == 0) return;
        nativeCopyStateFromAndPrune(
                mNativeNavigationControllerAndroid,
                sourceImpl.mNativeNavigationControllerAndroid,
                replaceEntry);
    }

    @Override
    public String getEntryExtraData(int index, String key) {
        if (mNativeNavigationControllerAndroid == 0) return null;
        return nativeGetEntryExtraData(mNativeNavigationControllerAndroid, index, key);
    }

    @Override
    public void setEntryExtraData(int index, String key, String value) {
        if (mNativeNavigationControllerAndroid == 0) return;
        nativeSetEntryExtraData(mNativeNavigationControllerAndroid, index, key, value);
    }

    @CalledByNative
    private static void addToNavigationHistory(Object history, Object navigationEntry) {
        ((NavigationHistory) history).addEntry((NavigationEntry) navigationEntry);
    }

    @CalledByNative
    private static NavigationEntry createNavigationEntry(int index, String url,
            String virtualUrl, String originalUrl, String title, Bitmap favicon, int transition) {
        return new NavigationEntry(index, url, virtualUrl, originalUrl, title, favicon, transition);
    }

    private native boolean nativeCanGoBack(long nativeNavigationControllerAndroid);
    private native boolean nativeCanGoForward(long nativeNavigationControllerAndroid);
    private native boolean nativeIsInitialNavigation(long nativeNavigationControllerAndroid);
    private native void nativeLoadIfNecessary(long nativeNavigationControllerAndroid);
    private native boolean nativeNeedsReload(long nativeNavigationControllerAndroid);
    private native void nativeSetNeedsReload(long nativeNavigationControllerAndroid);
    private native boolean nativeCanGoToOffset(
            long nativeNavigationControllerAndroid, int offset);
    private native void nativeGoBack(long nativeNavigationControllerAndroid);
    private native void nativeGoForward(long nativeNavigationControllerAndroid);
    private native void nativeGoToOffset(long nativeNavigationControllerAndroid, int offset);
    private native void nativeGoToNavigationIndex(
            long nativeNavigationControllerAndroid, int index);
    private native void nativeCancelPendingReload(long nativeNavigationControllerAndroid);
    private native void nativeContinuePendingReload(long nativeNavigationControllerAndroid);
    private native void nativeReload(long nativeNavigationControllerAndroid,
            boolean checkForRepost);
    private native void nativeReloadBypassingCache(long nativeNavigationControllerAndroid,
            boolean checkForRepost);
    private native void nativeLoadUrl(long nativeNavigationControllerAndroid, String url,
            int loadUrlType, int transitionType, String referrerUrl, int referrerPolicy,
            int uaOverrideOption, String extraHeaders, ResourceRequestBody postData,
            String baseUrlForDataUrl, String virtualUrlForDataUrl, String dataUrlAsString,
            boolean canLoadLocalResources, boolean isRendererInitiated,
            boolean shouldReplaceCurrentEntry);
    private native void nativeClearHistory(long nativeNavigationControllerAndroid);
    private native int nativeGetNavigationHistory(long nativeNavigationControllerAndroid,
            Object history);
    private native void nativeGetDirectedNavigationHistory(long nativeNavigationControllerAndroid,
            NavigationHistory history, boolean isForward, int itemLimit);
    private native String nativeGetOriginalUrlForVisibleNavigationEntry(
            long nativeNavigationControllerAndroid);
    private native void nativeClearSslPreferences(long nativeNavigationControllerAndroid);
    private native boolean nativeGetUseDesktopUserAgent(long nativeNavigationControllerAndroid);
    private native void nativeSetUseDesktopUserAgent(long nativeNavigationControllerAndroid,
            boolean override, boolean reloadOnChange);
    private native NavigationEntry nativeGetEntryAtIndex(
            long nativeNavigationControllerAndroid, int index);
    private native NavigationEntry nativeGetPendingEntry(long nativeNavigationControllerAndroid);
    private native int nativeGetLastCommittedEntryIndex(long nativeNavigationControllerAndroid);
    private native boolean nativeRemoveEntryAtIndex(long nativeNavigationControllerAndroid,
            int index);
    private native boolean nativeCanCopyStateOver(long nativeNavigationControllerAndroid);
    private native boolean nativeCanPruneAllButLastCommitted(
            long nativeNavigationControllerAndroid);
    private native void nativeCopyStateFrom(long nativeNavigationControllerAndroid,
            long sourceNavigationControllerAndroid, boolean needsReload);
    private native void nativeCopyStateFromAndPrune(long nativeNavigationControllerAndroid,
            long sourceNavigationControllerAndroid, boolean replaceEntry);
    private native String nativeGetEntryExtraData(
            long nativeNavigationControllerAndroid, int index, String key);
    private native void nativeSetEntryExtraData(
            long nativeNavigationControllerAndroid, int index, String key, String value);
}
