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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.api.modelprovider.TokenCompleted;
import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.android.libraries.feed.basicstream.internal.drivers.StreamDriver.StreamContentListener;
import com.google.android.libraries.feed.basicstream.internal.drivers.testing.FakeFeatureDriver;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelChild;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelCursor;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelFeature;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelToken;
import com.google.common.collect.Lists;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import com.google.search.now.ui.stream.StreamStructureProto.Card;
import com.google.search.now.ui.stream.StreamStructureProto.Cluster;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link StreamDriver}. */
@RunWith(RobolectricTestRunner.class)
public class StreamDriverTest {

  private StreamDriverForTest streamDriver;

  @Mock private ModelFeature streamFeature;
  @Mock private ModelProvider modelProvider;
  @Mock private ContinuationDriver continuationDriver;
  @Mock private StreamContentListener contentListener;
  private Configuration configuration = new Configuration.Builder().build();

  @Before
  public void setup() {
    initMocks(this);
    ThreadUtils threadUtils = new ThreadUtils();

    when(continuationDriver.getLeafFeatureDriver()).thenReturn(continuationDriver);
    when(modelProvider.getRootFeature()).thenReturn(streamFeature);

    streamDriver = new StreamDriverForTest(modelProvider, threadUtils);

    streamDriver.setStreamContentListener(contentListener);
  }

  @Test
  public void testBuildChildren() {
    when(streamFeature.getCursor())
        .thenReturn(new FakeCursorBuilder().addCard().addCluster().addToken().build());

    // Causes StreamDriver to build a list of children based on the children from the cursor.
    List<LeafFeatureDriver> leafFeatureDrivers = streamDriver.getLeafFeatureDrivers();

    assertThat(leafFeatureDrivers).hasSize(3);

    assertThat(leafFeatureDrivers.get(0)).isEqualTo(getLeafFeatureDriverFromCard(0));
    assertThat(leafFeatureDrivers.get(1)).isEqualTo(getLeafFeatureDriverFromCluster(1));
    assertThat(leafFeatureDrivers.get(2)).isEqualTo(continuationDriver);
  }

  @Test
  public void testContinuationToken_createsContinuationContentModel() {
    when(streamFeature.getCursor()).thenReturn(new FakeCursorBuilder().addToken().build());

    List<LeafFeatureDriver> leafFeatureDrivers = streamDriver.getLeafFeatureDrivers();
    assertThat(leafFeatureDrivers).hasSize(1);
    assertThat(leafFeatureDrivers.get(0)).isEqualTo(continuationDriver);
  }

  @Test
  public void testContinuationToken_tokenHandling() {
    List<LeafFeatureDriver> finalContentModels =
        new TokenHandlingTestBuilder()
            .setInitialCursor(new FakeCursorBuilder().addToken().build())
            .setTokenPayloadCursor(new FakeCursorBuilder().addCluster().build())
            .setTokenIndex(0)
            .run();

    assertThat(finalContentModels).hasSize(1);
    assertThat(finalContentModels.get(0)).isEqualTo(getLeafFeatureDriverFromCluster(1));

    // If the above two assertions pass, this is also guaranteed to pass. This is just to explicitly
    // check that the ContinuationDriver has been removed.
    assertThat(finalContentModels).doesNotContain(continuationDriver);
  }

  @Test
  public void testContinuationToken_tokenHandling_notifiesObservers() {
    new TokenHandlingTestBuilder()
        .setInitialCursor(new FakeCursorBuilder().addToken().build())
        .setTokenPayloadCursor(new FakeCursorBuilder().addCluster().build())
        .setTokenIndex(0)
        .run();

    verify(contentListener).notifyContentRemoved(0);
    verify(contentListener)
        .notifyContentsAdded(0, Lists.newArrayList(getLeafFeatureDriverFromCluster(1)));
  }

  @Test
  public void testContinuationToken_tokenChildrenAddedAtTokenPosition() {
    List<LeafFeatureDriver> finalContentModels =
        new TokenHandlingTestBuilder()
            .setInitialCursor(new FakeCursorBuilder().addCluster().addToken().build())
            .setTokenPayloadCursor(new FakeCursorBuilder().addCluster().addToken().build())
            .setTokenIndex(1)
            .run();

    assertThat(finalContentModels).hasSize(3);
    assertThat(finalContentModels)
        .containsExactly(
            getLeafFeatureDriverFromCluster(0),
            getLeafFeatureDriverFromCluster(2),
            continuationDriver);
  }

