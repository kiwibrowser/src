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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.common.functional.Committer;
import com.google.android.libraries.feed.feedmodelprovider.internal.ModelMutationImpl.Change;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure.Operation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of {@link ModelMutationImpl}. */
@RunWith(RobolectricTestRunner.class)
public class ModelMutationImplTest {
  @Mock private Committer<Void, Change> committer;

  @Before
  public void setup() {
    initMocks(this);
  }

  @Test
  public void testFeature() {
    ModelMutationImpl modelMutator = new ModelMutationImpl(committer);
    StreamFeature streamFeature = StreamFeature.newBuilder().build();
    modelMutator.addChild(createStreamStructureFromFeature(streamFeature));
    assertThat(modelMutator.change.structureChanges).hasSize(1);
    modelMutator.commit();
    verify(committer).commit(modelMutator.change);
  }

  @Test
  public void testToken() {
    ModelMutationImpl modelMutator = new ModelMutationImpl(committer);
    StreamToken streamToken = StreamToken.newBuilder().build();
    modelMutator.addChild(createStreamStructureFromToken(streamToken));
    assertThat(modelMutator.change.structureChanges).hasSize(1);
    modelMutator.commit();
    verify(committer).commit(modelMutator.change);
  }

  @Test
  public void testTokenForMutation() {
    ModelMutationImpl modelMutator = new ModelMutationImpl(committer);
    StreamToken streamToken = StreamToken.newBuilder().build();
    modelMutator.setMutationSourceToken(streamToken);
    assertThat(modelMutator.change.structureChanges).isEmpty();
    modelMutator.commit();
    verify(committer).commit(modelMutator.change);
  }

  @Test
  public void testRemove() {
    ModelMutationImpl modelMutator = new ModelMutationImpl(committer);
    assertThat(modelMutator.change.structureChanges).isEmpty();
    modelMutator.removeChild(StreamStructure.getDefaultInstance());
    assertThat(modelMutator.change.structureChanges).hasSize(1);
  }

  private StreamStructure createStreamStructureFromFeature(StreamFeature feature) {
    StreamStructure.Builder builder =
        StreamStructure.newBuilder()
            .setContentId(feature.getContentId())
            .setOperation(Operation.UPDATE_OR_APPEND);
    if (feature.hasParentId()) {
      builder.setParentContentId(feature.getParentId());
    }
    return builder.build();
  }

  private StreamStructure createStreamStructureFromToken(StreamToken token) {
    StreamStructure.Builder builder =
        StreamStructure.newBuilder()
            .setContentId(token.getContentId())
            .setOperation(Operation.UPDATE_OR_APPEND);
    if (token.hasParentId()) {
      builder.setParentContentId(token.getParentId());
    }
    return builder.build();
  }
}
