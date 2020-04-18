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

import android.database.Observable;
import com.google.android.libraries.feed.api.modelprovider.FeatureChangeObserver;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import java.util.ArrayList;
import java.util.List;

/** Implementation of the {@link ModelFeature} */
public class UpdatableModelFeature extends Observable<FeatureChangeObserver>
    implements ModelFeature {

  private StreamFeature streamFeature;
  private final CursorProvider cursorProvider;

  UpdatableModelFeature(StreamFeature streamFeature, CursorProvider cursorProvider) {
    this.streamFeature = streamFeature;
    this.cursorProvider = cursorProvider;
  }

  @Override
  public StreamFeature getStreamFeature() {
    return streamFeature;
  }

  @Override
  public ModelCursor getCursor() {
    return cursorProvider.getCursor(streamFeature.getContentId());
  }

  @Override
  /*@Nullable*/
  public ModelCursor getDirectionalCursor(boolean forward, /*@Nullable*/ String startingChild) {
    return null;
  }

  public List<FeatureChangeObserver> getObserversToNotify() {
    synchronized (mObservers) {
      return new ArrayList<>(mObservers);
    }
  }

  void setFeatureValue(StreamFeature streamFeature) {
    this.streamFeature = streamFeature;
  }
}
