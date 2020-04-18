// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.os.Process;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeClassQualifiedName;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.net.NetworkChangeNotifierAutoDetect;
import org.chromium.net.RegistrationPolicyAlwaysRegister;

import java.util.ArrayList;
import java.util.List;

/**
 * Contains the Java code used by the BackgroundSyncNetworkObserverAndroid C++ class.
 *
 * The purpose of this class is to listen for and forward network connectivity events to the
 * BackgroundSyncNetworkObserverAndroid objects even when the application is paused. The standard
 * NetworkChangeNotifier does not listen for connectivity events when the application is paused.
 *
 * This class maintains a NetworkChangeNotifierAutoDetect, which exists for as long as any
 * BackgroundSyncNetworkObserverAndroid objects are registered.
 *
 * This class lives on the main thread.
 */
@JNINamespace("content")
class BackgroundSyncNetworkObserver implements NetworkChangeNotifierAutoDetect.Observer {
    private static final String TAG = "cr_BgSyncNetObserver";

    private NetworkChangeNotifierAutoDetect mNotifier;

    // The singleton instance.
    @SuppressLint("StaticFieldLeak")
    private static BackgroundSyncNetworkObserver sInstance;

    // List of native observers. These are each called when the network state changes.
    private List<Long> mNativePtrs;

    private int mLastBroadcastConnectionType;
    private boolean mHasBroadcastConnectionType;

    private BackgroundSyncNetworkObserver() {
        ThreadUtils.assertOnUiThread();
        mNativePtrs = new ArrayList<Long>();
    }

    private static boolean canCreateObserver() {
        return ApiCompatibilityUtils.checkPermission(ContextUtils.getApplicationContext(),
                       Manifest.permission.ACCESS_NETWORK_STATE, Process.myPid(), Process.myUid())
                == PackageManager.PERMISSION_GRANTED;
    }

    @CalledByNative
    private static BackgroundSyncNetworkObserver createObserver(long nativePtr) {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new BackgroundSyncNetworkObserver();
        }
        sInstance.registerObserver(nativePtr);
        return sInstance;
    }

    private void registerObserver(final long nativePtr) {
        ThreadUtils.assertOnUiThread();
        if (!canCreateObserver()) {
            RecordHistogram.recordBooleanHistogram(
                    "BackgroundSync.NetworkObserver.HasPermission", false);
            return;
        }

        // Create the NetworkChangeNotifierAutoDetect if it does not exist already.
        if (mNotifier == null) {
            mNotifier = new NetworkChangeNotifierAutoDetect(
                    this, new RegistrationPolicyAlwaysRegister());
            RecordHistogram.recordBooleanHistogram(
                    "BackgroundSync.NetworkObserver.HasPermission", true);
        }
        mNativePtrs.add(nativePtr);

        nativeNotifyConnectionTypeChanged(
                nativePtr, mNotifier.getCurrentNetworkState().getConnectionType());
    }

    @CalledByNative
    private void removeObserver(long nativePtr) {
        ThreadUtils.assertOnUiThread();
        mNativePtrs.remove(nativePtr);
        // Destroy the NetworkChangeNotifierAutoDetect if there are no more observers.
        if (mNativePtrs.size() == 0 && mNotifier != null) {
            mNotifier.destroy();
            mNotifier = null;
        }
    }

    private void broadcastNetworkChangeIfNecessary(int newConnectionType) {
        if (mHasBroadcastConnectionType && newConnectionType == mLastBroadcastConnectionType)
            return;

        mHasBroadcastConnectionType = true;
        mLastBroadcastConnectionType = newConnectionType;
        for (Long nativePtr : mNativePtrs) {
            nativeNotifyConnectionTypeChanged(nativePtr, newConnectionType);
        }
    }

    @Override
    public void onConnectionTypeChanged(int newConnectionType) {
        ThreadUtils.assertOnUiThread();
        broadcastNetworkChangeIfNecessary(newConnectionType);
    }

    @Override
    public void onConnectionSubtypeChanged(int newConnectionSubtype) {}

    @Override
    public void onNetworkConnect(long netId, int connectionType) {
        ThreadUtils.assertOnUiThread();
        // If we're in doze mode (N+ devices), onConnectionTypeChanged may not
        // be called, but this function should. So update the connection type
        // if necessary.
        broadcastNetworkChangeIfNecessary(mNotifier.getCurrentNetworkState().getConnectionType());
    }

    @Override
    public void onNetworkSoonToDisconnect(long netId) {}

    @Override
    public void onNetworkDisconnect(long netId) {
        ThreadUtils.assertOnUiThread();
        // If we're in doze mode (N+ devices), onConnectionTypeChanged may not
        // be called, but this function should. So update the connection type
        // if necessary.
        broadcastNetworkChangeIfNecessary(mNotifier.getCurrentNetworkState().getConnectionType());
    }

    @Override
    public void purgeActiveNetworkList(long[] activeNetIds) {}

    @NativeClassQualifiedName("BackgroundSyncNetworkObserverAndroid::Observer")
    private native void nativeNotifyConnectionTypeChanged(long nativePtr, int newConnectionType);
}
