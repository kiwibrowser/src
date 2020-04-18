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

package com.google.android.libraries.feed.api.actionparser;

import android.view.View;
import com.google.android.libraries.feed.host.action.StreamActionApi;
import com.google.search.now.ui.action.FeedActionPayloadProto.FeedActionPayload;
import com.google.search.now.ui.piet.ActionsProto.Action;

/** Parses actions from Piet and directs the Stream to handle the action. */
public interface ActionParser {

  void parseAction(
      Action action, StreamActionApi streamActionApi, View view, /*@Nullable*/ String veLoggingToken);

  void parseFeedActionPayload(
      FeedActionPayload feedActionPayload, StreamActionApi streamActionApi, View view);

  boolean canPerformAction(FeedActionPayload feedActionPayload, StreamActionApi streamActionApi);
}
