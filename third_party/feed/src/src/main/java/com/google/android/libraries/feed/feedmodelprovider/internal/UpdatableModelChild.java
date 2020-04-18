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

package com.google.android.libraries.feed.feedmodelprovider.internal;

import android.text.TextUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;

/** */
public class UpdatableModelChild implements ModelChild {
  private static final String TAG = "UpdatableModelChild";

  private final String contentId;
  /*@Nullable*/ private final String parentContentId;

  private @Type int type = Type.UNBOUND;
  private UpdatableModelFeature modelFeature;
  private UpdatableModelToken modelToken;

  public UpdatableModelChild(String contentId, /*@Nullable*/ String parentContentId) {
    this.contentId = contentId;
    this.parentContentId = parentContentId;
  }

  void bindFeature(UpdatableModelFeature modelFeature) {
    validateType(Type.UNBOUND);
    this.modelFeature = modelFeature;
    type = Type.FEATURE;
  }

  public void bindToken(UpdatableModelToken modelToken) {
    validateType(Type.UNBOUND);
    this.modelToken = modelToken;
    type = Type.TOKEN;
  }

  @Override
  public ModelFeature getModelFeature() {
    validateType(Type.FEATURE);
    // TODO: We want to prune these once they are accessed by the cursor.  Add support
    // for this.
    return modelFeature;
  }

  @Override
  public ModelToken getModelToken() {
    validateType(Type.TOKEN);
    return modelToken;
  }

  public UpdatableModelToken getUpdatableModelToken() {
    validateType(Type.TOKEN);
    return modelToken;
  }

  void updateFeature(StreamPayload payload) {
    switch (type) {
      case Type.FEATURE:
        if (payload.hasStreamFeature()) {
          modelFeature.setFeatureValue(payload.getStreamFeature());
        } else {
          Logger.e(TAG, "Attempting to update a ModelFeature without providing a feature");
        }
        break;
      case Type.TOKEN:
        Logger.e(TAG, "Update called for TOKEN is unsupported");
        break;
      case Type.UNBOUND:
        Logger.e(TAG, "updateFeature called on UNBOUND child");
        break;
      default:
        Logger.e(TAG, "Update called for unsupported type: %s", type);
    }
  }

  @Override
  public @Type int getType() {
    return type;
  }

  @Override
  public String getContentId() {
    return contentId;
  }

  /*@Nullable*/
  @Override
  public String getParentId() {
    return parentContentId;
  }

  @Override
  public boolean hasParentId() {
    return !TextUtils.isEmpty(parentContentId);
  }

  private void validateType(@Type int type) {
    if (this.type != type) {
      throw new IllegalStateException(
          String.format("ModelChild type error - Type %s, expected %s", this.type, type));
    }
  }
}
