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

package com.google.android.libraries.feed.common.testing;

import com.google.protobuf.ByteString;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.DataOperationProto.DataOperation;
import com.google.search.now.wire.feed.DataOperationProto.DataOperation.Operation;
import com.google.search.now.wire.feed.FeatureProto.Feature;
import com.google.search.now.wire.feed.FeatureProto.Feature.RenderableUnit;
import com.google.search.now.wire.feed.FeedResponseProto.FeedResponse;
import com.google.search.now.wire.feed.PayloadMetadataProto.PayloadMetadata;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.SemanticPropertiesProto.SemanticProperties;
import com.google.search.now.wire.feed.TokenProto.Token;
import java.util.ArrayList;
import java.util.List;

/** Builder making creation of wire protocol Response object easier. */
public class WireProtocolResponseBuilder {
  // this needs to match the root content_id defined in the FeedProtocolAdapter.
  public static final ContentId ROOT_CONTENT_ID =
      ContentId.newBuilder().setContentDomain("stream_root").setId(0).setTable("feature").build();
  public static final ContentId PIET_SHARED_STATE =
      ContentId.newBuilder()
          .setContentDomain("piet-shared-state")
          .setId(1)
          .setTable("feature")
          .build();
  private static final int CONTENT_ID_CARD_INCREMENT = 100;
  private static final ContentId TOKEN_CONTENT_ID =
      ContentId.newBuilder().setContentDomain("token").setId(0).setTable("token").build();

  public static ContentId createFeatureContentId(int id) {
    return ContentId.newBuilder().setContentDomain("feature").setId(id).setTable("feature").build();
  }

  private final WireProtocolInfo wireProtocolInfo = new WireProtocolInfo();
  private final FeedResponse.Builder feedResponseBuilder = FeedResponse.newBuilder();

  private int tokenId = 0;
  /*@Nullable*/ private ByteString token = null;

  /** Add a CLEAR_ALL data operation to the response */
  public WireProtocolResponseBuilder addClearOperation() {
    wireProtocolInfo.hasClearOperation = true;
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder().setOperation(Operation.CLEAR_ALL).build());
    return this;
  }

  public WireProtocolResponseBuilder addStreamToken(int tokenId, ByteString token) {
    this.token = token;
    this.tokenId = tokenId;
    wireProtocolInfo.hasToken = true;
    return this;
  }

  public WireProtocolResponseBuilder addPietSharedState() {
    PayloadMetadata payloadMetadata =
        PayloadMetadata.newBuilder().setContentId(PIET_SHARED_STATE).build();
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setPietSharedState(PietSharedState.getDefaultInstance())
            .setMetadata(payloadMetadata)
            .build());
    return this;
  }

  /** Add a data operation for a feature that represents the Root of the Stream. */
  public WireProtocolResponseBuilder addRootFeature() {
    return addRootFeature(ROOT_CONTENT_ID);
  }

  public WireProtocolResponseBuilder addRootFeature(ContentId contentId) {
    PayloadMetadata payloadMetadata = PayloadMetadata.newBuilder().setContentId(contentId).build();
    Feature feature = Feature.newBuilder().setRenderableUnit(RenderableUnit.STREAM).build();

    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(feature)
            .setMetadata(payloadMetadata)
            .build());
    wireProtocolInfo.featuresAdded.add(contentId);
    return this;
  }

  public WireProtocolResponseBuilder addClusterFeature(ContentId contentId, ContentId parentId) {
    PayloadMetadata payloadMetadata = PayloadMetadata.newBuilder().setContentId(contentId).build();
    Feature feature =
        Feature.newBuilder()
            .setRenderableUnit(RenderableUnit.CLUSTER)
            .setParentId(parentId)
            .build();
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(feature)
            .setMetadata(payloadMetadata)
            .build());
    wireProtocolInfo.featuresAdded.add(contentId);
    return this;
  }

  public WireProtocolResponseBuilder addCard(ContentId contentId, ContentId parentId) {
    // Create a Card
    PayloadMetadata payloadMetadata = PayloadMetadata.newBuilder().setContentId(contentId).build();
    Feature feature =
        Feature.newBuilder().setRenderableUnit(RenderableUnit.CARD).setParentId(parentId).build();
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(feature)
            .setMetadata(payloadMetadata)
            .build());
    wireProtocolInfo.featuresAdded.add(contentId);

    // Create content within the Card
    payloadMetadata =
        PayloadMetadata.newBuilder()
            .setContentId(createNewContentId(contentId, CONTENT_ID_CARD_INCREMENT))
            .build();
    feature =
        Feature.newBuilder()
            .setRenderableUnit(RenderableUnit.CONTENT)
            .setParentId(contentId)
            .build();
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(feature)
            .setMetadata(payloadMetadata)
            .build());
    wireProtocolInfo.featuresAdded.add(payloadMetadata.getContentId());
    return this;
  }

  public WireProtocolResponseBuilder addCardWithSemanticData(
      ContentId contentId, ByteString semanticData) {
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(Feature.newBuilder().setRenderableUnit(RenderableUnit.CARD))
            .setMetadata(
                PayloadMetadata.newBuilder()
                    .setContentId(contentId)
                    .setSemanticProperties(
                        SemanticProperties.newBuilder().setSemanticPropertiesData(semanticData))));
    wireProtocolInfo.featuresAdded.add(contentId);
    return this;
  }

  public WireProtocolResponseBuilder removeFeature(ContentId contentId, ContentId parentId) {
    PayloadMetadata payloadMetadata = PayloadMetadata.newBuilder().setContentId(contentId).build();
    Feature feature =
        Feature.newBuilder().setRenderableUnit(RenderableUnit.CARD).setParentId(parentId).build();
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.REMOVE)
            .setFeature(feature)
            .setMetadata(payloadMetadata)
            .build());
    wireProtocolInfo.featuresAdded.add(contentId);
    return this;
  }

  public Response build() {
    if (token != null) {
      addToken(tokenId, token);
    }
    return Response.newBuilder()
        .setExtension(FeedResponse.feedResponse, feedResponseBuilder.build())
        .build();
  }

  private void addToken(int tokenId, ByteString token) {
    PayloadMetadata payloadMetadata =
        PayloadMetadata.newBuilder()
            .setContentId(createNewContentId(TOKEN_CONTENT_ID, tokenId))
            .build();
    Feature.Builder featureBuilder =
        Feature.newBuilder().setRenderableUnit(RenderableUnit.TOKEN).setParentId(ROOT_CONTENT_ID);
    Token wireToken = Token.newBuilder().setNextPageToken(token).build();
    featureBuilder.setExtension(Token.tokenExtension, wireToken);
    feedResponseBuilder.addDataOperation(
        DataOperation.newBuilder()
            .setOperation(Operation.UPDATE_OR_APPEND)
            .setFeature(featureBuilder.build())
            .setMetadata(payloadMetadata)
            .build());
  }

  public WireProtocolInfo getWireProtocolInfo() {
    return wireProtocolInfo;
  }

  /** Captures information about the wire protocol that was created. */
  public static class WireProtocolInfo {
    public boolean hasClearOperation = false;
    public boolean hasToken = false;
    public List<ContentId> featuresAdded = new ArrayList<>();
  }

  private ContentId createNewContentId(ContentId contentId, int idIncrement) {
    ContentId.Builder builder = contentId.toBuilder();
    builder.setId(contentId.getId() + idIncrement);
    return builder.build();
  }
}
