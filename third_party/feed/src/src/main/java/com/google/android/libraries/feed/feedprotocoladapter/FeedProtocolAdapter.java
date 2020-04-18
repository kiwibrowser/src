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

import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.Validators;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure.Operation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.ui.stream.StreamStructureProto.Card;
import com.google.search.now.ui.stream.StreamStructureProto.Cluster;
import com.google.search.now.ui.stream.StreamStructureProto.Content;
import com.google.search.now.ui.stream.StreamStructureProto.Stream;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.ContentIdProto.ContentId.Builder;
import com.google.search.now.wire.feed.DataOperationProto.DataOperation;
import com.google.search.now.wire.feed.FeatureProto.Feature;
import com.google.search.now.wire.feed.FeatureProto.Feature.RenderableUnit;
import com.google.search.now.wire.feed.FeedResponseProto.FeedResponse;
import com.google.search.now.wire.feed.PietSharedStateItemProto.PietSharedStateItem;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.TokenProto.Token;
import java.util.ArrayList;
import java.util.List;

/** A ProtocolAdapter which converts from the wire protocol to the internal protocol. */
public class FeedProtocolAdapter implements ProtocolAdapter, Dumpable {

  private static final String TAG = "FeedProtocolAdapter";
  public static final String CONTENT_ID_DELIMITER = "::";

  private final TimingUtils timingUtils;

  // Operation counts for #dump(Dumpable)
  private int responseHandlingCount = 0;
  private int convertContentIdCount = 0;

  public FeedProtocolAdapter(TimingUtils timingUtils) {
    this.timingUtils = timingUtils;
  }

  @Override
  public String getStreamContentId(ContentId contentId) {
    convertContentIdCount++;
    return createContentId(contentId);
  }

  @Override
  public Result<ContentId> getWireContentId(String contentId) {
    String[] splitContentId = contentId.split(CONTENT_ID_DELIMITER, -1);
    // Can't create if all 3 parts aren't present (at the very least empty)
    if (splitContentId.length != 3) {
      Logger.e(TAG, "Error parsing string content ID - splitting did not result in 3 parts");
      return Result.failure();
    }
    String table = splitContentId[0];
    String contentDomain = splitContentId[1];
    long id;
    try {
      id = Long.parseLong(splitContentId[2]);
    } catch (NumberFormatException e) {
      Logger.e(TAG, e, "Error converting content ID to wire format");
      return Result.failure();
    }
    Builder builder = ContentId.newBuilder().setId(id);
    if (!table.isEmpty()) {
      builder.setTable(table);
    }
    if (!contentDomain.isEmpty()) {
      builder.setContentDomain(contentDomain);
    }
    return Result.success(builder.build());
  }

  @Override
  public Result<List<StreamDataOperation>> createModel(Response response) {
    responseHandlingCount++;

    Result<List<StreamDataOperation>> result =
        createOperations(response.getExtension(FeedResponse.feedResponse).getDataOperationList());
    if (!result.isSuccessful()) {
      return Result.failure();
    }
    List<StreamDataOperation> streamDataOperations = result.getValue();
    return Result.success(streamDataOperations);
  }

  @Override
  public Result<List<StreamDataOperation>> createOperations(List<DataOperation> dataOperations) {
    ElapsedTimeTracker totalTimeTracker = timingUtils.getElapsedTimeTracker(TAG);
    List<StreamDataOperation> streamDataOperations = new ArrayList<>();
    for (DataOperation operation : dataOperations) {
      // The operations defined in stream_data.proto and data_operation.proto have the same
      // integer value
      Operation streamOperation = Operation.forNumber(operation.getOperation().getNumber());
      String contentId;
      if (streamOperation == Operation.CLEAR_ALL) {
        streamDataOperations.add(createDataOperation(Operation.CLEAR_ALL, null, null).build());
        continue;
      } else if (streamOperation == Operation.REMOVE) {
        if (operation.getMetadata().hasContentId()) {
          contentId = createContentId(operation.getMetadata().getContentId());
          String parentId = null;
          if (operation.getFeature().hasParentId()) {
            parentId = createContentId(operation.getFeature().getParentId());
          }
          streamDataOperations.add(
              createDataOperation(Operation.REMOVE, contentId, parentId).build());

        } else {
          Logger.w(TAG, "REMOVE defined without a ContentId identifying the item to remove");
        }
        continue;
      } else if (operation.getMetadata().hasContentId()) {
        contentId = createContentId(operation.getMetadata().getContentId());
      } else {
        // This is an error state, every card should have a content id
        Logger.e(TAG, "ContentId not defined for DataOperation");
        continue;
      }

      if (operation.hasFeature()) {
        handleFeatureOperation(operation, contentId, streamDataOperations);
      } else if (operation.hasPietSharedState()) {
        PietSharedStateItem item =
            PietSharedStateItem.newBuilder()
                .setPietSharedState(operation.getPietSharedState())
                .build();
        StreamSharedState state =
            StreamSharedState.newBuilder()
                .setPietSharedStateItem(item)
                .setContentId(contentId)
                .build();
        streamDataOperations.add(
            createSharedStateDataOperation(streamOperation, contentId, state).build());
      }

      if (operation.getMetadata().getSemanticProperties().hasSemanticPropertiesData()) {
        streamDataOperations.add(
            createSemanticDataOperation(
                    contentId,
                    operation.getMetadata().getSemanticProperties().getSemanticPropertiesData())
                .build());
      }
    }
    totalTimeTracker.stop("", "convertWireProtocol", "mutations", dataOperations.size());
    return Result.success(streamDataOperations);
  }

