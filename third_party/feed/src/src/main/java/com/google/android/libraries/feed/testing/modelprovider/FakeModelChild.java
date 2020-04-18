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

package com.google.android.libraries.feed.testing.modelprovider;

import static com.google.android.libraries.feed.common.Validators.checkNotNull;
import static com.google.android.libraries.feed.common.Validators.checkState;

import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;

/** Fake for {@link com.google.android.libraries.feed.api.modelprovider.ModelChild}. */
public class FakeModelChild implements ModelChild {

  private final String contentId;

  private final /*@Nullable*/ ModelFeature modelFeature;
  private final /*@Nullable*/ ModelToken modelToken;
  private final /*@Nullable*/ String parentId;

  private FakeModelChild(
      String contentId,
      /*@Nullable*/ ModelFeature modelFeature,
      /*@Nullable*/ ModelToken modelToken,
      /*@Nullable*/ String parentId) {
    // A ModelChild can't represent both a ModelFeature and a ModelToken.
    checkState(modelFeature == null || modelToken == null);
    this.contentId = contentId;
    this.modelFeature = modelFeature;
    this.modelToken = modelToken;
    this.parentId = parentId;
  }

  @Override
  public @Type int getType() {
    if (modelFeature != null) {
      return Type.FEATURE;
    }

    if (modelToken != null) {
      return Type.TOKEN;
    }

    return Type.UNBOUND;
  }

  @Override
  public String getContentId() {
    return contentId;
  }

  @Override
  public boolean hasParentId() {
    return parentId != null;
  }

  @Override
  public /*@Nullable*/ String getParentId() {
    return parentId;
  }

  @Override
  public ModelFeature getModelFeature() {
    checkState(modelFeature != null, "Must call setModelFeature on builder to have a ModelFeature");

    // checkNotNull for nullness checker, if modelFeature is null, the checkState above will fail.
    return checkNotNull(modelFeature);
  }

  @Override
  public ModelToken getModelToken() {
    checkState(modelToken != null, "Must call setModelToken on builder to have a ModelToken");

    // checkNotNull for nullness checker, if modelToken is null, the checkState above will fail.
    return checkNotNull(modelToken);
  }

  public static class Builder {
    private String contentId = "";
    private /*@Nullable*/ ModelFeature modelFeature;
    private /*@Nullable*/ ModelToken modelToken;
    private /*@Nullable*/ String parentId;

    public Builder setContentId(String contentId) {
      this.contentId = contentId;
      return this;
    }

    public Builder setParentId(/*@Nullable*/ String parentId) {
      this.parentId = parentId;
      return this;
    }

    public Builder setModelFeature(ModelFeature modelFeature) {
      checkState(modelToken == null);
      this.modelFeature = modelFeature;
      return this;
    }

    public Builder setModelToken(ModelToken modelToken) {
      checkState(modelFeature == null);
      this.modelToken = modelToken;
      return this;
    }

    public FakeModelChild build() {
      return new FakeModelChild(contentId, modelFeature, modelToken, parentId);
    }
  }
}
