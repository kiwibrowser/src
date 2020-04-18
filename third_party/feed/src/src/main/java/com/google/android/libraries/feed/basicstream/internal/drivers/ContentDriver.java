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

package com.google.android.libraries.feed.basicstream.internal.drivers;

import static com.google.android.libraries.feed.common.Validators.checkState;

import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.PietViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.stream.StreamStructureProto;
import com.google.search.now.ui.stream.StreamStructureProto.Content;
import com.google.search.now.ui.stream.StreamStructureProto.PietContent;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import java.util.ArrayList;
import java.util.List;

/** {@link FeatureDriver} for content. */
public class ContentDriver extends LeafFeatureDriver {

  private static final String TAG = "ContentDriver";

  private final List<PietSharedState> pietSharedStates;
  private final Frame frame;

  /*@Nullable*/ private PietViewHolder viewHolder;

  ContentDriver(ModelFeature contentFeatureModel, ModelProvider modelProvider) {
    PietContent pietContent = getPietContent(contentFeatureModel.getStreamFeature().getContent());
    frame = pietContent.getFrame();
    pietSharedStates = getPietSharedStates(pietContent, modelProvider);
  }

  @Override
  public LeafFeatureDriver getLeafFeatureDriver() {
    return this;
  }

  private PietContent getPietContent(/*@UnderInitialization*/ ContentDriver this, Content content) {
    checkState(
        content.getType() == StreamStructureProto.Content.Type.PIET,
        "Expected Piet type for feature");

    checkState(
        content.hasExtension(PietContent.pietContentExtension),
        "Expected Piet content for feature");

    return content.getExtension(PietContent.pietContentExtension);
  }

  private List<PietSharedState> getPietSharedStates(
      /*@UnderInitialization*/ ContentDriver this,
      PietContent pietContent,
      ModelProvider modelProvider) {
    List<PietSharedState> sharedStates = new ArrayList<>();
    for (ContentId contentId : pietContent.getPietSharedStatesList()) {
      PietSharedState pietSharedState = extractPietSharedState(contentId, modelProvider);
      if (pietSharedState == null) {
        return new ArrayList<>();
      }

      sharedStates.add(pietSharedState);
    }
    return sharedStates;
  }

  /*@Nullable*/
  private PietSharedState extractPietSharedState(
      /*@UnderInitialization*/ ContentDriver this,
      ContentId pietSharedStateId,
      ModelProvider modelProvider) {
    StreamSharedState sharedState = modelProvider.getSharedState(pietSharedStateId);
    if (sharedState != null) {
      return sharedState.getPietSharedStateItem().getPietSharedState();
    }
    Logger.e(
        TAG,
        "Shared state was null. Stylesheets and templates on PietSharedState "
            + "will not be loaded.");
    return null;
  }

  @Override
  public void bind(FeedViewHolder viewHolder) {
    if (!(viewHolder instanceof PietViewHolder)) {
      throw new AssertionError();
    }

    this.viewHolder = (PietViewHolder) viewHolder;

    ((PietViewHolder) viewHolder).bind(frame, pietSharedStates);
  }

  @Override
  public void unbind() {
    if (viewHolder == null) {
      return;
    }

    viewHolder.unbind();
    viewHolder = null;
  }

  @Override
  public int getItemViewType() {
    return ViewHolderType.TYPE_CARD;
  }

  @Override
  public long itemId() {
    return hashCode();
  }

  @VisibleForTesting
  boolean isBound() {
    return viewHolder != null;
  }
}
