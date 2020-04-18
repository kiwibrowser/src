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

package com.google.android.libraries.feed.basicstream.internal;

import android.content.Context;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;
import com.google.android.libraries.feed.api.actionmanager.ActionManager;
import com.google.android.libraries.feed.api.actionparser.ActionParser;
import com.google.android.libraries.feed.host.action.ActionApi;
import com.google.android.libraries.feed.host.action.StreamActionApi;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.ui.action.FeedActionProto.LabelledFeedActionData;
import com.google.search.now.ui.action.FeedActionProto.OpenContextMenuData;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Action handler for Stream. */
public class StreamActionApiImpl implements StreamActionApi {

  private static final String TAG = "StreamActionApiImpl";
  private final Context context;
  private final ActionApi actionApi;
  private final ActionParser actionParser;
  private final ActionManager actionManager;

  public StreamActionApiImpl(
      Context context,
      ActionApi actionApi,
      ActionParser actionParser,
      ActionManager actionManager) {
    this.context = context;
    this.actionApi = actionApi;
    this.actionParser = actionParser;
    this.actionManager = actionManager;
  }

  @Override
  public void openContextMenu(OpenContextMenuData openContextMenuData, View anchorView) {
    List<LabelledFeedActionData> enabledActions = new ArrayList<>();

    PopupMenu popupMenu = new PopupMenu(context, anchorView);
    int itemId = 0;
    for (LabelledFeedActionData labelledFeedActionData :
        openContextMenuData.getContextMenuDataList()) {
      if (actionParser.canPerformAction(labelledFeedActionData.getFeedActionPayload(), this)) {
        popupMenu.getMenu().add(Menu.NONE, itemId++, Menu.NONE, labelledFeedActionData.getLabel());
        enabledActions.add(labelledFeedActionData);
      }
    }

    popupMenu.setOnMenuItemClickListener(
        item -> {
          actionParser.parseFeedActionPayload(
              enabledActions.get(item.getItemId()).getFeedActionPayload(),
              StreamActionApiImpl.this,
              anchorView);
          return true;
        });

    popupMenu.show();
  }

  @Override
  public boolean canOpenContextMenu() {
    return true;
  }

  @Override
  public boolean canDismiss() {
    return true;
  }

  @Override
  public void dismiss(String contentId, List<StreamDataOperation> dataOperations) {
    // TODO: Send sessionToken to actionManager
    actionManager.dismiss(
        Collections.singletonList(contentId), dataOperations, /* sessionToken= */ null);
  }

  @Override
  public void openUrl(String url) {
    actionApi.openUrl(url);
  }

  @Override
  public boolean canOpenUrl() {
    return actionApi.canOpenUrl();
  }

  @Override
  public void openUrlInIncognitoMode(String url) {
    actionApi.openUrlInIncognitoMode(url);
  }

  @Override
  public boolean canOpenUrlInIncognitoMode() {
    return actionApi.canOpenUrlInIncognitoMode();
  }

  @Override
  public void openUrlInNewTab(String url) {
    actionApi.openUrlInNewTab(url);
  }

  @Override
  public boolean canOpenUrlInNewTab() {
    return actionApi.canOpenUrlInNewTab();
  }

  @Override
  public void openUrlInNewWindow(String url) {
    actionApi.openUrlInNewWindow(url);
  }

  @Override
  public boolean canOpenUrlInNewWindow() {
    return actionApi.canOpenUrlInNewWindow();
  }

  @Override
  public void downloadUrl(String url) {
    actionApi.downloadUrl(url);
  }

  @Override
  public boolean canDownloadUrl() {
    return actionApi.canDownloadUrl();
  }

  @Override
  public void learnMore() {
    actionApi.learnMore();
  }

  @Override
  public boolean canLearnMore() {
    return actionApi.canLearnMore();
  }
}
