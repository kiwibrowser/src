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

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.ticl.BasicSystemResources;
import com.google.ipc.invalidation.ticl.android2.channel.AndroidNetworkChannel;
import com.google.ipc.invalidation.util.Preconditions;

import android.content.Context;

/**
 * Factory class for Android system resources.
 *
 */
public class ResourcesFactory {
  /**
   * A scheduler that supports no operations. Used as the listener scheduler, which should never be
   * called in Android.
   */
  private static class InvalidScheduler implements Scheduler {
    @Override
    public void setSystemResources(SystemResources resources) {
    }

    @Override
    public void schedule(int delayMs, Runnable runnable) {
      throw new UnsupportedOperationException();
    }

    @Override
    public boolean isRunningOnThread() {
      throw new UnsupportedOperationException();
    }

    @Override
    public long getCurrentTimeMs() {
      throw new UnsupportedOperationException();
    }

  }

  /** Implementation of {@link SystemResources} for the Android Ticl. */
  public static class AndroidResources extends BasicSystemResources {
    /** Android system context. */
    private final Context context;

    /** Ticl-provided receiver for network events. */
    private NetworkChannel.NetworkListener networkListener;

    /**
     * Creates an instance of resources for production code.
     *
     * @param context Android system context
     * @param clock source of time for the internal scheduler
     * @param logPrefix log prefix
     */
    private AndroidResources(Context context, AndroidClock clock, String logPrefix) {
      super(AndroidLogger.forPrefix(logPrefix), new AndroidInternalScheduler(context, clock),
          new InvalidScheduler(), new AndroidNetworkChannel(context), new AndroidStorage(context),
          getPlatformString());
      this.context = Preconditions.checkNotNull(context);
    }

    /** Creates an instance for test from the provided resources and context. */
    
    AndroidResources(Logger logger, AndroidInternalScheduler internalScheduler,
        NetworkChannel network, Storage storage, Context context) {
      super(logger, internalScheduler, new InvalidScheduler(), network, storage,
          getPlatformString());
      this.context = Preconditions.checkNotNull(context);
    }

    /** Returns the Android system context. */
    Context getContext() {
      return context;
    }

    /**
     * Sets the network message listener provided by the Ticl. The network calls this method when
     * the Ticl provides it with a listener; the Ticl service later retrieves the listener when
     * it has a network event to communicate to the Ticl.
     */
    public void setNetworkListener(NetworkChannel.NetworkListener networkListener) {
      if (this.networkListener != null) {
        throw new IllegalStateException("Listener already set: " + networkListener);
      }
      this.networkListener = Preconditions.checkNotNull(networkListener);
    }

    /** Clears the network listener. */
    void clearNetworkListener() {
      this.networkListener = null;
    }

    /** Returns the network listener provided by the Ticl. */
    NetworkChannel.NetworkListener getNetworkListener() {
      return Preconditions.checkNotNull(networkListener, "network listener not yet set");
    }

    /** Returns the platform string to use when constructing the resources. */
    private static String getPlatformString() {
      return "Android-" + android.os.Build.VERSION.RELEASE;
    }
  }

  /**
   * Creates a production instance.
   *
   * @param context Android system context
   * @param clock source of time for the internal scheduler
   * @param prefix log prefix
   */
  static AndroidResources createResources(Context context, AndroidClock clock, String prefix) {
    return new AndroidResources(context, clock, prefix);
  }

  private ResourcesFactory() {
    // Prevent instantiation.
  }
}
