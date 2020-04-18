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

package com.google.android.libraries.feed.feedactionparser.internal;

import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.ui.action.FeedActionPayloadProto.FeedActionPayload;
import com.google.search.now.ui.action.PietExtensionsProto.PietFeedActionPayload;
import com.google.search.now.ui.piet.ActionsProto.Action;

/** Class which is able to retrieve FeedActions from Piet actions */
public class PietFeedActionPaylaodRetriever {

  private static final String TAG = "PietFAPRetriever";

  /**
   * Gets the feed action from a Piet Action.
   *
   * @param action the Piet Action to pull the feed action metadata out of.
   */
  /*@Nullable*/
  public FeedActionPayload getFeedActionPayload(Action action) {
    /*@Nullable*/
    PietFeedActionPayload feedActionPayloadExtension =
        action.getExtension(PietFeedActionPayload.pietFeedActionPayloadExtension);
    if (feedActionPayloadExtension != null && feedActionPayloadExtension.hasFeedActionPayload()) {
      return feedActionPayloadExtension.getFeedActionPayload();
    }
    Logger.e(TAG, "FeedActionExtension was null or did not contain a feed action payload");
    return null;
  }
}