  @Test
  public void testContinuationToken_tokenChildrenAddedAtTokenPosition_tokenNotAtEnd() {
    List<LeafFeatureDriver> finalContentModels =
        new TokenHandlingTestBuilder()
            .setInitialCursor(new FakeCursorBuilder().addCluster().addToken().addCluster().build())
            .setTokenIndex(1)
            .setTokenPayloadCursor(
                new FakeCursorBuilder().addCluster().addCard().addCluster().build())
            .run();

    assertThat(finalContentModels).hasSize(5);
    assertThat(finalContentModels)
        .containsExactly(
            getLeafFeatureDriverFromCluster(0),
            getLeafFeatureDriverFromCluster(2),
            getLeafFeatureDriverFromCluster(3),
            getLeafFeatureDriverFromCard(4),
            getLeafFeatureDriverFromCluster(5));
  }

  @Test
  public void testOnChange_remove() {

    FakeModelCursor fakeModelCursor =
        new FakeCursorBuilder().addCard().addCard().addCard().addCard().build();

    when(streamFeature.getCursor()).thenReturn(fakeModelCursor);

    List<LeafFeatureDriver> leafFeatureDrivers = streamDriver.getLeafFeatureDrivers();

    assertThat(leafFeatureDrivers).hasSize(4);

    streamDriver.onChange(
        new ChildRemoveBuilder()
            .addChildForRemoval(fakeModelCursor.getChildAt(1))
            .addChildForRemoval(fakeModelCursor.getChildAt(2))
            .build());

    assertThat(streamDriver.getLeafFeatureDrivers())
        .containsExactly(leafFeatureDrivers.get(0), leafFeatureDrivers.get(3));
  }

  @Test
  public void testOnChange_remove_notifiesListener() {
    FakeModelCursor fakeModelCursor = new FakeCursorBuilder().addCard().addCard().build();

    when(streamFeature.getCursor()).thenReturn(fakeModelCursor);

    // Causes StreamDriver to build a list of children based on the children from the cursor.
    streamDriver.getLeafFeatureDrivers();

    streamDriver.onChange(
        new ChildRemoveBuilder().addChildForRemoval(fakeModelCursor.getChildAt(0)).build());

    verify(contentListener).notifyContentRemoved(0);
    verifyNoMoreInteractions(contentListener);
  }

  // TODO: Instead of just checking that the ModelFeature is of the correct type, check
  // that it is the one created by the FakeCursorBuilder.
  private LeafFeatureDriver getLeafFeatureDriverFromCard(int i) {
    FakeFeatureDriver featureDriver = (FakeFeatureDriver) streamDriver.childrenCreated.get(i);
    assertThat(featureDriver.getModelFeature().getStreamFeature().hasCard()).isTrue();
    return streamDriver.childrenCreated.get(i).getLeafFeatureDriver();
  }

  // TODO: Instead of just checking that the ModelFeature is of the correct type, check
  // that it is the one created by the FakeCursorBuilder.
  private LeafFeatureDriver getLeafFeatureDriverFromCluster(int i) {
    FakeFeatureDriver featureDriver = (FakeFeatureDriver) streamDriver.childrenCreated.get(i);
    assertThat(featureDriver.getModelFeature().getStreamFeature().hasCluster()).isTrue();
    return streamDriver.childrenCreated.get(i).getLeafFeatureDriver();
  }

  private class StreamDriverForTest extends StreamDriver {
    // TODO: create a fake for ContinuationDriver so that this can be
    // List<FakeFeatureDriver>
    private List<FeatureDriver> childrenCreated;

    StreamDriverForTest(ModelProvider modelProvider, ThreadUtils threadUtils) {
      super(modelProvider, threadUtils, configuration);
      childrenCreated = new ArrayList<>();
    }

    @Override
    ContinuationDriver createContinuationDriver(
        ModelProvider modelProvider, ModelToken modelToken, Configuration configuration) {
      childrenCreated.add(continuationDriver);
      return continuationDriver;
    }

    @Override
    FeatureDriver createClusterDriver(ModelFeature modelFeature) {
      FeatureDriver featureDriver =
          new FakeFeatureDriver.Builder().setModelFeature(modelFeature).build();
      childrenCreated.add(featureDriver);
      return featureDriver;
    }

