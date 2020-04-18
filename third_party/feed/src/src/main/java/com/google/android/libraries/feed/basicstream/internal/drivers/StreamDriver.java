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


import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange;
import com.google.android.libraries.feed.api.modelprovider.FeatureChangeObserver;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelChild.Type;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.config.Configuration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Generates a list of {@link LeafFeatureDriver} instances for an entire stream. */
public class StreamDriver implements FeatureChangeObserver {

  private static final String TAG = "StreamDriver";
  private final ThreadUtils threadUtils;
  private final ModelProvider modelProvider;
  private final Map<ModelChild, FeatureDriver> modelChildFeatureDriverMap;
  private final List<FeatureDriver> featureDrivers;
  private final Configuration configuration;

  private boolean rootFeatureConsumed;

  /*@Nullable*/ private StreamContentListener contentListener;

  public StreamDriver(
      ModelProvider modelProvider, ThreadUtils threadUtils, Configuration configuration) {
    this.threadUtils = threadUtils;
    this.modelProvider = modelProvider;
    this.modelChildFeatureDriverMap = new HashMap<>();
    this.featureDrivers = new ArrayList<>();
    this.configuration = configuration;
  }

  /**
   * Returns a the list of {@link LeafFeatureDriver} instances for the children generated from the
   * given {@link ModelFeature}.
   */
  public List<LeafFeatureDriver> getLeafFeatureDrivers() {
    if (!rootFeatureConsumed) {
      ModelFeature rootFeature = modelProvider.getRootFeature();
      if (rootFeature != null) {
        createAndInsertChildren(rootFeature, modelProvider);
        rootFeature.registerObserver(this);
        rootFeatureConsumed = true;
      }
    }

    return buildLeafFeatureDrivers(featureDrivers);
  }

  private List<FeatureDriver> createAndInsertChildren(
      ModelFeature streamFeature, ModelProvider modelProvider) {
    return createAndInsertChildren(streamFeature.getCursor(), modelProvider);
  }

  private List<FeatureDriver> createAndInsertChildren(
      ModelCursor streamCursor, ModelProvider modelProvider) {
    return createAndInsertChildrenAtIndex(streamCursor, modelProvider, 0);
  }

  private List<FeatureDriver> createAndInsertChildrenAtIndex(
      ModelCursor streamCursor, ModelProvider modelProvider, int insertionIndex) {
    List<FeatureDriver> newChildren = new ArrayList<>();
    ModelChild child;
    while ((child = streamCursor.getNextItem()) != null) {
      FeatureDriver featureDriverChild = createChild(child, modelProvider);
      if (featureDriverChild != null) {
        newChildren.add(featureDriverChild);
        featureDrivers.add(insertionIndex++, featureDriverChild);
        modelChildFeatureDriverMap.put(child, featureDriverChild);
      }
    }
    return newChildren;
  }

  /*@Nullable*/
  private FeatureDriver createChild(ModelChild child, ModelProvider modelProvider) {
    switch (child.getType()) {
      case Type.FEATURE:
        return createFeatureChild(child.getModelFeature());
      case Type.TOKEN:
        ModelToken modelToken = child.getModelToken();
        ContinuationDriver continuationDriver =
            createContinuationDriver(modelProvider, modelToken, configuration);

        modelToken.registerObserver(
            tokenCompleted -> {
              threadUtils.checkMainThread();

              int continuationIndex = removeDriver(child);

              List<FeatureDriver> newChildren =
                  createAndInsertChildrenAtIndex(
                      tokenCompleted.getCursor(), modelProvider, continuationIndex);

              if (contentListener != null) {
                contentListener.notifyContentsAdded(
                    continuationIndex, buildLeafFeatureDrivers(newChildren));
              }
            });

        return continuationDriver;
      case Type.UNBOUND:
        Logger.e(TAG, "Found unbound child %s, ignoring it", child.getContentId());
        return null;
      default:
        Logger.wtf(TAG, "Received illegal child: %s from cursor.", child.getType());
        return null;
    }
  }

  private /*@Nullable*/ FeatureDriver createFeatureChild(ModelFeature modelFeature) {
    if (modelFeature.getStreamFeature().hasCard()) {
      return createCardDriver(modelFeature);
    } else if (modelFeature.getStreamFeature().hasCluster()) {
      return createClusterDriver(modelFeature);
    }

    Logger.w(
        TAG,
        "Invalid StreamFeature Type, must be Card or Cluster but was %s",
        modelFeature.getStreamFeature().getFeaturePayloadCase());
    return null;
  }

  private List<LeafFeatureDriver> buildLeafFeatureDrivers(List<FeatureDriver> featureDrivers) {
    List<LeafFeatureDriver> leafFeatureDrivers = new ArrayList<>();
    for (FeatureDriver featureDriver : featureDrivers) {
      LeafFeatureDriver childContentModel = featureDriver.getLeafFeatureDriver();
      if (childContentModel != null) {
        leafFeatureDrivers.add(childContentModel);
      }
    }
    return leafFeatureDrivers;
  }

  @Override
  public void onChange(FeatureChange change) {
    Logger.v(TAG, "Received change.");

    List<ModelChild> removedChildren = change.getChildChanges().getRemovedChildren();

    for (ModelChild removedChild : removedChildren) {
      if (!(removedChild.getType() == Type.FEATURE || removedChild.getType() == Type.TOKEN)) {
        Logger.e(
            TAG, "Attempting to remove non-removable child of type: %s", removedChild.getType());
        continue;
      }

      removeDriver(removedChild);
    }
  }

  /**
   * Removes the {@link FeatureDriver} represented by the {@link ModelChild} from all collections
   * containing it and updates any listening instances of {@link StreamContentListener} of the
   * removal.
   *
   * <p>Returns the index at which the {@link FeatureDriver} was removed, or -1 if it was not found.
   */
  private int removeDriver(ModelChild modelChild) {
    FeatureDriver featureDriver = modelChildFeatureDriverMap.get(modelChild);
    if (featureDriver == null) {
      Logger.w(TAG, "Attempting to remove feature from ModelChild not in map.");
      return -1;
    }

    for (int i = 0; i < featureDrivers.size(); i++) {
      if (featureDrivers.get(i) == featureDriver) {
        featureDrivers.remove(i);
        modelChildFeatureDriverMap.remove(modelChild);
        if (contentListener != null) {
          contentListener.notifyContentRemoved(i);
        }
        return i;
      }
    }

    Logger.wtf(TAG, "Attempting to remove feature contained on map but not on list of children.");
    return -1;
  }

  @VisibleForTesting
  FeatureDriver createClusterDriver(ModelFeature modelFeature) {
    return new ClusterDriver(modelFeature, modelProvider);
  }

  @VisibleForTesting
  FeatureDriver createCardDriver(ModelFeature modelFeature) {
    return new CardDriver(modelFeature, modelProvider);
  }

  @VisibleForTesting
  ContinuationDriver createContinuationDriver(
      ModelProvider modelProvider, ModelToken modelToken, Configuration configuration) {
    return new ContinuationDriver(modelProvider, modelToken, configuration);
  }

  public void setStreamContentListener(/*@Nullable*/ StreamContentListener contentListener) {
    this.contentListener = contentListener;
  }

  /** Allows listening for changes in the contents held by a {@link StreamDriver} */
  public interface StreamContentListener {

    /** Called when the given content has been added at the given index of stream content. */
    void notifyContentsAdded(int index, List<LeafFeatureDriver> newFeatureDrivers);

    /** Called when the content at the given index of stream content has been removed. */
    void notifyContentRemoved(int index);
  }
}
