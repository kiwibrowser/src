/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.wifi.nan;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.IWifiNanEventListener;
import android.net.wifi.nan.IWifiNanManager;
import android.net.wifi.nan.IWifiNanSessionListener;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.os.Binder;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.util.SparseArray;

import java.io.FileDescriptor;
import java.io.PrintWriter;

public class WifiNanServiceImpl extends IWifiNanManager.Stub {
    private static final String TAG = "WifiNanService";
    private static final boolean DBG = false;
    private static final boolean VDBG = false; // STOPSHIP if true

    private Context mContext;
    private WifiNanStateManager mStateManager;
    private final boolean mNanSupported;

    private final Object mLock = new Object();
    private final SparseArray<IBinder.DeathRecipient> mDeathRecipientsByUid = new SparseArray<>();
    private int mNextNetworkRequestToken = 1;
    private int mNextSessionId = 1;

    public WifiNanServiceImpl(Context context) {
        mContext = context.getApplicationContext();

        mNanSupported = mContext.getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_WIFI_NAN);
        if (DBG) Log.w(TAG, "WifiNanServiceImpl: mNanSupported=" + mNanSupported);

        mStateManager = WifiNanStateManager.getInstance();
    }

    public void start() {
        Log.i(TAG, "Starting Wi-Fi NAN service");

        // TODO: share worker thread with other Wi-Fi handlers
        HandlerThread wifiNanThread = new HandlerThread("wifiNanService");
        wifiNanThread.start();

        mStateManager.start(wifiNanThread.getLooper());
    }

    @Override
    public void connect(final IBinder binder, IWifiNanEventListener listener, int events) {
        enforceAccessPermission();
        enforceChangePermission();

        final int uid = getCallingUid();

        if (VDBG) Log.v(TAG, "connect: uid=" + uid);


        IBinder.DeathRecipient dr = new IBinder.DeathRecipient() {
            @Override
            public void binderDied() {
                if (DBG) Log.d(TAG, "binderDied: uid=" + uid);
                binder.unlinkToDeath(this, 0);

                synchronized (mLock) {
                    mDeathRecipientsByUid.delete(uid);
                }

                mStateManager.disconnect(uid);
            }
        };
        synchronized (mLock) {
            mDeathRecipientsByUid.put(uid, dr);
        }
        try {
            binder.linkToDeath(dr, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "Error on linkToDeath - " + e);
        }

        mStateManager.connect(uid, listener, events);
    }

    @Override
    public void disconnect(IBinder binder) {
        enforceAccessPermission();
        enforceChangePermission();

        int uid = getCallingUid();

        if (VDBG) Log.v(TAG, "disconnect: uid=" + uid);

        synchronized (mLock) {
            IBinder.DeathRecipient dr = mDeathRecipientsByUid.get(uid);
            if (dr != null) {
                binder.unlinkToDeath(dr, 0);
                mDeathRecipientsByUid.delete(uid);
            }
        }

        mStateManager.disconnect(uid);
    }

    @Override
    public void requestConfig(ConfigRequest configRequest) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) {
            Log.v(TAG,
                    "requestConfig: uid=" + getCallingUid() + ", configRequest=" + configRequest);
        }

        mStateManager.requestConfig(getCallingUid(), configRequest);
    }

    @Override
    public void stopSession(int sessionId) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) Log.v(TAG, "stopSession: sessionId=" + sessionId + ", uid=" + getCallingUid());

        mStateManager.stopSession(getCallingUid(), sessionId);
    }

    @Override
    public void destroySession(int sessionId) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) Log.v(TAG, "destroySession: sessionId=" + sessionId + ", uid=" + getCallingUid());

        mStateManager.destroySession(getCallingUid(), sessionId);
    }

    @Override
    public int createSession(IWifiNanSessionListener listener, int events) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) Log.v(TAG, "createSession: uid=" + getCallingUid());

        int sessionId;
        synchronized (mLock) {
            sessionId = mNextSessionId++;
        }

        mStateManager.createSession(getCallingUid(), sessionId, listener, events);

        return sessionId;
    }

    @Override
    public void publish(int sessionId, PublishData publishData, PublishSettings publishSettings) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) {
            Log.v(TAG, "publish: uid=" + getCallingUid() + ", sessionId=" + sessionId + ", data='"
                    + publishData + "', settings=" + publishSettings);
        }

        mStateManager.publish(getCallingUid(), sessionId, publishData, publishSettings);
    }

    @Override
    public void subscribe(int sessionId, SubscribeData subscribeData,
            SubscribeSettings subscribeSettings) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) {
            Log.v(TAG, "subscribe: uid=" + getCallingUid() + ", sessionId=" + sessionId + ", data='"
                    + subscribeData + "', settings=" + subscribeSettings);
        }

        mStateManager.subscribe(getCallingUid(), sessionId, subscribeData, subscribeSettings);
    }

    @Override
    public void sendMessage(int sessionId, int peerId, byte[] message, int messageLength,
            int messageId) {
        enforceAccessPermission();
        enforceChangePermission();

        if (VDBG) {
            Log.v(TAG,
                    "sendMessage: sessionId=" + sessionId + ", uid=" + getCallingUid() + ", peerId="
                            + peerId + ", messageLength=" + messageLength + ", messageId="
                            + messageId);
        }

        mStateManager.sendMessage(getCallingUid(), sessionId, peerId, message, messageLength,
                messageId);
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mContext.checkCallingOrSelfPermission(
                android.Manifest.permission.DUMP) != PackageManager.PERMISSION_GRANTED) {
            pw.println("Permission Denial: can't dump WifiNanService from pid="
                    + Binder.getCallingPid() + ", uid=" + Binder.getCallingUid());
            return;
        }
        pw.println("Wi-Fi NAN Service");
        pw.println("  mNanSupported: " + mNanSupported);
        pw.println("  mNextNetworkRequestToken: " + mNextNetworkRequestToken);
        pw.println("  mNextSessionId: " + mNextSessionId);
        pw.println("  mDeathRecipientsByUid: " + mDeathRecipientsByUid);
        mStateManager.dump(fd, pw, args);
    }

    private void enforceAccessPermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.ACCESS_WIFI_STATE, TAG);
    }

    private void enforceChangePermission() {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.CHANGE_WIFI_STATE, TAG);
    }
}
