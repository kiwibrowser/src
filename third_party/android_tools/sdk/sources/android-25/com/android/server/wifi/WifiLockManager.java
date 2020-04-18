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

package com.android.server.wifi;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.WorkSource;
import android.util.Slog;

import com.android.internal.app.IBatteryStats;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * WifiLockManager maintains the list of wake locks held by different applications.
 */
public class WifiLockManager {
    private static final String TAG = "WifiLockManager";
    private boolean mVerboseLoggingEnabled = false;

    private final Context mContext;
    private final IBatteryStats mBatteryStats;

    private final List<WifiLock> mWifiLocks = new ArrayList<>();
    // some wifi lock statistics
    private int mFullHighPerfLocksAcquired;
    private int mFullHighPerfLocksReleased;
    private int mFullLocksAcquired;
    private int mFullLocksReleased;
    private int mScanLocksAcquired;
    private int mScanLocksReleased;

    WifiLockManager(Context context, IBatteryStats batteryStats) {
        mContext = context;
        mBatteryStats = batteryStats;
    }

    /**
     * Method allowing a calling app to acquire a Wifi WakeLock in the supplied mode.
     *
     * This method verifies that the caller has permission to make the call and that the lock mode
     * is a valid WifiLock mode.
     * @param lockMode int representation of the Wifi WakeLock type.
     * @param tag String passed to WifiManager.WifiLock
     * @param binder IBinder for the calling app
     * @param ws WorkSource of the calling app
     *
     * @return true if the lock was successfully acquired, false if the lockMode was invalid.
     */
    public boolean acquireWifiLock(int lockMode, String tag, IBinder binder, WorkSource ws) {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.WAKE_LOCK, null);
        if (!isValidLockMode(lockMode)) {
            throw new IllegalArgumentException("lockMode =" + lockMode);
        }
        if (ws == null || ws.size() == 0) {
            ws = new WorkSource(Binder.getCallingUid());
        } else {
            mContext.enforceCallingOrSelfPermission(
                    android.Manifest.permission.UPDATE_DEVICE_STATS, null);
        }
        return addLock(new WifiLock(lockMode, tag, binder, ws));
    }

    /**
     * Method used by applications to release a WiFi Wake lock.  This method checks permissions for
     * the caller and if allowed, releases the underlying WifiLock(s).
     *
     * @param binder IBinder for the calling app.
     * @return true if the lock was released, false if the caller did not hold any locks
     */
    public boolean releaseWifiLock(IBinder binder) {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.WAKE_LOCK, null);
        return releaseLock(binder);
    }

    /**
     * Method used to get the strongest lock type currently held by the WifiLockManager.
     *
     * If no locks are held, WifiManager.WIFI_MODE_NO_LOCKS_HELD is returned.
     *
     * @return int representing the currently held (highest power consumption) lock.
     */
    public synchronized int getStrongestLockMode() {
        if (mWifiLocks.isEmpty()) {
            return WifiManager.WIFI_MODE_NO_LOCKS_HELD;
        }

        if (mFullHighPerfLocksAcquired > mFullHighPerfLocksReleased) {
            return WifiManager.WIFI_MODE_FULL_HIGH_PERF;
        }

        if (mFullLocksAcquired > mFullLocksReleased) {
            return WifiManager.WIFI_MODE_FULL;
        }

        return WifiManager.WIFI_MODE_SCAN_ONLY;
    }

    /**
     * Method to create a WorkSource containing all active WifiLock WorkSources.
     */
    public synchronized WorkSource createMergedWorkSource() {
        WorkSource mergedWS = new WorkSource();
        for (WifiLock lock : mWifiLocks) {
            mergedWS.add(lock.getWorkSource());
        }
        return mergedWS;
    }

    /**
     * Method used to update WifiLocks with a new WorkSouce.
     *
     * @param binder IBinder for the calling application.
     * @param ws WorkSource to add to the existing WifiLock(s).
     */
    public synchronized void updateWifiLockWorkSource(IBinder binder, WorkSource ws) {
        // Does the caller have permission to make this call?
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.UPDATE_DEVICE_STATS, null);

        // Now check if there is an active lock
        WifiLock wl = findLockByBinder(binder);
        if (wl == null) {
            throw new IllegalArgumentException("Wifi lock not active");
        }

        WorkSource newWorkSource;
        if (ws == null || ws.size() == 0) {
            newWorkSource = new WorkSource(Binder.getCallingUid());
        } else {
            // Make a copy of the WorkSource before adding it to the WakeLock
            newWorkSource = new WorkSource(ws);
        }

        long ident = Binder.clearCallingIdentity();
        try {
            mBatteryStats.noteFullWifiLockReleasedFromSource(wl.mWorkSource);
            wl.mWorkSource = newWorkSource;
            mBatteryStats.noteFullWifiLockAcquiredFromSource(wl.mWorkSource);
        } catch (RemoteException e) {
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
    }

    private static boolean isValidLockMode(int lockMode) {
        if (lockMode != WifiManager.WIFI_MODE_FULL
                && lockMode != WifiManager.WIFI_MODE_SCAN_ONLY
                && lockMode != WifiManager.WIFI_MODE_FULL_HIGH_PERF) {
            return false;
        }
        return true;
    }

    private synchronized boolean addLock(WifiLock lock) {
        if (mVerboseLoggingEnabled) {
            Slog.d(TAG, "addLock: " + lock);
        }

        if (findLockByBinder(lock.getBinder()) != null) {
            if (mVerboseLoggingEnabled) {
                Slog.d(TAG, "attempted to add a lock when already holding one");
            }
            return false;
        }

        mWifiLocks.add(lock);

        boolean lockAdded = false;
        long ident = Binder.clearCallingIdentity();
        try {
            mBatteryStats.noteFullWifiLockAcquiredFromSource(lock.mWorkSource);
            switch(lock.mMode) {
                case WifiManager.WIFI_MODE_FULL:
                    ++mFullLocksAcquired;
                    break;
                case WifiManager.WIFI_MODE_FULL_HIGH_PERF:
                    ++mFullHighPerfLocksAcquired;
                    break;
                case WifiManager.WIFI_MODE_SCAN_ONLY:
                    ++mScanLocksAcquired;
                    break;
            }
            lockAdded = true;
        } catch (RemoteException e) {
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        return lockAdded;
    }

    private synchronized WifiLock removeLock(IBinder binder) {
        WifiLock lock = findLockByBinder(binder);
        if (lock != null) {
            mWifiLocks.remove(lock);
            lock.unlinkDeathRecipient();
        }
        return lock;
    }

    private synchronized boolean releaseLock(IBinder binder) {
        WifiLock wifiLock = removeLock(binder);
        if (wifiLock == null) {
            // attempting to release a lock that is not active.
            return false;
        }

        if (mVerboseLoggingEnabled) {
            Slog.d(TAG, "releaseLock: " + wifiLock);
        }

        long ident = Binder.clearCallingIdentity();
        try {
            mBatteryStats.noteFullWifiLockReleasedFromSource(wifiLock.mWorkSource);
            switch(wifiLock.mMode) {
                case WifiManager.WIFI_MODE_FULL:
                    ++mFullLocksReleased;
                    break;
                case WifiManager.WIFI_MODE_FULL_HIGH_PERF:
                    ++mFullHighPerfLocksReleased;
                    break;
                case WifiManager.WIFI_MODE_SCAN_ONLY:
                    ++mScanLocksReleased;
                    break;
            }
        } catch (RemoteException e) {
        } finally {
            Binder.restoreCallingIdentity(ident);
        }
        return true;
    }


    private synchronized WifiLock findLockByBinder(IBinder binder) {
        for (WifiLock lock : mWifiLocks) {
            if (lock.getBinder() == binder) {
                return lock;
            }
        }
        return null;
    }

    protected void dump(PrintWriter pw) {
        pw.println("Locks acquired: " + mFullLocksAcquired + " full, "
                + mFullHighPerfLocksAcquired + " full high perf, "
                + mScanLocksAcquired + " scan");
        pw.println("Locks released: " + mFullLocksReleased + " full, "
                + mFullHighPerfLocksReleased + " full high perf, "
                + mScanLocksReleased + " scan");
        pw.println();
        pw.println("Locks held:");
        for (WifiLock lock : mWifiLocks) {
            pw.print("    ");
            pw.println(lock);
        }
    }

    protected void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            mVerboseLoggingEnabled = true;
        } else {
            mVerboseLoggingEnabled = false;
        }
    }

    private class WifiLock implements IBinder.DeathRecipient {
        String mTag;
        int mUid;
        IBinder mBinder;
        int mMode;
        WorkSource mWorkSource;

        WifiLock(int lockMode, String tag, IBinder binder, WorkSource ws) {
            mTag = tag;
            mBinder = binder;
            mUid = Binder.getCallingUid();
            mMode = lockMode;
            mWorkSource = ws;
            try {
                mBinder.linkToDeath(this, 0);
            } catch (RemoteException e) {
                binderDied();
            }
        }

        protected WorkSource getWorkSource() {
            return mWorkSource;
        }

        protected int getUid() {
            return mUid;
        }

        protected IBinder getBinder() {
            return mBinder;
        }

        public void binderDied() {
            releaseLock(mBinder);
        }

        public void unlinkDeathRecipient() {
            mBinder.unlinkToDeath(this, 0);
        }

        public String toString() {
            return "WifiLock{" + this.mTag + " type=" + this.mMode + " uid=" + mUid + "}";
        }
    }
}
