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

import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.IWifiNanEventListener;
import android.net.wifi.nan.IWifiNanSessionListener;
import android.net.wifi.nan.WifiNanEventListener;
import android.os.RemoteException;
import android.util.Log;
import android.util.SparseArray;

import java.io.FileDescriptor;
import java.io.PrintWriter;

public class WifiNanClientState {
    private static final String TAG = "WifiNanClientState";
    private static final boolean DBG = false;
    private static final boolean VDBG = false; // STOPSHIP if true

    /* package */ static final int CLUSTER_CHANGE_EVENT_STARTED = 0;
    /* package */ static final int CLUSTER_CHANGE_EVENT_JOINED = 1;

    private IWifiNanEventListener mListener;
    private int mEvents;
    private final SparseArray<WifiNanSessionState> mSessions = new SparseArray<>();

    private int mUid;
    private ConfigRequest mConfigRequest;

    public WifiNanClientState(int uid, IWifiNanEventListener listener, int events) {
        mUid = uid;
        mListener = listener;
        mEvents = events;
    }

    public void destroy() {
        mListener = null;
        for (int i = 0; i < mSessions.size(); ++i) {
            mSessions.valueAt(i).destroy();
        }
        mSessions.clear();
        mConfigRequest = null;
    }

    public void setConfigRequest(ConfigRequest configRequest) {
        mConfigRequest = configRequest;
    }

    public ConfigRequest getConfigRequest() {
        return mConfigRequest;
    }

    public int getUid() {
        return mUid;
    }

    public WifiNanSessionState getNanSessionStateForPubSubId(int pubSubId) {
        for (int i = 0; i < mSessions.size(); ++i) {
            WifiNanSessionState session = mSessions.valueAt(i);
            if (session.isPubSubIdSession(pubSubId)) {
                return session;
            }
        }

        return null;
    }

    public void createSession(int sessionId, IWifiNanSessionListener listener, int events) {
        WifiNanSessionState session = mSessions.get(sessionId);
        if (session != null) {
            Log.e(TAG, "createSession: sessionId already exists (replaced) - " + sessionId);
        }

        mSessions.put(sessionId, new WifiNanSessionState(sessionId, listener, events));
    }

    public void destroySession(int sessionId) {
        WifiNanSessionState session = mSessions.get(sessionId);
        if (session == null) {
            Log.e(TAG, "destroySession: sessionId doesn't exist - " + sessionId);
            return;
        }

        mSessions.delete(sessionId);
        session.destroy();
    }

    public WifiNanSessionState getSession(int sessionId) {
        return mSessions.get(sessionId);
    }

    public void onConfigCompleted(ConfigRequest completedConfig) {
        if (mListener != null && (mEvents & WifiNanEventListener.LISTEN_CONFIG_COMPLETED) != 0) {
            try {
                mListener.onConfigCompleted(completedConfig);
            } catch (RemoteException e) {
                Log.w(TAG, "onConfigCompleted: RemoteException - ignored: " + e);
            }
        }
    }

    public void onConfigFailed(ConfigRequest failedConfig, int reason) {
        if (mListener != null && (mEvents & WifiNanEventListener.LISTEN_CONFIG_FAILED) != 0) {
            try {
                mListener.onConfigFailed(failedConfig, reason);
            } catch (RemoteException e) {
                Log.w(TAG, "onConfigFailed: RemoteException - ignored: " + e);
            }
        }
    }

    public int onNanDown(int reason) {
        if (mListener != null && (mEvents & WifiNanEventListener.LISTEN_NAN_DOWN) != 0) {
            try {
                mListener.onNanDown(reason);
            } catch (RemoteException e) {
                Log.w(TAG, "onNanDown: RemoteException - ignored: " + e);
            }

            return 1;
        }

        return 0;
    }

    public int onInterfaceAddressChange(byte[] mac) {
        if (mListener != null && (mEvents & WifiNanEventListener.LISTEN_IDENTITY_CHANGED) != 0) {
            try {
                mListener.onIdentityChanged();
            } catch (RemoteException e) {
                Log.w(TAG, "onIdentityChanged: RemoteException - ignored: " + e);
            }

            return 1;
        }

        return 0;
    }

    public int onClusterChange(int flag, byte[] mac) {
        if (mListener != null && (mEvents & WifiNanEventListener.LISTEN_IDENTITY_CHANGED) != 0) {
            try {
                mListener.onIdentityChanged();
            } catch (RemoteException e) {
                Log.w(TAG, "onIdentityChanged: RemoteException - ignored: " + e);
            }

            return 1;
        }

        return 0;
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("NanClientState:");
        pw.println("  mUid: " + mUid);
        pw.println("  mConfigRequest: " + mConfigRequest);
        pw.println("  mListener: " + mListener);
        pw.println("  mEvents: " + mEvents);
        pw.println("  mSessions: [" + mSessions + "]");
        for (int i = 0; i < mSessions.size(); ++i) {
            mSessions.valueAt(i).dump(fd, pw, args);
        }
    }
}
