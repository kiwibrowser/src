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

import static com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType.TYPE_CONTINUATION;
import static com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType.TYPE_HEADER;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;
import com.google.android.libraries.feed.api.actionmanager.ActionManager;
import com.google.android.libraries.feed.api.actionparser.ActionParser;
import com.google.android.libraries.feed.basicstream.internal.drivers.LeafFeatureDriver;
import com.google.android.libraries.feed.basicstream.internal.drivers.StreamDriver;
import com.google.android.libraries.feed.basicstream.internal.drivers.StreamDriver.StreamContentListener;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ContinuationViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.HeaderViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.PietViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.action.ActionApi;
import com.google.android.libraries.feed.host.stream.CardConfiguration;
import com.google.android.libraries.feed.piet.PietManager;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/** A RecyclerView adapter which can show a list of views with Piet Stream features. */
public class StreamRecyclerViewAdapter extends RecyclerView.Adapter<FeedViewHolder>
    implements StreamContentListener {
  private static final String TAG = "StreamRecyclerViewAdapt";

  private final Context context;
  private final CardConfiguration cardConfiguration;
  private final PietManager pietManager;
  private final ActionParser actionParser;
  private final ActionApi actionApi;
  private final ActionManager actionManager;
  private final List<LeafFeatureDriver> leafFeatureDrivers;
  private final List<View> headers;
  private final HashMap<FeedViewHolder, LeafFeatureDriver> boundViewHolderToLeafFeatureDriverMap;
  /*@Nullable*/ private StreamDriver streamDriver;

  // Suppress initialization warnings for calling setHasStableIds on RecyclerView.Adapter
  @SuppressWarnings("initialization")
  public StreamRecyclerViewAdapter(
      Context context,
      CardConfiguration cardConfiguration,
      PietManager pietManager,
      ActionParser actionParser,
      ActionApi actionApi,
      ActionManager actionManager) {
    this.context = context;
    this.cardConfiguration = cardConfiguration;
    this.pietManager = pietManager;
    this.actionParser = actionParser;
    this.actionApi = actionApi;
    this.actionManager = actionManager;
    headers = new ArrayList<>();
    leafFeatureDrivers = new ArrayList<>();
    setHasStableIds(true);
    boundViewHolderToLeafFeatureDriverMap = new HashMap<>();
  }

  @Override
  public FeedViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    if (viewType == TYPE_HEADER) {
      FrameLayout frameLayout = new FrameLayout(parent.getContext());
      frameLayout.setLayoutParams(
          new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
      return new HeaderViewHolder(frameLayout);
    } else if (viewType == TYPE_CONTINUATION) {
      FrameLayout frameLayout = new FrameLayout(parent.getContext());
      frameLayout.setLayoutParams(
          new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
      return new ContinuationViewHolder(parent.getContext(), frameLayout);
    }

    FrameLayout cardView = new FrameLayout(context);
    StreamActionApiImpl streamActionApi =
        new StreamActionApiImpl(context, actionApi, actionParser, actionManager);
    return new PietViewHolder(
        cardConfiguration, cardView, pietManager, context, streamActionApi, actionParser);
  }

  @Override
  public void onBindViewHolder(FeedViewHolder viewHolder, int i) {
    if (isHeader(i)) {
      // TODO: Create ContentModels for headers.
      injectHeader((HeaderViewHolder) viewHolder, i);
      return;
    }
    Logger.d(TAG, "onBindViewHolder %s", i);
    LeafFeatureDriver leafFeatureDriver = leafFeatureDrivers.get(positionToStreamIndex(i));

    leafFeatureDriver.bind(viewHolder);
    boundViewHolderToLeafFeatureDriverMap.put(viewHolder, leafFeatureDriver);
  }

  private void injectHeader(HeaderViewHolder headerViewHolder, int position) {
    View header = headers.get(position);
    if (header.getParent() == null) {
      headerViewHolder.bind(header);
    }
  }

  @Override
  public void onViewRecycled(FeedViewHolder viewHolder) {
    if (viewHolder instanceof HeaderViewHolder) {
      viewHolder.unbind();
      return;
    }

    LeafFeatureDriver leafFeatureDriver = boundViewHolderToLeafFeatureDriverMap.get(viewHolder);

    if (leafFeatureDriver == null) {
      Logger.wtf(TAG, "Could not find driver for unbinding");
      return;
    }

    leafFeatureDriver.unbind();
    boundViewHolderToLeafFeatureDriverMap.remove(viewHolder);
  }

  @Override
  public int getItemCount() {
    return leafFeatureDrivers.size() + headers.size();
  }

  @Override
  @ViewHolderType
  public int getItemViewType(int position) {
    if (isHeader(position)) {
      return TYPE_HEADER;
    }

    return leafFeatureDrivers.get(positionToStreamIndex(position)).getItemViewType();
  }

  @Override
  public long getItemId(int position) {
    if (isHeader(position)) {
      return headers.get(position).hashCode();
    }

    return leafFeatureDrivers.get(positionToStreamIndex(position)).itemId();
  }

  @VisibleForTesting
  public List<LeafFeatureDriver> getLeafFeatureDrivers() {
    return leafFeatureDrivers;
  }

  private boolean isHeader(int position) {
    return position < headers.size();
  }

  private int positionToStreamIndex(int position) {
    return position - headers.size();
  }

  public void setHeaders(List<View> newHeaders) {
    headers.clear();
    headers.addAll(newHeaders);
    notifyDataSetChanged();
  }

  public void setDriver(StreamDriver newStreamDriver) {
    if (streamDriver != null) {
      streamDriver.setStreamContentListener(null);
    }

    leafFeatureDrivers.clear();

    newStreamDriver.setStreamContentListener(this);
    leafFeatureDrivers.addAll(newStreamDriver.getLeafFeatureDrivers());

    streamDriver = newStreamDriver;
    notifyDataSetChanged();
  }

  @Override
  public void notifyContentsAdded(int index, List<LeafFeatureDriver> newFeatureDrivers) {
    leafFeatureDrivers.addAll(index, newFeatureDrivers);

    int insertionIndex = headers.size() + index;
    notifyItemRangeInserted(insertionIndex, newFeatureDrivers.size());
  }

  @Override
  public void notifyContentRemoved(int index) {
    int removalIndex = headers.size() + index;
    leafFeatureDrivers.remove(index);
    notifyItemRemoved(removalIndex);
  }

}
