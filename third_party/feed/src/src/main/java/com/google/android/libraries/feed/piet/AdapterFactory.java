// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet;

import android.content.Context;

/**
 * An AdapterFactory manages binding a model to an Adapter and releasing it once we are done with
 * the view. This provides the basic support for Recycling Adapters and Views. The {@link
 * AdapterKeySupplier} is provided by the {@link ElementAdapter} to supply all the information
 * needed needed by this class.
 *
 * @param <A> A subclass of {@link ElementAdapter} which this factory is managing.
 * @param <M> The model. This is the model type which is bound to the view by the adapter.
 */
class AdapterFactory<A extends ElementAdapter<?, M>, M> {
  private static final String TAG = "AdapterFactory";

  // TODO: Make these configurable instead of constants.
  private static final int DEFAULT_POOL_SIZE = 100;
  private static final int DEFAULT_NUM_POOLS = 10;

  private final Context context;
  private final AdapterParameters parameters;
  private final AdapterKeySupplier<A, M> keySupplier;
  private final Statistics statistics;
  private final RecyclerPool<A> recyclingPool;

  /** Provides Adapter class level details to the factory. */
  interface AdapterKeySupplier<A extends ElementAdapter<?, M>, M> {

    /** Returns a String tag for the Adapter. This will be used in messages and developer logging */
    String getAdapterTag();

    /** Returns a new Adapter. */
    A getAdapter(Context context, AdapterParameters parameters);

    /** Returns the Key based upon the model. */
    RecyclerKey getKey(FrameContext frameContext, M model);
  }

  /** Key supplier with a singleton key. */
  abstract static class SingletonKeySupplier<A extends ElementAdapter<?, M>, M>
      implements AdapterKeySupplier<A, M> {
    public static final RecyclerKey SINGLETON_KEY = new RecyclerKey();

    @Override
    public RecyclerKey getKey(FrameContext frameContext, M model) {
      return SINGLETON_KEY;
    }
  }

  // SuppressWarnings for various errors. We don't want to have to import checker framework and
  // checker doesn't like generic assignments.
  @SuppressWarnings("nullness")
  AdapterFactory(
      Context context, AdapterParameters parameters, AdapterKeySupplier<A, M> keySupplier) {
    this.context = context;
    this.parameters = parameters;
    this.keySupplier = keySupplier;
    this.statistics = new Statistics(keySupplier.getAdapterTag());
    if (keySupplier instanceof SingletonKeySupplier) {
      recyclingPool =
          new SingleKeyRecyclerPool<>(SingletonKeySupplier.SINGLETON_KEY, DEFAULT_POOL_SIZE);
    } else {
      recyclingPool = new KeyedRecyclerPool<>(DEFAULT_NUM_POOLS, DEFAULT_POOL_SIZE);
    }
  }

  /** Returns an adapter suitable for binding the given model. */
  public A get(M model, FrameContext frameContext) {
    statistics.getCalls++;
    A a = recyclingPool.get(keySupplier.getKey(frameContext, model));
    if (a == null) {
      a = keySupplier.getAdapter(context, parameters);
      statistics.adapterCreation++;
    } else {
      statistics.poolHit++;
    }
    return a;
  }

  /** Release the Adapter, releases the model and will recycle the Adapter */
  public void release(A a) {
    statistics.releaseCalls++;
    a.unbindModel();
    a.releaseAdapter();
    RecyclerKey key = a.getKey();
    if (key != null) {
      recyclingPool.put(key, a);
    }
  }

  public void purgeRecyclerPool() {
    recyclingPool.clear();
  }

  /** Basic statistics about hits, creations, etc. used to track how get/release are being used. */
  static class Statistics {
    String factoryName;
    int adapterCreation = 0;
    int poolHit = 0;
    int releaseCalls = 0;
    int getCalls = 0;

    // TODO: Pass in the KeyProvider here
    public Statistics(String factoryName) {
      this.factoryName = factoryName;
    }

    @Override
    public String toString() {
      // String used to show statistics during debugging in Android Studio.
      return "Stats: "
          + factoryName
          + ", Hits:"
          + poolHit
          + ", creations "
          + adapterCreation
          + ", Release: "
          + releaseCalls;
    }
  }
}
