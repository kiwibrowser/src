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

package com.google.android.libraries.feed.common.protoextensions;

import com.google.android.libraries.feed.host.proto.ProtoExtensionProvider;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.protobuf.GeneratedMessageLite.GeneratedExtension;
import com.google.search.now.ui.action.FeedActionProto.FeedAction;
import com.google.search.now.ui.action.PietExtensionsProto.PietFeedActionPayload;
import com.google.search.now.ui.stream.StreamStructureProto;
import com.google.search.now.ui.stream.StreamStructureProto.Card;
import com.google.search.now.ui.stream.StreamStructureProto.Content;
import com.google.search.now.ui.stream.StreamStructureProto.PietContent;
import com.google.search.now.wire.feed.FeedRequestProto.FeedRequest;
import com.google.search.now.wire.feed.FeedResponseProto.FeedResponse;
import com.google.search.now.wire.feed.TokenProto;

/**
 * Creates and initializes the proto extension registry, adding feed-internal extensions as well as
 * those provided by the host through the {@link ProtoExtensionProvider}.
 */
public class FeedExtensionRegistry {
  private final ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();

  /**
   * Creates the registry.
   *
   * <p>TODO: Move this initialization code into Feed initialization, once that exists.
   */
  public FeedExtensionRegistry(ProtoExtensionProvider extensionProvider) {
    // Set up all the extensions we use inside the Feed.
    extensionRegistry.add(Card.cardExtension);
    extensionRegistry.add(Content.contentExtension);
    extensionRegistry.add(FeedAction.feedActionExtension);
    extensionRegistry.add(FeedRequest.feedRequest);
    extensionRegistry.add(FeedResponse.feedResponse);
    extensionRegistry.add(PietContent.pietContentExtension);
    extensionRegistry.add(PietFeedActionPayload.pietFeedActionPayloadExtension);
    extensionRegistry.add(StreamStructureProto.Stream.streamExtension);
    extensionRegistry.add(TokenProto.Token.tokenExtension);

    // Call the host and add all the extensions it uses.
    for (GeneratedExtension<?, ?> extension : extensionProvider.getProtoExtensions()) {
      extensionRegistry.add(extension);
    }
  }

  /** Returns the {@link ExtensionRegistryLite}. */
  public ExtensionRegistryLite getExtensionRegistry() {
    return extensionRegistry.getUnmodifiable();
  }
}