    @Override
    FeatureDriver createCardDriver(ModelFeature modelFeature) {
      FeatureDriver featureDriver =
          new FakeFeatureDriver.Builder().setModelFeature(modelFeature).build();
      childrenCreated.add(featureDriver);
      return featureDriver;
    }
  }

  /** Sets up a {@link FakeModelCursor}. */
  private static class FakeCursorBuilder {

    List<ModelChild> cursorChildren = new ArrayList<>();

    FakeCursorBuilder addCard() {
      ModelChild cardChild =
          new FakeModelChild.Builder()
              .setModelFeature(
                  new FakeModelFeature.Builder()
                      .setStreamFeature(
                          StreamFeature.newBuilder().setCard(Card.getDefaultInstance()).build())
                      .build())
              .build();

      cursorChildren.add(cardChild);

      return this;
    }

    FakeCursorBuilder addCluster() {
      ModelChild clusterChild =
          new FakeModelChild.Builder()
              .setModelFeature(
                  new FakeModelFeature.Builder()
                      .setStreamFeature(
                          StreamFeature.newBuilder()
                              .setCluster(Cluster.getDefaultInstance())
                              .build())
                      .build())
              .build();

      cursorChildren.add(clusterChild);

      return this;
    }

    FakeCursorBuilder addToken() {
      ModelChild tokenChild =
          new FakeModelChild.Builder().setModelToken(new FakeModelToken.Builder().build()).build();

      cursorChildren.add(tokenChild);

      return this;
    }

    public FakeModelCursor build() {
      return new FakeModelCursor(cursorChildren);
    }
  }

  /**
   * Sets up the interactions between the {@link StreamDriver} and {@link ModelProvider} to handle a
   * cursor that has one {@link ModelToken}. Then simulates the clicking of the {@link ModelToken}.
   */
  private class TokenHandlingTestBuilder {

    private FakeModelCursor initialCursor;
    private FakeModelCursor payloadCursor;
    private int tokenIndex;

    TokenHandlingTestBuilder setInitialCursor(FakeModelCursor initialCursor) {
      this.initialCursor = initialCursor;
      return this;
    }

    TokenHandlingTestBuilder setTokenPayloadCursor(FakeModelCursor initialCursor) {
      this.payloadCursor = initialCursor;
      return this;
    }

    TokenHandlingTestBuilder setTokenIndex(int tokenIndex) {
      this.tokenIndex = tokenIndex;
      return this;
    }

    List<LeafFeatureDriver> run() {
      when(streamFeature.getCursor()).thenReturn(initialCursor);

      // Causes StreamDriver to build a list of children based on the children from the cursor.
      streamDriver.getLeafFeatureDrivers();

      FakeModelToken token = (FakeModelToken) initialCursor.getChildAt(tokenIndex).getModelToken();

      HashSet<TokenCompletedObserver> tokenCompletedObservers = token.getObservers();

      assertThat(tokenCompletedObservers).hasSize(1);

      for (TokenCompletedObserver tokenCompletedObserver : tokenCompletedObservers) {
        tokenCompletedObserver.onTokenCompleted(new TokenCompleted(payloadCursor));
      }

      return streamDriver.getLeafFeatureDrivers();
    }
  }

  /**
   * Builds a {@link FeatureChange} representing the removal of given {@link FakeFeatureDriver}
   * instances.
   */
  private static class ChildRemoveBuilder {

    private List<ModelChild> removedChildren = new ArrayList<>();

    ChildRemoveBuilder addChildForRemoval(ModelChild modelChild) {
      removedChildren.add(modelChild);
      return this;
    }

    FeatureChange build() {
      return new FeatureChange() {
        @Override
        public String getContentId() {
          return null;
        }

        @Override
        public boolean isFeatureChanged() {
          return false;
        }

        @Override
        public ModelFeature getModelFeature() {
          return null;
        }

        @Override
        public ChildChanges getChildChanges() {
          return new ChildChanges() {
            @Override
            public List<ModelChild> getAppendedChildren() {
              return null;
            }

            @Override
            public List<ModelChild> getRemovedChildren() {
              return removedChildren;
            }
          };
        }
      };
    }
  }
}
