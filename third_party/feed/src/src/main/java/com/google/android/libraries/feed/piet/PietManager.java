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
import android.support.annotation.VisibleForTesting;
import android.view.ViewGroup;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;

/** Manages a top-level session of Piet. */
public class PietManager {

  private final AssetProvider assetProvider;
  private final DebugBehavior debugBehavior;
  private final CustomElementProvider customElementProvider;
  private final HostBindingProvider hostBindingProvider;
  @VisibleForTesting /*@Nullable*/ AdapterParameters adapterParameters = null;

  public PietManager(
      DebugBehavior debugBehavior,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider) {
    this.debugBehavior = debugBehavior;
    this.assetProvider = assetProvider;
    this.customElementProvider = customElementProvider;
    this.hostBindingProvider = new HostBindingProvider();
  }

  public PietManager(
      DebugBehavior debugBehavior,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider,
      HostBindingProvider hostBindingProvider) {
    this.debugBehavior = debugBehavior;
    this.assetProvider = assetProvider;
    this.customElementProvider = customElementProvider;
    this.hostBindingProvider = hostBindingProvider;
  }

  public FrameAdapter createPietFrameAdapter(
      Supplier</*@Nullable*/ ViewGroup> cardViewProducer,
      ActionHandler actionHandler,
      Context context) {
    AdapterParameters parameters = getAdapterParameters(context, cardViewProducer);

    return new FrameAdapter(
        context,
        parameters,
        actionHandler,
        debugBehavior,
        assetProvider,
        customElementProvider,
        hostBindingProvider);
  }

  /**
   * Return the {@link AdapterParameters}. If one doesn't exist this will create a new instance. The
   * {@code AdapterParameters} is scoped to the {@code Context}.
   */
  @VisibleForTesting
  AdapterParameters getAdapterParameters(
      Context context, Supplier</*@Nullable*/ ViewGroup> cardViewProducer) {
    if (adapterParameters == null || adapterParameters.context != context) {
      adapterParameters = new AdapterParameters(context, cardViewProducer);
    }
    return adapterParameters;
  }

  public void purgeRecyclerPools() {
    if (adapterParameters != null) {
      adapterParameters.elementAdapterFactory.purgeRecyclerPools();
    }
  }
}
