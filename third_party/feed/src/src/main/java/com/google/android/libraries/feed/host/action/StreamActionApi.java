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

package com.google.android.libraries.feed.host.action;

import android.view.View;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.ui.action.FeedActionProto.OpenContextMenuData;
import java.util.List;

/**
 * Allows the {@link com.google.android.libraries.feed.api.actionparser.ActionParser} to communicate
 * actions back to the Stream after parsing.
 */
public interface StreamActionApi extends ActionApi {

  /** Whether or not a context menu can be opened */
  boolean canOpenContextMenu();

  /** Opens context menu. */
  void openContextMenu(OpenContextMenuData openContextMenuData, View anchorView);

  /** Whether or not a card can be dismissed. */
  boolean canDismiss();

  /** Dismisses card. */
  void dismiss(String contentId, List<StreamDataOperation> dataOperations);
}
