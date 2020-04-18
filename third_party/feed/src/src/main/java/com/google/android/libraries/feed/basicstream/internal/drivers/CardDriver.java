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

import static com.google.android.libraries.feed.common.Validators.checkState;

import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;

/** {@link FeatureDriver} for cards. */
public class CardDriver implements FeatureDriver {
  private static final String TAG = "CardDriver";

  private final ModelFeature cardModel;
  private final ModelProvider modelProvider;
  /*@Nullable*/ private ContentDriver contentDriver;

  public CardDriver(ModelFeature cardModel, ModelProvider modelProvider) {
    this.cardModel = cardModel;
    this.modelProvider = modelProvider;
  }

  @Override
  /*@Nullable*/
  public LeafFeatureDriver getLeafFeatureDriver() {
    if (contentDriver == null) {
      contentDriver = createContentChild(cardModel);
    }

    if (contentDriver != null) {
      return contentDriver.getLeafFeatureDriver();
    }

    return null;
  }

  /*@Nullable*/
  private ContentDriver createContentChild(ModelFeature modelFeature) {
    ModelCursor cursor = modelFeature.getCursor();
    // TODO: add change listener to ModelFeature.
    ModelChild child;
    if ((child = cursor.getNextItem()) != null) {
      ModelFeature contentModel = child.getModelFeature();
      checkState(contentModel.getStreamFeature().hasContent(), "Expected content for feature");
      return createContentDriver(contentModel);
    }

    return null;
  }

  @VisibleForTesting
  ContentDriver createContentDriver(ModelFeature contentModel) {
    return new ContentDriver(contentModel, modelProvider);
  }
}
