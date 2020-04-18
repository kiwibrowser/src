// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.offline_items_collection;

import android.os.Handler;

import org.chromium.base.Callback;
import org.chromium.base.ObserverList;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.ArrayList;

/**
 * A helper class responsible for exposing the C++ OfflineContentAggregator
 * (components/offline_items_collection/core/offline_content_aggregator.h) class to Java.  This
 * class is created and owned by it's C++ counterpart OfflineContentAggregatorBridge
 * (components/offline_items_collection/core/android/offline_content_aggregator_bridge.h).
 */
@JNINamespace("offline_items_collection::android")
public class OfflineContentAggregatorBridge implements OfflineContentProvider {
    private final Handler mHandler = new Handler();

    private long mNativeOfflineContentAggregatorBridge;
    private ObserverList<OfflineContentProvider.Observer> mObservers;

    /**
     * A private constructor meant to be called by the C++ OfflineContentAggregatorBridge.
     * @param nativeOfflineContentAggregatorBridge A C++ pointer to the
     * OfflineContentAggregatorBridge.
     */
    private OfflineContentAggregatorBridge(long nativeOfflineContentAggregatorBridge) {
        mNativeOfflineContentAggregatorBridge = nativeOfflineContentAggregatorBridge;
        mObservers = new ObserverList<OfflineContentProvider.Observer>();
    }

    // OfflineContentProvider implementation.
    @Override
    public void openItem(ContentId id) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeOpenItem(mNativeOfflineContentAggregatorBridge, id.namespace, id.id);
    }

    @Override
    public void removeItem(ContentId id) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeRemoveItem(mNativeOfflineContentAggregatorBridge, id.namespace, id.id);
    }

    @Override
    public void cancelDownload(ContentId id) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeCancelDownload(mNativeOfflineContentAggregatorBridge, id.namespace, id.id);
    }

    @Override
    public void pauseDownload(ContentId id) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativePauseDownload(mNativeOfflineContentAggregatorBridge, id.namespace, id.id);
    }

    @Override
    public void resumeDownload(ContentId id, boolean hasUserGesture) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeResumeDownload(
                mNativeOfflineContentAggregatorBridge, id.namespace, id.id, hasUserGesture);
    }

    @Override
    public void getItemById(ContentId id, Callback<OfflineItem> callback) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeGetItemById(mNativeOfflineContentAggregatorBridge, id.namespace, id.id, callback);
    }

    @Override
    public void getAllItems(Callback<ArrayList<OfflineItem>> callback) {
        if (mNativeOfflineContentAggregatorBridge == 0) return;
        nativeGetAllItems(mNativeOfflineContentAggregatorBridge, callback);
    }

    @Override
    public void getVisualsForItem(ContentId id, VisualsCallback callback) {
        nativeGetVisualsForItem(
                mNativeOfflineContentAggregatorBridge, id.namespace, id.id, callback);
    }

    @Override
    public void addObserver(final OfflineContentProvider.Observer observer) {
        mObservers.addObserver(observer);
    }

    @Override
    public void removeObserver(OfflineContentProvider.Observer observer) {
        mObservers.removeObserver(observer);
    }

    // Methods called from C++ via JNI.
    @CalledByNative
    private void onItemsAdded(ArrayList<OfflineItem> items) {
        if (items.size() == 0) return;

        for (Observer observer : mObservers) {
            observer.onItemsAdded(items);
        }
    }

    @CalledByNative
    private void onItemRemoved(String nameSpace, String id) {
        ContentId contentId = new ContentId(nameSpace, id);

        for (Observer observer : mObservers) {
            observer.onItemRemoved(contentId);
        }
    }

    @CalledByNative
    private void onItemUpdated(OfflineItem item) {
        for (Observer observer : mObservers) {
            observer.onItemUpdated(item);
        }
    }

    @CalledByNative
    private static void onVisualsAvailable(
            VisualsCallback callback, String nameSpace, String id, OfflineItemVisuals visuals) {
        callback.onVisualsAvailable(new ContentId(nameSpace, id), visuals);
    }

    /**
     * Called when the C++ OfflineContentAggregatorBridge is destroyed.  This tears down the Java
     * component of the JNI bridge so that this class, which may live due to other references, no
     * longer attempts to access the C++ side of the bridge.
     */
    @CalledByNative
    private void onNativeDestroyed() {
        mNativeOfflineContentAggregatorBridge = 0;
    }

    /**
     * A private static factory method meant to be called by the C++ OfflineContentAggregatorBridge.
     * @param nativeOfflineContentAggregatorBridge A C++ pointer to the
     * OfflineContentAggregatorBridge.
     * @return A new instance of this OfflineContentAggregatorBridge.
     */
    @CalledByNative
    private static OfflineContentAggregatorBridge create(
            long nativeOfflineContentAggregatorBridge) {
        return new OfflineContentAggregatorBridge(nativeOfflineContentAggregatorBridge);
    }

    // Methods called to C++ via JNI.
    private native void nativeOpenItem(
            long nativeOfflineContentAggregatorBridge, String nameSpace, String id);
    private native void nativeRemoveItem(
            long nativeOfflineContentAggregatorBridge, String nameSpace, String id);
    private native void nativeCancelDownload(
            long nativeOfflineContentAggregatorBridge, String nameSpace, String id);
    private native void nativePauseDownload(
            long nativeOfflineContentAggregatorBridge, String nameSpace, String id);
    private native void nativeResumeDownload(long nativeOfflineContentAggregatorBridge,
            String nameSpace, String id, boolean hasUserGesture);
    private native void nativeGetItemById(long nativeOfflineContentAggregatorBridge,
            String nameSpace, String id, Callback<OfflineItem> callback);
    private native void nativeGetAllItems(
            long nativeOfflineContentAggregatorBridge, Callback<ArrayList<OfflineItem>> callback);
    private native void nativeGetVisualsForItem(long nativeOfflineContentAggregatorBridge,
            String nameSpace, String id, VisualsCallback callback);
}