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

package com.google.android.libraries.feed.feedactionparser;

import static com.google.android.libraries.feed.common.Validators.checkState;
import static com.google.search.now.ui.action.FeedActionProto.FeedActionMetadata.Type.OPEN_URL;
import static com.google.search.now.ui.action.FeedActionProto.FeedActionMetadata.Type.OPEN_URL_INCOGNITO;
import static com.google.search.now.ui.action.FeedActionProto.FeedActionMetadata.Type.OPEN_URL_NEW_WINDOW;

import android.view.View;
import com.google.android.libraries.feed.api.actionparser.ActionParser;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.feedactionparser.internal.PietFeedActionPaylaodRetriever;
import com.google.android.libraries.feed.host.action.StreamActionApi;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.ui.action.FeedActionPayloadProto.FeedActionPayload;
import com.google.search.now.ui.action.FeedActionProto.FeedAction;
import com.google.search.now.ui.action.FeedActionProto.FeedActionMetadata;
import com.google.search.now.ui.action.FeedActionProto.FeedActionMetadata.Type;
import com.google.search.now.ui.action.FeedActionProto.OpenUrlData;
import com.google.search.now.ui.piet.ActionsProto.Action;
import java.util.List;

/**
 * Action parser which is able to parse Feed actions and notify clients about which action needs to
 * be performed.
 */
public class FeedActionParser implements ActionParser {

  private static final String TAG = "FeedActionParser";

  private final PietFeedActionPaylaodRetriever pietFeedActionPaylaodRetriever;
  private final ProtocolAdapter protocolAdapter;

  public FeedActionParser(ProtocolAdapter protocolAdapter) {
    this.protocolAdapter = protocolAdapter;
    this.pietFeedActionPaylaodRetriever = new PietFeedActionPaylaodRetriever();
  }

  @Override
  public void parseAction(
      Action action, StreamActionApi streamActionApi, View view, /*@Nullable*/ String veLoggingToken) {
    FeedActionPayload feedActionPayload =
        pietFeedActionPaylaodRetriever.getFeedActionPayload(action);
    if (feedActionPayload == null) {
      Logger.w(TAG, "Unable to get FeedActionPayload from PietFeedActionPaylaodRetriever");
      return;
    }
    parseFeedActionPayload(feedActionPayload, streamActionApi, view);
  }

  @Override
  public void parseFeedActionPayload(
      FeedActionPayload feedActionPayload, StreamActionApi streamActionApi, View view) {
    FeedActionMetadata feedActionMetadata =
        feedActionPayload.getExtension(FeedAction.feedActionExtension).getMetadata();
    switch (feedActionMetadata.getType()) {
      case OPEN_URL:
      case OPEN_URL_NEW_WINDOW:
      case OPEN_URL_INCOGNITO:
        handleOpenUrl(
            feedActionMetadata.getType(), feedActionMetadata.getOpenUrlData(), streamActionApi);
        break;
      case OPEN_CONTEXT_MENU:
        if (!streamActionApi.canOpenContextMenu()) {
          Logger.e(TAG, "Cannot open context menu: StreamActionApi does not support it.");
          break;
        }

        if (!feedActionMetadata.hasOpenContextMenuData()) {
          Logger.e(TAG, "Cannot open context menu: does not have context menu data.");
          break;
        }

        streamActionApi.openContextMenu(feedActionMetadata.getOpenContextMenuData(), view);
        break;
      case DISMISS:
        if (!streamActionApi.canDismiss()) {
          Logger.e(TAG, "Cannot dismiss: StreamActionApi does not support it");
          break;
        }

        if (!feedActionMetadata.getDismissData().hasContentId()) {
          Logger.e(TAG, "Cannot dismiss: no Content Id");
          break;
        }

        Result<List<StreamDataOperation>> streamDataOperationsResult =
            protocolAdapter.createOperations(
                feedActionMetadata.getDismissData().getDataOperationsList());

        if (!streamDataOperationsResult.isSuccessful()) {
          Logger.e(TAG, "Cannot dismiss: conversion to StreamDataOperation failed.");
          break;
        }

        streamActionApi.dismiss(
            protocolAdapter.getStreamContentId(feedActionMetadata.getDismissData().getContentId()),
            streamDataOperationsResult.getValue());
        break;
      case DOWNLOAD:
        if (!streamActionApi.canDownloadUrl()) {
          Logger.e(TAG, "Cannot download: StreamActionApi does not support it");
          break;
        }

        if (!feedActionMetadata.getDownloadData().hasUrl()) {
          Logger.e(TAG, " Cannot download: no URL");
          break;
        }

        streamActionApi.downloadUrl(feedActionMetadata.getDownloadData().getUrl());
        break;
      default:
        Logger.wtf(TAG, "Haven't implemented host handling of %s", feedActionMetadata.getType());
    }
  }

  private void handleOpenUrl(
      Type urlType, OpenUrlData openUrlData, StreamActionApi streamActionApi) {
    checkState(
        urlType.equals(OPEN_URL)
            || urlType.equals(OPEN_URL_NEW_WINDOW)
            || urlType.equals(OPEN_URL_INCOGNITO),
        "Attempting to handle URL that is not a URL type: %s",
        urlType);
    if (!canPerformAction(urlType, streamActionApi)) {
      Logger.e(TAG, "Cannot open URL action: %s, not supported.", urlType);
      return;
    }

    if (!openUrlData.hasUrl()) {
      Logger.e(TAG, "Cannot open URL action: %s, no URL available.", urlType);
      return;
    }

    String url = openUrlData.getUrl();
    switch (urlType) {
      case OPEN_URL:
        streamActionApi.openUrl(url);
        break;
      case OPEN_URL_NEW_WINDOW:
        streamActionApi.openUrlInNewWindow(url);
        break;
      case OPEN_URL_INCOGNITO:
        streamActionApi.openUrlInIncognitoMode(url);
        break;
      default:
        throw new AssertionError("Unhandled URL type: " + urlType);
    }
  }

  @Override
  public boolean canPerformAction(
      FeedActionPayload feedActionPayload, StreamActionApi streamActionApi) {
    return canPerformAction(
        feedActionPayload.getExtension(FeedAction.feedActionExtension).getMetadata().getType(),
        streamActionApi);
  }

  private boolean canPerformAction(Type type, StreamActionApi streamActionApi) {
    switch (type) {
      case OPEN_URL:
        return streamActionApi.canOpenUrl();
      case OPEN_URL_NEW_WINDOW:
        return streamActionApi.canOpenUrlInNewWindow();
      case OPEN_URL_INCOGNITO:
        return streamActionApi.canOpenUrlInIncognitoMode();
      case OPEN_CONTEXT_MENU:
        return streamActionApi.canOpenContextMenu();
      case DISMISS:
        return streamActionApi.canDismiss();
      case DOWNLOAD:
        return streamActionApi.canDownloadUrl();
      case UNKNOWN:
        break;
    }
    Logger.e(TAG, "Unhandled feed action type: %s", type);
    return false;
  }
}
