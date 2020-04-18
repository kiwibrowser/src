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

package com.google.android.libraries.feed.basicstream.internal.drivers.testing;

import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.basicstream.internal.drivers.FeatureDriver;
import com.google.android.libraries.feed.basicstream.internal.drivers.LeafFeatureDriver;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelFeature;

/** Fake for {@link FeatureDriver}. */
public class FakeFeatureDriver implements FeatureDriver {
  /*@Nullable*/ private final LeafFeatureDriver leafFeatureDriver;
  private final ModelFeature modelFeature;

  private FakeFeatureDriver(
      /*@Nullable*/ LeafFeatureDriver leafFeatureDriver, ModelFeature modelFeature) {
    this.leafFeatureDriver = leafFeatureDriver;
    this.modelFeature = modelFeature;
  }

  @Override
  /*@Nullable*/
  public LeafFeatureDriver getLeafFeatureDriver() {
    return leafFeatureDriver;
  }

  public ModelFeature getModelFeature() {
    return modelFeature;
  }

  public static class Builder {
    /*@Nullable*/
    private LeafFeatureDriver leafFeatureDriver = new FakeLeafFeatureDriver.Builder().build();

    private ModelFeature modelFeature = new FakeModelFeature.Builder().build();

    public Builder setLeafFeatureDriver(/*@Nullable*/ LeafFeatureDriver contentModel) {
      this.leafFeatureDriver = contentModel;
      return this;
    }

    public Builder setModelFeature(ModelFeature modelFeature) {
      this.modelFeature = modelFeature;
      return this;
    }

    public FakeFeatureDriver build() {
      return new FakeFeatureDriver(leafFeatureDriver, modelFeature);
    }
  }
}
