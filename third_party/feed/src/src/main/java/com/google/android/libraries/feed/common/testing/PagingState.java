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

import static com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder.ROOT_CONTENT_ID;
import static com.google.common.truth.Truth.assertThat;

import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder.WireProtocolInfo;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.ResponseProto.Response;
import java.nio.charset.Charset;

/**
 * Creates a paging based responses. Create two {@link Response} object, and initial response and a
 * page response. In addition, create a {@link StreamToken} which is used to trigger the paged
 * response.
 */
public class PagingState {
  public final Response initialResponse;
  public final Response pageResponse;
  public final StreamToken streamToken;

  public PagingState(
      ContentId[] initialCards,
      ContentId[] pageCards,
      int tokenId,
      ContentIdGenerators idGenerator) {
    ByteString token = ByteString.copyFrom(Integer.toString(tokenId), Charset.defaultCharset());
    initialResponse = getInitialResponse(initialCards, tokenId, token);
    streamToken = getStreamToken(token, tokenId, idGenerator);
    pageResponse = getPageResponse(pageCards);
  }

  private static Response getInitialResponse(ContentId[] cards, int tokenId, ByteString token) {
    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    for (ContentId contentId : cards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    responseBuilder.addStreamToken(tokenId, token);
    WireProtocolInfo info = responseBuilder.getWireProtocolInfo();
    assertThat(info.hasToken).isTrue();
    return responseBuilder.build();
  }

  private static Response getPageResponse(ContentId[] pagedCards) {
    WireProtocolResponseBuilder pageResponseBuilder = new WireProtocolResponseBuilder();
    for (ContentId contentId : pagedCards) {
      pageResponseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    return pageResponseBuilder.build();
  }

  private static StreamToken getStreamToken(
      ByteString token, int id, ContentIdGenerators idGenerator) {
    ContentId tokenId =
        ContentId.newBuilder().setContentDomain("token").setId(id).setTable("feature").build();

    StreamToken.Builder tokenBuilder = StreamToken.newBuilder();
    // TODO: This needs to be the real root id.  How do we get the root id?
    tokenBuilder.setParentId(idGenerator.createContentId(ROOT_CONTENT_ID));
    tokenBuilder.setContentId(idGenerator.createContentId(tokenId));
    tokenBuilder.setNextPageToken(token);
    return tokenBuilder.build();
  }
}
