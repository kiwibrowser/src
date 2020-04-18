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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelChild.Type;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.testing.modelprovider.FakeModelCursor;
import com.google.common.collect.Lists;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import com.google.search.now.ui.stream.StreamStructureProto.Card;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link ClusterDriver}. */
@RunWith(RobolectricTestRunner.class)
public class ClusterDriverTest {

  private static final StreamFeature CARD_STREAM_FEATURE =
      StreamFeature.newBuilder().setCard(Card.getDefaultInstance()).build();

  @Mock private LeafFeatureDriver leafFeatureDriver;
  @Mock private ModelFeature clusterModelFeature;
  @Mock private ModelProvider modelProvider;

  private ClusterDriverForTest clusterDriver;

  @Before
  public void setup() {
    initMocks(this);

    // Child produced by Cursor representing a card.
    ModelChild cardChild = mock(ModelChild.class);
    when(cardChild.getType()).thenReturn(Type.FEATURE);

    // ModelFeature representing a card.
    ModelFeature cardModelFeature = mock(ModelFeature.class);
    when(cardModelFeature.getStreamFeature()).thenReturn(CARD_STREAM_FEATURE);
    when(cardChild.getModelFeature()).thenReturn(cardModelFeature);

    // Cursor representing the content of the cluster, which is one card.
    FakeModelCursor clusterCursor = new FakeModelCursor(Lists.newArrayList(cardChild));
    when(clusterModelFeature.getCursor()).thenReturn(clusterCursor);

    // CardDriver created by ClusterDriverForTest
    CardDriver cardDriver = mock(CardDriver.class);
    when(cardDriver.getLeafFeatureDriver()).thenReturn(leafFeatureDriver);

    clusterDriver = new ClusterDriverForTest(clusterModelFeature, modelProvider, cardDriver);
  }

  @Test
  public void testGetContentModel() {
    assertThat(clusterDriver.getLeafFeatureDriver()).isEqualTo(leafFeatureDriver);
  }

  @Test
  public void testGetContentModel_reusesPreviousContentModel() {
    clusterDriver.getLeafFeatureDriver();

    verify(clusterModelFeature).getCursor();

    clusterDriver.getLeafFeatureDriver();

    verifyNoMoreInteractions(clusterModelFeature);
  }

  class ClusterDriverForTest extends ClusterDriver {

    private final CardDriver cardDriver;

    ClusterDriverForTest(
        ModelFeature clusterModel, ModelProvider modelProvider, CardDriver cardDriver) {
      super(clusterModel, modelProvider);
      this.cardDriver = cardDriver;
    }

    @Override
    CardDriver createCardDriver(ModelFeature content) {
      return cardDriver;
    }
  }
}
