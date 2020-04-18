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

package com.google.android.libraries.feed.basicstream.internal.drivers;

import static com.google.android.libraries.feed.common.Validators.checkNotNull;
import static com.google.android.libraries.feed.common.Validators.checkState;

import android.support.annotation.VisibleForTesting;
import android.view.View;
import android.view.View.OnClickListener;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ContinuationViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;

/** {@link FeatureDriver} for the more button. */
public class ContinuationDriver extends LeafFeatureDriver implements OnClickListener {

  private static final String TAG = "ContinuationDriver";
  private final ModelToken modelToken;
  private final ModelProvider modelProvider;
  private boolean showSpinner;
  /*@Nullable*/ private ContinuationViewHolder continuationViewHolder;

  ContinuationDriver(
      ModelProvider modelProvider, ModelToken modelToken, Configuration configuration) {
    this.modelProvider = modelProvider;
    this.modelToken = modelToken;
    this.showSpinner =
        configuration.getValueOrDefault(ConfigKey.TRIGGER_IMMEDIATE_PAGINATION, false);
  }

  // TODO: Instead of implementing an onClickListener, define a new interface with a method
  // with no view argument.
  @Override
  public void onClick(View v) {
    if (!isBound()) {
      Logger.wtf(TAG, "Calling onClick before binding.");
      return;
    }
    showSpinner = true;
    checkNotNull(continuationViewHolder).setShowSpinner(true);
    modelProvider.handleToken(modelToken);
  }

  @Override
  public void bind(FeedViewHolder viewHolder) {
    if (isBound()) {
      Logger.wtf(TAG, "Rebinding.");
    }
    checkState(viewHolder instanceof ContinuationViewHolder);

    ((ContinuationViewHolder) viewHolder).bind(this, showSpinner);
    continuationViewHolder = (ContinuationViewHolder) viewHolder;
  }

  @Override
  public int getItemViewType() {
    return ViewHolderType.TYPE_CONTINUATION;
  }

  @Override
  public void unbind() {
    if (continuationViewHolder == null) {
      return;
    }

    continuationViewHolder.unbind();
    continuationViewHolder = null;
  }

  @VisibleForTesting
  boolean isBound() {
    return continuationViewHolder != null;
  }

}
