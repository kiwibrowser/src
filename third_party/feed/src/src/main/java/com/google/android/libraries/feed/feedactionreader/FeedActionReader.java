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

package com.google.android.libraries.feed.feedactionreader;

import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.DismissActionWithSemanticProperties;
import com.google.android.libraries.feed.api.common.SemanticPropertiesWithId;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.search.now.feed.client.StreamDataProto.StreamAction;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** Feed implementation of {@link ActionReader} */
public class FeedActionReader implements ActionReader {
  private static final String TAG = "FeedActionReader";

  private final Store store;
  private final Clock clock;
  private final ProtocolAdapter protocolAdapter;
  private final long dismissActionTTLSeconds;

  public FeedActionReader(
      Store store, Clock clock, ProtocolAdapter protocolAdapter, Configuration configuration) {
    this.store = store;
    this.clock = clock;
    this.protocolAdapter = protocolAdapter;
    this.dismissActionTTLSeconds =
        configuration.getValueOrDefault(
            ConfigKey.DEFAULT_ACTION_TTL_SECONDS, TimeUnit.DAYS.toSeconds(3));
  }

  @Override
  public Result<List<DismissActionWithSemanticProperties>>
      getDismissActionsWithSemanticProperties() {
    Result<List<StreamAction>> dismissActionsResult = store.getAllDismissActions();
    if (!dismissActionsResult.isSuccessful()) {
      Logger.e(TAG, "Error fetching dismiss actions from store");
      return Result.failure();
    }
    List<StreamAction> dismissActions = dismissActionsResult.getValue();
    List<String> contentIds = new ArrayList<>(dismissActions.size());
    long minValidTime =
        TimeUnit.MILLISECONDS.toSeconds(clock.currentTimeMillis()) - dismissActionTTLSeconds;
    for (StreamAction dismissAction : dismissActions) {
      if (dismissAction.getTimestampSeconds() > minValidTime) {
        contentIds.add(dismissAction.getFeatureContentId());
      }
    }

    Result<List<SemanticPropertiesWithId>> semanticPropertiesResult =
        store.getSemanticProperties(contentIds);
    if (!semanticPropertiesResult.isSuccessful()) {
      return Result.failure();
    }
    List<DismissActionWithSemanticProperties> dismissActionWithSemanticProperties =
        new ArrayList<>(contentIds.size());

    for (SemanticPropertiesWithId semanticPropertiesWithId : semanticPropertiesResult.getValue()) {
      Result<ContentId> wireContentIdResult =
          protocolAdapter.getWireContentId(semanticPropertiesWithId.contentId);
      if (!wireContentIdResult.isSuccessful()) {
        Logger.e(
            TAG,
            "Error converting to wire result for contentId: %s",
            semanticPropertiesWithId.contentId);
        continue;
      }
      dismissActionWithSemanticProperties.add(
          new DismissActionWithSemanticProperties(
              wireContentIdResult.getValue(), semanticPropertiesWithId.semanticData));
      // Also strip out from the content ids list (so that we can put those in with null semantic
      // properties
      contentIds.remove(semanticPropertiesWithId.contentId);
    }
    for (String contentId : contentIds) {
      Result<ContentId> wireContentIdResult = protocolAdapter.getWireContentId(contentId);
      if (!wireContentIdResult.isSuccessful()) {
        Logger.e(TAG, "Error converting to wire result for contentId: %s", contentId);
        continue;
      }
      dismissActionWithSemanticProperties.add(
          new DismissActionWithSemanticProperties(wireContentIdResult.getValue(), null));
    }
    return Result.success(dismissActionWithSemanticProperties);
  }
}
