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

import com.google.android.libraries.feed.api.modelprovider.FeatureChangeObserver;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import java.util.ArrayList;
import java.util.HashSet;

/** Fake for {@link ModelFeature}. */
public class FakeModelFeature implements ModelFeature {

  private final StreamFeature streamFeature;
  private final ModelCursor modelCursor;
  private final HashSet<FeatureChangeObserver> observers = new HashSet<>();

  private FakeModelFeature(ModelCursor modelCursor, StreamFeature streamFeature) {
    this.modelCursor = modelCursor;
    this.streamFeature = streamFeature;
  }

  public HashSet<FeatureChangeObserver> getObservers() {
    return observers;
  }

  @Override
  public StreamFeature getStreamFeature() {
    return streamFeature;
  }

  @Override
  public ModelCursor getCursor() {
    return modelCursor;
  }

  @Override
  public /*@Nullable*/ ModelCursor getDirectionalCursor(
      boolean forward, /*@Nullable*/ String startingChild) {
    return null;
  }

  @Override
  public void registerObserver(FeatureChangeObserver observer) {
    this.observers.add(observer);
  }

  @Override
  public void unregisterObserver(FeatureChangeObserver observer) {
    this.observers.remove(observer);
  }

  public static class Builder {
    private ModelCursor modelCursor = new FakeModelCursor(new ArrayList<>());
    private StreamFeature streamFeature = StreamFeature.getDefaultInstance();

    public Builder setModelCursor(ModelCursor modelCursor) {
      this.modelCursor = modelCursor;
      return this;
    }

    public Builder setStreamFeature(StreamFeature streamFeature) {
      this.streamFeature = streamFeature;
      return this;
    }

    public FakeModelFeature build() {
      return new FakeModelFeature(modelCursor, streamFeature);
    }
  }
}