  private void handleFeatureOperation(
      DataOperation operation, String contentId, List<StreamDataOperation> streamDataOperations) {
    Operation streamOperation = Operation.forNumber(operation.getOperation().getNumber());
    StreamFeature.Builder streamFeature = StreamFeature.newBuilder();
    streamFeature.setContentId(contentId);
    String parentId = null;
    if (operation.getFeature().hasParentId()) {
      parentId = createContentId(operation.getFeature().getParentId());
      streamFeature.setParentId(parentId);
    }
    if (operation.getFeature().getRenderableUnit() == RenderableUnit.TOKEN) {
      Feature feature = operation.getFeature();
      if (feature.hasExtension(Token.tokenExtension)) {
        // Create a StreamToken operation
        Token token = feature.getExtension(Token.tokenExtension);
        StreamToken streamToken =
            Validators.checkNotNull(
                createStreamToken(contentId, parentId, token.getNextPageToken()));
        streamDataOperations.add(
            createTokenDataOperation(contentId, streamToken.getParentId(), streamToken).build());
      } else {
        Logger.e(TAG, "Extension not found for TOKEN");
      }
    } else {
      // Create a StreamFeature operation
      switch (operation.getFeature().getRenderableUnit()) {
        case STREAM:
          streamFeature.setStream(operation.getFeature().getExtension(Stream.streamExtension));
          break;
        case CARD:
          streamFeature.setCard(operation.getFeature().getExtension(Card.cardExtension));
          break;
        case CONTENT:
          streamFeature.setContent(operation.getFeature().getExtension(Content.contentExtension));
          break;
        case CLUSTER:
          streamFeature.setCluster(operation.getFeature().getExtension(Cluster.clusterExtension));
          break;
        case TOKEN:
          // handled above...
        default:
          // TODO: This should be handled and we should return a result.
          Logger.e(TAG, "Unknown Feature payload %s", operation.getFeature().getRenderableUnit());
          throw new RuntimeException("Unknown feature payload");
      }
      streamDataOperations.add(
          createFeatureDataOperation(streamOperation, contentId, parentId, streamFeature.build())
              .build());
    }
  }

  /*@Nullable*/
  private StreamToken createStreamToken(
      String tokenId, /*@Nullable*/ String parentId, ByteString continuationToken) {
    if (continuationToken.isEmpty()) {
      return null;
    }
    StreamToken.Builder tokenBuilder = StreamToken.newBuilder();
    if (parentId != null) {
      tokenBuilder.setParentId(parentId);
    }
    tokenBuilder.setContentId(tokenId);
    tokenBuilder.setNextPageToken(continuationToken);
    return tokenBuilder.build();
  }

  private StreamDataOperation.Builder createFeatureDataOperation(
      Operation operation,
      /*@Nullable*/ String contentId,
      /*@Nullable*/ String parentId,
      StreamFeature streamFeature) {
    StreamDataOperation.Builder dataOperation = createDataOperation(operation, contentId, parentId);
    dataOperation.setStreamPayload(StreamPayload.newBuilder().setStreamFeature(streamFeature));
    return dataOperation;
  }

  private StreamDataOperation.Builder createSharedStateDataOperation(
      Operation operation, /*@Nullable*/ String contentId, StreamSharedState sharedState) {
    StreamDataOperation.Builder dataOperation = createDataOperation(operation, contentId, null);
    dataOperation.setStreamPayload(StreamPayload.newBuilder().setStreamSharedState(sharedState));
    return dataOperation;
  }

  private StreamDataOperation.Builder createTokenDataOperation(
      /*@Nullable*/ String contentId, /*@Nullable*/ String parentId, StreamToken streamToken) {
    StreamDataOperation.Builder dataOperation =
        createDataOperation(Operation.UPDATE_OR_APPEND, contentId, parentId);
    dataOperation.setStreamPayload(StreamPayload.newBuilder().setStreamToken(streamToken));
    return dataOperation;
  }

  private StreamDataOperation.Builder createDataOperation(
      Operation operation, /*@Nullable*/ String contentId, /*@Nullable*/ String parentId) {
    StreamDataOperation.Builder streamDataOperation = StreamDataOperation.newBuilder();
    StreamStructure.Builder streamStructure = StreamStructure.newBuilder();
    streamStructure.setOperation(operation);
    if (contentId != null) {
      streamStructure.setContentId(contentId);
    }
    if (parentId != null) {
      streamStructure.setParentContentId(parentId);
    }
    streamDataOperation.setStreamStructure(streamStructure);
    return streamDataOperation;
  }

  private StreamDataOperation.Builder createSemanticDataOperation(
      String contentId, ByteString semanticData) {
    return StreamDataOperation.newBuilder()
        .setStreamPayload(StreamPayload.newBuilder().setSemanticData(semanticData))
        .setStreamStructure(StreamStructure.newBuilder().setContentId(contentId));
  }

  private String createContentId(ContentId contentId) {
    // Using String concat for performance reasons.  This is called a lot for large feed responses.
    return contentId.getTable()
        + CONTENT_ID_DELIMITER
        + contentId.getContentDomain()
        + CONTENT_ID_DELIMITER
        + contentId.getId();
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("responseHandlingCount").value(responseHandlingCount);
    dumper.forKey("convertContentIdCount").value(convertContentIdCount).compactPrevious();
  }
}
