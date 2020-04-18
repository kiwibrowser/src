/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.ticl.android2;

import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.util.Preconditions;

import android.content.Context;
import android.os.Build;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Singleton that manages wake locks identified by a key. Wake locks are refcounted so if they are
 * acquired multiple times with the same key they will not unlocked until they are released an
 * equivalent number of times.
 */
public class WakeLockManager {
  /** Logger. */
  private static final Logger logger = AndroidLogger.forTag("WakeLockMgr");

  /** Lock over all state. Must be acquired by all non-private methods. */
  private static final Object LOCK = new Object();

  /**
   * SDK_INT version taken from android.BUILD.VERSION_CODE.ICE_CREAM_SANDWICH. We cannot reference
   * the field directly because if it is not inlined by the Java compiler, it will not be available
   * in the earlier versions of Android for which the version check in acquire() exists.
   */
  private static final int ICE_CREAM_SANDWICH_VERSION_CODE = 14;

  /** Singleton instance. */
  private static WakeLockManager theManager;

  /** Wake locks by key. */
  private final Map<Object, PowerManager.WakeLock> wakeLocks =
      new HashMap<Object, PowerManager.WakeLock>();

  private final PowerManager powerManager;

  private final Context applicationContext;

  private WakeLockManager(Context context) {
    powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
    applicationContext = Preconditions.checkNotNull(context);
  }

  /** Returns the wake lock manager. */
  public static WakeLockManager getInstance(Context context) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(context.getApplicationContext());
    synchronized (LOCK) {
      if (theManager == null) {
        theManager = new WakeLockManager(context.getApplicationContext());
      } else {
        if (theManager.applicationContext != context.getApplicationContext()) {
          String message = new StringBuilder()
              .append("Provided context ")
              .append(context.getApplicationContext())
              .append("does not match stored context ")
              .append(theManager.applicationContext)
              .toString();
          throw new IllegalStateException(message);
        }
      }
      return theManager;
    }
  }

  /**
   * Acquires a wake lock identified by the {@code key} that will be automatically released after at
   * most {@code timeoutMs}.
   */
  public void acquire(Object key, int timeoutMs) {
    synchronized (LOCK) {
      cleanup();
      Preconditions.checkNotNull(key, "Key can not be null");

      // Prior to ICS, acquiring a lock with a timeout and then explicitly releasing the lock
      // results in runtime errors. We rely on the invalidation system correctly releasing locks
      // rather than defensively requesting a timeout.
      if (Build.VERSION.SDK_INT >= ICE_CREAM_SANDWICH_VERSION_CODE) {
        log(key, "acquiring with timeout " + timeoutMs);
        getWakeLock(key).acquire(timeoutMs);
      } else {
        log(key, "acquiring");
        getWakeLock(key).acquire();
      }
    }
  }

  /**
   * Releases the wake lock identified by the {@code key} if it is currently held.
   */
  public void release(Object key) {
    synchronized (LOCK) {
      cleanup();
      Preconditions.checkNotNull(key, "Key can not be null");
      PowerManager.WakeLock wakelock = getWakeLock(key);

      // If the lock is not held (if for instance there is a wake lock timeout), we cannot release
      // again without triggering a RuntimeException.
      if (!wakelock.isHeld()) {
        logger.warning("Over-release of wakelock: %s", key);
        return;
      }

      // We held the wake lock recently, so it's likely safe to release it. Between the isHeld()
      // check and the release() call, the wake lock may time out however and we catch the resulting
      // RuntimeException.
      try {
        wakelock.release();
      } catch (RuntimeException exception) {
        logger.warning("Over-release of wakelock: %s, %s", key, exception);
      }
      log(key, "released");

      // Now if the lock is not held, that means we were the last holder, so we should remove it
      // from the map.
      if (!wakelock.isHeld()) {
        wakeLocks.remove(key);
        log(key, "freed");
      }
    }
  }

  /**
   * Returns whether there is currently a wake lock held for the provided {@code key}.
   */
  public boolean isHeld(Object key) {
    synchronized (LOCK) {
      cleanup();
      Preconditions.checkNotNull(key, "Key can not be null");
      if (!wakeLocks.containsKey(key)) {
        return false;
      }
      return getWakeLock(key).isHeld();
    }
  }

  /** Returns whether the manager has any active (held) wake locks. */
  
  public boolean hasWakeLocks() {
    synchronized (LOCK) {
      cleanup();
      return !wakeLocks.isEmpty();
    }
  }

  /** Discards (without releasing) all wake locks. */
  
  public void resetForTest() {
    synchronized (LOCK) {
      cleanup();
      wakeLocks.clear();
    }
  }

  /**
   * Returns a wake lock to use for {@code key}. If a lock is already present in the map,
   * returns that lock. Else, creates a new lock, installs it in the map, and returns it.
   * <p>
   * REQUIRES: caller must hold {@link #LOCK}.
   */
  private PowerManager.WakeLock getWakeLock(Object key) {
    if (key == null) {
      throw new IllegalArgumentException("Key can not be null");
    }
    PowerManager.WakeLock wakeLock = wakeLocks.get(key);
    if (wakeLock == null) {
      wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, key.toString());
      wakeLocks.put(key, wakeLock);
    }
    return wakeLock;
  }

  /**
   * Removes any non-held wake locks from {@link #wakeLocks}. Such locks may be present when a
   * wake lock acquired with a timeout is not released before the timeout expires. We only
   * explicitly remove wake locks from the map when {@link #release} is called, so a timeout results
   * in a non-held wake lock in the map.
   * <p>
   * Must be called as the first line of all non-private methods.
   * <p>
   * REQUIRES: caller must hold {@link #LOCK}.
   */
  private void cleanup() {
    Iterator<Map.Entry<Object, WakeLock>> wakeLockIter = wakeLocks.entrySet().iterator();

    // Check each map entry.
    while (wakeLockIter.hasNext()) {
      Map.Entry<Object, WakeLock> wakeLockEntry = wakeLockIter.next();
      if (!wakeLockEntry.getValue().isHeld()) {
        // Warn and remove the entry from the map if the lock is not held.
        logger.warning("Found un-held wakelock '%s' -- timed-out?", wakeLockEntry.getKey());
        wakeLockIter.remove();
      }
    }
  }

  /** Logs a debug message that {@code action} has occurred for {@code key}. */
  private static void log(Object key, String action) {
    logger.fine("WakeLock %s for key: {%s}", action, key);
  }
}
