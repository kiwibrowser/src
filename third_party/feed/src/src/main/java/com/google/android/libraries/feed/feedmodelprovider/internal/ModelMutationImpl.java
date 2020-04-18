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

import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.modelprovider.ModelMutation;
import com.google.android.libraries.feed.common.functional.Committer;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.ArrayList;
import java.util.List;

/**
 * Implementation of the {@link ModelMutation}. When the mutation is committed updating the Model
 */
public class ModelMutationImpl implements ModelMutation {

  /** The data representing the Change to the model. */
  public static class Change {
    public final List<StreamStructure> structureChanges = new ArrayList<>();
    public final List<StreamStructure> updateChanges = new ArrayList<>();
    public StreamToken mutationSourceToken;
    public StreamSession streamSession;
    public boolean cachedBindings;
  }

  private final Committer<Void, Change> committer;
  @VisibleForTesting final Change change = new Change();

  public ModelMutationImpl(Committer<Void, Change> committer) {
    this.committer = committer;
  }

  @Override
  public ModelMutation addChild(StreamStructure streamStructure) {
    change.structureChanges.add(streamStructure);
    return this;
  }

  @Override
  public ModelMutation removeChild(StreamStructure streamStructure) {
    change.structureChanges.add(streamStructure);
    return this;
  }

  @Override
  public ModelMutation updateChild(StreamStructure updateChild) {
    change.updateChanges.add(updateChild);
    return this;
  }

  @Override
  public ModelMutation setMutationSourceToken(StreamToken mutationSourceToken) {
    change.mutationSourceToken = mutationSourceToken;
    return this;
  }

  @Override
  public ModelMutation setStreamSession(StreamSession streamSession) {
    change.streamSession = streamSession;
    return this;
  }

  @Override
  public ModelMutation hasCachedBindings(boolean cachedBindings) {
    change.cachedBindings = cachedBindings;
    return this;
  }

  @Override
  public void commit() {
    committer.commit(change);
  }
}
