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

package com.google.android.libraries.feed.feedprotocoladapter;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.ResponseProto.Response;
import java.nio.charset.Charset;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link FeedProtocolAdapter} class. */
@RunWith(RobolectricTestRunner.class)
public class FeedProtocolAdapterTest {

  private final TimingUtils timingUtils = new TimingUtils();
  private WireProtocolResponseBuilder responseBuilder;

  @Before
  public void init() {
    initMocks(this);
    responseBuilder = new WireProtocolResponseBuilder();
  }

  @Test
  public void testConvertContentId() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    ContentId contentId = WireProtocolResponseBuilder.createFeatureContentId(13);
    String streamContentId = protocolAdapter.getStreamContentId(contentId);
    assertThat(streamContentId).isNotNull();
    assertThat(streamContentId).contains(contentId.getContentDomain());
    assertThat(streamContentId).contains(Long.toString(contentId.getId()));
    assertThat(streamContentId).contains(contentId.getTable());
  }

  @Test
  public void testConvertContentId_malformed_notThreeParts() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    String streamContentId = "test" + FeedProtocolAdapter.CONTENT_ID_DELIMITER + "break";
    Result<ContentId> contentIdResult = protocolAdapter.getWireContentId(streamContentId);
    assertThat(contentIdResult.isSuccessful()).isFalse();
  }

  @Test
  public void testConvertContentId_malformed_nonNumericId() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    String streamContentId =
        "test"
            + FeedProtocolAdapter.CONTENT_ID_DELIMITER
            + "break"
            + FeedProtocolAdapter.CONTENT_ID_DELIMITER
            + "test";
    Result<ContentId> contentIdResult = protocolAdapter.getWireContentId(streamContentId);
    assertThat(contentIdResult.isSuccessful()).isFalse();
  }

  @Test
  public void testConvertContentId_roundTrip() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    ContentId contentId = WireProtocolResponseBuilder.createFeatureContentId(13);
    String streamContentId = protocolAdapter.getStreamContentId(contentId);
    Result<ContentId> contentIdResult = protocolAdapter.getWireContentId(streamContentId);
    assertThat(contentIdResult.isSuccessful()).isTrue();
    assertThat(contentIdResult.getValue()).isNotNull();
    assertThat(contentIdResult.getValue()).isEqualTo(contentId);
  }

  @Test
  public void testConvertContentId_roundTrip_partialContentId() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    ContentId contentId = ContentId.newBuilder().setId(13).build();
    String streamContentId = protocolAdapter.getStreamContentId(contentId);
    Result<ContentId> contentIdResult = protocolAdapter.getWireContentId(streamContentId);
    assertThat(contentIdResult.isSuccessful()).isTrue();
    assertThat(contentIdResult.getValue()).isNotNull();
    assertThat(contentIdResult.getValue()).isEqualTo(contentId);
  }

  @Test
  public void testSimpleResponse_clear() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    Response response = responseBuilder.addClearOperation().build();
    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    assertThat(results.getValue()).hasSize(1);
  }

  @Test
  public void testSimpleResponse_feature() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    Response response = responseBuilder.addRootFeature().build();

    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    assertThat(results.getValue()).hasSize(1);

    StreamDataOperation sdo = results.getValue().get(0);
    assertThat(sdo.hasStreamPayload()).isTrue();
    assertThat(sdo.getStreamPayload().hasStreamFeature()).isTrue();
    assertThat(sdo.hasStreamStructure()).isTrue();
    assertThat(sdo.getStreamStructure().hasContentId()).isTrue();
    // Added the root
    assertThat(sdo.getStreamStructure().hasParentContentId()).isFalse();
  }

  @Test
  public void testSimpleResponse_feature_semanticProperties() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    ContentId contentId = WireProtocolResponseBuilder.createFeatureContentId(13);
    ByteString semanticData = ByteString.copyFromUtf8("helloWorld");
    Response response =
        new WireProtocolResponseBuilder().addCardWithSemanticData(contentId, semanticData).build();

    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    // Note that 2 operations are created (the card and the semantic data). We want the latter.
    assertThat(results.getValue()).hasSize(2);
    StreamDataOperation sdo = results.getValue().get(1);
    assertThat(sdo.getStreamPayload().hasSemanticData()).isTrue();
    assertThat(sdo.getStreamPayload().getSemanticData()).isEqualTo(semanticData);
  }

  @Test
  public void testResponse_rootClusterCardContent() {
    ContentId rootId = ContentId.newBuilder().setId(1).build();
    ContentId clusterId = ContentId.newBuilder().setId(2).build();
    ContentId cardId = ContentId.newBuilder().setId(3).build();
    Response response =
        responseBuilder
            .addRootFeature(rootId)
            .addClusterFeature(clusterId, rootId)
            .addCard(cardId, clusterId)
            .build();
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);

    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    List<StreamDataOperation> operations = results.getValue();

    assertThat(operations).hasSize(4);
    assertThat(operations.get(0).getStreamPayload().getStreamFeature().hasStream()).isTrue();
    assertThat(operations.get(1).getStreamPayload().getStreamFeature().hasCluster()).isTrue();
    assertThat(operations.get(2).getStreamPayload().getStreamFeature().hasCard()).isTrue();
    assertThat(operations.get(3).getStreamPayload().getStreamFeature().hasContent()).isTrue();
  }

  @Test
  public void testResponse_remove() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    Response response =
        responseBuilder
            .removeFeature(ContentId.getDefaultInstance(), ContentId.getDefaultInstance())
            .build();
    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    assertThat(results.getValue()).hasSize(1);
  }

  @Test
  public void testPietSharedState() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    Response response = responseBuilder.addPietSharedState().build();
    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    assertThat(results.getValue()).hasSize(1);
    StreamDataOperation sdo = results.getValue().get(0);
    assertThat(sdo.hasStreamPayload()).isTrue();
    assertThat(sdo.getStreamPayload().hasStreamSharedState()).isTrue();
    assertThat(sdo.hasStreamStructure()).isTrue();
    assertThat(sdo.getStreamStructure().hasContentId()).isTrue();
  }

  @Test
  public void testContinuationToken_nextPageToken() {
    FeedProtocolAdapter protocolAdapter = new FeedProtocolAdapter(timingUtils);
    ByteString tokenForMutation = ByteString.copyFrom("token", Charset.defaultCharset());
    Response response = responseBuilder.addStreamToken(1, tokenForMutation).build();

    Result<List<StreamDataOperation>> results = protocolAdapter.createModel(response);
    assertThat(results.isSuccessful()).isTrue();
    assertThat(results.getValue()).hasSize(1);
    StreamDataOperation sdo = results.getValue().get(0);
    assertThat(sdo.hasStreamPayload()).isTrue();
    assertThat(sdo.getStreamPayload().hasStreamToken()).isTrue();
  }
}
