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

package com.google.android.libraries.feed.feedmodelprovider;

import android.database.Observable;
import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange;
import com.google.android.libraries.feed.api.modelprovider.FeatureChangeObserver;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelChild.Type;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelMutation;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderObserver;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.api.modelprovider.TokenCompleted;
import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.Validators;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Committer;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.android.libraries.feed.feedmodelprovider.internal.CursorProvider;
import com.google.android.libraries.feed.feedmodelprovider.internal.FeatureChangeImpl;
import com.google.android.libraries.feed.feedmodelprovider.internal.ModelChildBinder;
import com.google.android.libraries.feed.feedmodelprovider.internal.ModelCursorImpl;
import com.google.android.libraries.feed.feedmodelprovider.internal.ModelMutationImpl;
import com.google.android.libraries.feed.feedmodelprovider.internal.ModelMutationImpl.Change;
import com.google.android.libraries.feed.feedmodelprovider.internal.UpdatableModelChild;
import com.google.android.libraries.feed.feedmodelprovider.internal.UpdatableModelFeature;
import com.google.android.libraries.feed.feedmodelprovider.internal.UpdatableModelToken;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure.Operation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.UUID;
import javax.annotation.concurrent.GuardedBy;

/** An implementation of {@link ModelProvider}. This will represent the Stream tree in memory. */
public class FeedModelProvider extends Observable<ModelProviderObserver>
    implements ModelProvider, Dumpable {

  private static final String TAG = "FeedModelProvider";
  private static final List<UpdatableModelChild> EMPTY_LIST =
      Collections.unmodifiableList(new ArrayList<>());
  private static final String SYNTHETIC_TOKEN_PREFIX = "_token:";

  private final Object lock = new Object();

  @GuardedBy("lock")
  /*@Nullable*/
  private UpdatableModelChild root = null;

  // The tree is model as a parent with an list of children.  A container is created for every
  // ModelChild with a child.
  @GuardedBy("lock")
  private final Map<String, ArrayList<UpdatableModelChild>> containers = new HashMap<>();

  @GuardedBy("lock")
  private final Map<String, UpdatableModelChild> contents = new HashMap<>();

  @GuardedBy("lock")
  private final Map<ByteString, TokenTracking> tokens = new HashMap<>();

  @GuardedBy("lock")
  private final Map<String, SyntheticTokenTracker> syntheticTokens = new HashMap<>();

  @GuardedBy("lock")
  private final List<WeakReference<ModelCursorImpl>> cursors = new ArrayList<>();

  @GuardedBy("lock")
  private @State int currentState = State.INITIALIZING;

  // #dump() operation counts
  private int removedChildrenCount = 0;
  private int removeScanCount = 0;
  private int commitCount = 0;
  private int commitTokenCount = 0;
  private int commitUpdateCount = 0;
  private int cursorsRemoved = 0;

  private final SessionManager sessionManager;
  private final ThreadUtils threadUtils;
  private final MainThreadRunner mainThreadRunner;
  private final ModelChildBinder modelChildBinder;
  private final TimingUtils timingUtils;

  private final int nonCacheInitialPageSize;
  private final int nonCachePageSize;
  private final int nonCacheMinPageSize;

  /*@Nullable*/ @VisibleForTesting StreamSession streamSession;

  FeedModelProvider(
      SessionManager sessionManager,
      ThreadUtils threadUtils,
      TimingUtils timingUtils,
      MainThreadRunner mainThreadRunner,
      Configuration config) {
    this.sessionManager = sessionManager;
    this.threadUtils = threadUtils;
    this.timingUtils = timingUtils;
    this.mainThreadRunner = mainThreadRunner;
    this.nonCacheInitialPageSize =
        config.getValueOrDefault(ConfigKey.INITIAL_NON_CACHED_PAGE_SIZE, 0);
    this.nonCachePageSize = config.getValueOrDefault(ConfigKey.NON_CACHED_PAGE_SIZE, 0);
    this.nonCacheMinPageSize = config.getValueOrDefault(ConfigKey.NON_CACHED_MIN_PAGE_SIZE, 0);

    CursorProvider cursorProvider =
        parentId -> {
          synchronized (lock) {
            ArrayList<UpdatableModelChild> children = containers.get(parentId);
            if (children == null) {
              Logger.i(TAG, "No children found for Cursor");
              ModelCursorImpl cursor = new ModelCursorImpl(parentId, EMPTY_LIST, threadUtils);
              cursors.add(new WeakReference<>(cursor));
              return cursor;
            }
            ModelCursorImpl cursor =
                new ModelCursorImpl(parentId, new ArrayList<>(children), threadUtils);
            cursors.add(new WeakReference<>(cursor));
            return cursor;
          }
        };
    modelChildBinder = new ModelChildBinder(sessionManager, cursorProvider, timingUtils);
  }

  @Override
  /*@Nullable*/
  public ModelFeature getRootFeature() {
    synchronized (lock) {
      if (root == null) {
        Logger.i(TAG, "Found Empty Stream");
        return null;
      }
      if (root.getType() != Type.FEATURE) {
        Logger.e(TAG, "Root is bound to the wrong type %s", root.getType());
        return null;
      }
      return root.getModelFeature();
    }
  }

  @Override
  /*@Nullable*/
  public ModelChild getModelChild(String contentId) {
    synchronized (lock) {
      return contents.get(contentId);
    }
  }

  @Override
  /*@Nullable*/
  public StreamSharedState getSharedState(ContentId contentId) {
    return sessionManager.getSharedState(contentId);
  }

  @Override
  public void handleToken(ModelToken modelToken) {
    if (modelToken instanceof UpdatableModelToken) {
      UpdatableModelToken token = (UpdatableModelToken) modelToken;
      if (token.isSynthetic()) {
        SyntheticTokenTracker tokenTracker;
        synchronized (lock) {
          tokenTracker = syntheticTokens.get(token.getStreamToken().getContentId());
        }
        if (tokenTracker == null) {
          Logger.e(TAG, "Unable to find the SyntheticTokenTracker");
          return;
        }
        // The nullness checker fails to understand tokenTracker can't be null in the Lambda usage
        SyntheticTokenTracker tt = Validators.checkNotNull(tokenTracker);
        sessionManager.runTask(
            "FeedModelProvider.handleSyntheticToken", () -> tt.handleSyntheticToken(token));
        return;
      }
    }
    StreamSession streamSession = Validators.checkNotNull(this.streamSession);
    sessionManager.handleToken(streamSession, modelToken.getStreamToken());
  }

  @Override
  public void triggerRefresh() {
    threadUtils.checkMainThread();
    sessionManager.triggerRefresh(streamSession);
  }

  @Override
  public void registerObserver(ModelProviderObserver observer) {
    super.registerObserver(observer);
    synchronized (lock) {
      // If we are in the ready state, then call the Observer to inform it things are ready.
      if (currentState == State.READY) {
        observer.onSessionStart();
      } else if (currentState == State.INVALIDATED) {
        observer.onSessionFinished();
      }
    }
  }

  @Override
  public @State int getCurrentState() {
    synchronized (lock) {
      return currentState;
    }
  }

  @Override
  /*@Nullable*/
  public String getSessionToken() {
    if (streamSession == null) {
      Logger.w(TAG, "streamSession is null, this should have been set during population");
      return null;
    }
    return streamSession.getStreamToken();
  }

  @Override
  public ModelMutation edit() {
    return new ModelMutationImpl(committer);
  }

  @Override
  public void invalidate() {
    Logger.i(TAG, "Invalidating the current ModelProvider: session %s", getSessionToken());
    synchronized (lock) {
      currentState = State.INVALIDATED;
      for (WeakReference<ModelCursorImpl> cursorRef : cursors) {
        ModelCursorImpl cursor = cursorRef.get();
        if (cursor != null) {
          cursor.release();
          cursorRef.clear();
        }
      }
      cursors.clear();
      tokens.clear();
      containers.clear();
    }

    // Always run the observers on the UI Thread
    List<ModelProviderObserver> observers = getObserversToNotify();
    mainThreadRunner.execute(
        TAG + " - onSessionFinished",
        () -> {
          for (ModelProviderObserver observer : observers) {
            observer.onSessionFinished();
          }
        });
  }

  @Override
  public void dump(Dumper dumper) {
    synchronized (lock) {
      dumper.title(TAG);
      dumper.forKey("currentState").value(currentState);
      dumper.forKey("contentCount").value(contents.size()).compactPrevious();
      dumper.forKey("containers").value(containers.size()).compactPrevious();
      dumper.forKey("tokens").value(tokens.size()).compactPrevious();
      dumper.forKey("syntheticTokens").value(syntheticTokens.size()).compactPrevious();
      dumper.forKey("observers").value(mObservers.size()).compactPrevious();
      dumper.forKey("commitCount").value(commitCount);
      dumper.forKey("commitTokenCount").value(commitTokenCount).compactPrevious();
      dumper.forKey("commitUpdateCount").value(commitUpdateCount).compactPrevious();
      dumper.forKey("removeCount").value(removedChildrenCount);
      dumper.forKey("removeScanCount").value(removeScanCount).compactPrevious();
      if (root != null) {
        // This is here to satisfy the nullness checker.
        UpdatableModelChild nonNullRoot = Validators.checkNotNull(root);
        if (nonNullRoot.getType() != Type.FEATURE) {
          dumper.forKey("root").value("[ROOT NOT A FEATURE]");
          dumper.forKey("type").value(nonNullRoot.getType()).compactPrevious();
        } else if (nonNullRoot.getModelFeature().getStreamFeature() != null
            && nonNullRoot.getModelFeature().getStreamFeature().hasContentId()) {
          dumper
              .forKey("root")
              .value(nonNullRoot.getModelFeature().getStreamFeature().getContentId());
        } else {
          dumper.forKey("root").value("[FEATURE NOT DEFINED]");
        }
      } else {
        dumper.forKey("root").value("[UNDEFINED]");
      }
      int singleChild = 0;
      Dumper childDumper = dumper.getChildDumper();
      childDumper.title("Containers With Multiple Children");
      for (Entry<String, ArrayList<UpdatableModelChild>> entry : containers.entrySet()) {
        if (entry.getValue().size() > 1) {
          childDumper.forKey("Container").value(entry.getKey());
          childDumper.forKey("childrenCount").value(entry.getValue().size()).compactPrevious();
        } else {
          singleChild++;
        }
      }
      dumper.forKey("singleChildContainers").value(singleChild);
      dumper.forKey("cursors").value(cursors.size());
      int atEnd = 0;
      int cursorEmptyRefs = 0;
      for (WeakReference<ModelCursorImpl> cursorRef : cursors) {
        ModelCursorImpl cursor = cursorRef.get();
        if (cursor == null) {
          cursorEmptyRefs++;
        } else if (cursor.isAtEnd()) {
          atEnd++;
        }
      }
      dumper.forKey("cursorsRemoved").value(cursorsRemoved).compactPrevious();
      dumper.forKey("reclaimedWeakReferences").value(cursorEmptyRefs).compactPrevious();
      dumper.forKey("cursorsAtEnd").value(atEnd).compactPrevious();

      for (WeakReference<ModelCursorImpl> cursorRef : cursors) {
        ModelCursorImpl cursor = cursorRef.get();
        if (cursor != null && !cursor.isAtEnd()) {
          dumper.dump(cursor);
        }
      }
    }
  }

  @VisibleForTesting
  List<ModelProviderObserver> getObserversToNotify() {
    // Make a copy of the observers, so the observers are not mutated while invoking callbacks.
    // mObservers is locked when adding or removing observers. Also, release the lock before
    // invoking callbacks to avoid deadlocks. ([INTERNAL LINK])
    synchronized (mObservers) {
      return new ArrayList<>(mObservers);
    }
  }

  /**
   * Abstract class used by the {@code ModelMutatorCommitter} to modify the model state based upon
   * the current model state and the contents of the mutation. We define mutation handlers for the
   * initialization, for a mutation based upon a continuation token response, and then a standard
   * update mutation. The default implementation is a no-op.
   */
  abstract static class MutationHandler {

    /**
     * Called before processing the children of the mutation. This allows the model to be cleaned up
     * before new children are added.
     */
    void preMutation() {}

    /** Append a child to a parent */
    void appendChild(String parentKey, UpdatableModelChild child) {}

    /** Remove a child from a parent */
    void removeChild(String parentKey, UpdatableModelChild child) {}

    /**
     * This is called after the model has been updated. Typically this will notify observers of the
     * changes made during the mutation.
     */
    void postMutation() {}
  }

  /** This is the {@code ModelMutatorCommitter} which updates the model. */
  private final Committer<Void, Change> committer =
      new Committer<Void, Change>() {
        @Override
        public Void commit(Change change) {
          threadUtils.checkNotMainThread();
          ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
          commitCount++;

          if (change.streamSession != null) {
            streamSession = change.streamSession;
          }

          // Separate the Appends from the Removes.  All appends and updates are considered
          // unbound (childrenToBind) and will need to be sent to the ModelChildBinder.
          List<UpdatableModelChild> appendedChildren = new ArrayList<>();
          List<StreamStructure> removedChildren = new ArrayList<>();
          List<UpdatableModelChild> childrenToBind = new ArrayList<>();
          for (StreamStructure structureChange : change.structureChanges) {
            if (structureChange.getOperation() == Operation.UPDATE_OR_APPEND) {
              UpdatableModelChild child =
                  new UpdatableModelChild(
                      structureChange.getContentId(), structureChange.getParentContentId());
              appendedChildren.add(child);
              childrenToBind.add(child);
            } else if (structureChange.getOperation() == Operation.REMOVE) {
              removedChildren.add(structureChange);
            }
          }

          synchronized (lock) {
            // Add the updates to the childrenToBind
            for (StreamStructure updatedChild : change.updateChanges) {
              UpdatableModelChild child = contents.get(updatedChild.getContentId());
              if (child != null) {
                childrenToBind.add(child);
              } else {
                Logger.w(TAG, "child %s was not found for updating", updatedChild.getContentId());
              }
            }
          }

          // Mutate the Model
          MutationHandler mutationHandler =
              getMutationHandler(change.updateChanges, change.mutationSourceToken);
          processMutation(mutationHandler, appendedChildren, removedChildren);

          if (shouldInsertSyntheticToken(change)) {
            SyntheticTokenTracker tokenTracker =
                new SyntheticTokenTracker(
                    Validators.checkNotNull(root), 0, nonCacheInitialPageSize);
            childrenToBind = tokenTracker.insertToken();
          }

          bindChildrenAndTokens(childrenToBind, (bindingCount) -> mutationHandler.postMutation());
          timeTracker.stop("", "modelProviderCommit");
          Logger.i(
              TAG,
              "ModelProvider Mutation committed - appendedChildren %s, updateChildren %s, "
                  + "removedChildren %s, Token %s",
              appendedChildren.size(),
              change.updateChanges.size(),
              removedChildren.size(),
              change.mutationSourceToken != null);
          return null;
        }

        /** Returns a MutationHandler for processing the mutation */
        private MutationHandler getMutationHandler(
            List<StreamStructure> updatedChildren, StreamToken mutationSourceToken) {
          synchronized (lock) {
            MutationHandler mutationHandler;
            if (currentState == State.INITIALIZING) {
              Validators.checkState(
                  mutationSourceToken == null,
                  "Initializing the Model Provider from a Continuation Token");
              mutationHandler = new InitializeModel();
            } else if (mutationSourceToken != null) {
              mutationHandler = new TokenMutation(mutationSourceToken);
              commitTokenCount++;
            } else {
              mutationHandler = new UpdateMutation(updatedChildren);
              commitUpdateCount++;
            }
            return mutationHandler;
          }
        }

        /** Process the structure changes to update the model. */
        void processMutation(
            MutationHandler mutationHandler,
            List<UpdatableModelChild> appendedChildren,
            List<StreamStructure> removedChildren) {

          ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
          synchronized (lock) {
            // Processing before the structural mutation
            mutationHandler.preMutation();

            // process the structure changes
            boolean rootChanged = false;
            String currentParentKey = null;
            ArrayList<UpdatableModelChild> childrenList = null;
            for (UpdatableModelChild modelChild : appendedChildren) {
              if (!modelChild.hasParentId()) {
                rootChanged = createRoot(modelChild);
                contents.put(modelChild.getContentId(), modelChild);
                continue;
              }

              String parentKey = Validators.checkNotNull(modelChild.getParentId());
              if (!parentKey.equals(currentParentKey)) {
                childrenList = getChildList(parentKey);
                currentParentKey = parentKey;
              }
              if (childrenList == null) {
                Logger.e(TAG, "childrenList was not set");
                continue;
              }
              childrenList.add(modelChild);
              contents.put(modelChild.getContentId(), modelChild);
              mutationHandler.appendChild(parentKey, modelChild);
            }

            for (StreamStructure removeChild : removedChildren) {
              handleRemoveOperation(mutationHandler, removeChild);
              contents.remove(removeChild.getContentId());
            }

            // check if we need to notify that the root was changed
            if (rootChanged && currentState == State.READY) {
              List<ModelProviderObserver> observers = getObserversToNotify();
              mainThreadRunner.execute(
                  TAG + " - onRootSet",
                  () -> {
                    for (ModelProviderObserver observer : observers) {
                      observer.onRootSet();
                    }
                  });
            }
          }
          timeTracker.stop(
              "",
              "modelMutation",
              "appends",
              appendedChildren.size(),
              "removes",
              removedChildren.size());
        }
      };

  private void bindChildrenAndTokens(
      List<UpdatableModelChild> childrenToBind, Consumer<Integer> consumer) {
    // Bind the unbound children
    modelChildBinder.bindChildren(childrenToBind, consumer);

    synchronized (lock) {
      // Track any tokens we added to the tree
      for (UpdatableModelChild child : childrenToBind) {
        if (child.getType() == Type.TOKEN) {
          String parent = child.getParentId();
          if (parent == null) {
            Logger.w(
                TAG,
                "Found a token for a child %s without a parent, ignoring",
                child.getContentId());
            continue;
          }
          ArrayList<UpdatableModelChild> childrenList = getChildList(parent);
          TokenTracking tokenTracking =
              new TokenTracking(child.getUpdatableModelToken(), parent, childrenList);
          tokens.put(child.getModelToken().getStreamToken().getNextPageToken(), tokenTracking);
        }
      }
    }
  }

  private boolean shouldInsertSyntheticToken(Change change) {
    synchronized (lock) {
      return (root != null && !change.cachedBindings && nonCacheInitialPageSize > 0);
    }
  }

  /** Class which handles Synthetic tokens within the root children list. */
  @VisibleForTesting
  class SyntheticTokenTracker {
    private final List<UpdatableModelChild> childrenToBind = new ArrayList<>();
    private final UpdatableModelChild pagingChild;
    private final int startingPosition;
    private final int endPosition;
    private final boolean insertToken;

    private UpdatableModelChild tokenChild;

    SyntheticTokenTracker(UpdatableModelChild pagingChild, int startingPosition, int pageSize) {
      this.pagingChild = pagingChild;

      List<UpdatableModelChild> children = containers.get(pagingChild.getContentId());
      if (children == null) {
        Logger.e(TAG, "Paging child doesn't not have children");
        this.startingPosition = 0;
        this.endPosition = 0;
        this.insertToken = false;
        return;
      }
      int start = startingPosition;
      int end = startingPosition + pageSize;
      if (children.size() <= start) {
        Logger.e(
            TAG,
            "SyntheticTokenTrack to start track beyond child count, start %s, child length %s",
            startingPosition,
            children.size());
        // Bind everything
        start = 0;
        end = children.size();
      } else if (start + pageSize > children.size()
          || start + pageSize + nonCacheMinPageSize > children.size()) {
        end = children.size();
      }
      this.startingPosition = start;
      this.endPosition = end;
      this.insertToken = end < children.size();
    }

    /** Returns the UpdatableModelChild which represents the synthetic token added to the model. */
    UpdatableModelChild getTokenChild() {
      return tokenChild;
    }

    /** Insert a synthetic token into the tree. */
    List<UpdatableModelChild> insertToken() {
      ElapsedTimeTracker tt = timingUtils.getElapsedTimeTracker(TAG);
      traverse(pagingChild, startingPosition, endPosition);
      if (insertToken) {
        synchronized (lock) {
          ArrayList<UpdatableModelChild> rootChildren = containers.get(pagingChild.getContentId());
          if (rootChildren != null) {
            tokenChild = getSyntheticToken();
            rootChildren.add(endPosition, tokenChild);
            syntheticTokens.put(tokenChild.getContentId(), this);
            Logger.i(
                TAG,
                "Inserting a Synthetic Token %s at %s",
                tokenChild.getContentId(),
                endPosition);
          } else {
            Logger.e(TAG, "Unable to find paging node's children");
          }
        }
      }
      tt.stop("", "syntheticTokens");
      return childrenToBind;
    }

    /** Handle the synthetic token */
    void handleSyntheticToken(UpdatableModelToken token) {
      synchronized (lock) {
        StreamToken streamToken = token.getStreamToken();
        SyntheticTokenTracker tracker = syntheticTokens.remove(streamToken.getContentId());
        if (tracker == null) {
          Logger.e(TAG, "SyntheticTokenTracker was not found");
          return;
        }

        UpdatableModelChild tokenChild = tracker.getTokenChild();
        Logger.i(TAG, "Found Token %s", tokenChild != null);
        if (tokenChild != null && root != null) {
          List<UpdatableModelChild> rootChildren = containers.get(root.getContentId());
          if (rootChildren != null) {
            int pos = rootChildren.indexOf(tokenChild);
            if (pos > 0) {
              rootChildren.remove(pos);
              SyntheticTokenTracker tokenTracker =
                  new SyntheticTokenTracker(Validators.checkNotNull(root), pos, nonCachePageSize);
              List<UpdatableModelChild> childrenToBind = tokenTracker.insertToken();
              List<UpdatableModelChild> cursorSublist =
                  rootChildren.subList(pos, rootChildren.size());

              // Bind the unbound children
              bindChildrenAndTokens(
                  childrenToBind,
                  (bindingCount) -> {
                    ModelCursorImpl cursor =
                        new ModelCursorImpl(streamToken.getParentId(), cursorSublist, threadUtils);

                    TokenCompleted tokenCompleted = new TokenCompleted(cursor);
                    List<TokenCompletedObserver> observers = token.getObserversToNotify();
                    mainThreadRunner.execute(
                        TAG + " - onTokenChange",
                        () -> {
                          for (TokenCompletedObserver observer : observers) {
                            observer.onTokenCompleted(tokenCompleted);
                          }
                        });
                  });
            }
          }
        }
      }
    }

    private void traverse(UpdatableModelChild node, int start, int end) {
      synchronized (lock) {
        if (node.getType() == Type.UNBOUND) {
          childrenToBind.add(node);
        }
        String nodeId = node.getContentId();
        List<UpdatableModelChild> children = containers.get(nodeId);
        if (children != null && !children.isEmpty()) {
          int maxChildren = Math.min(end, children.size());
          for (int i = start; i < maxChildren; i++) {
            UpdatableModelChild child = children.get(i);
            traverse(child, 0, Integer.MAX_VALUE);
          }
        }
      }
    }

    private UpdatableModelChild getSyntheticToken() {
      synchronized (lock) {
        UpdatableModelChild r = Validators.checkNotNull(root);
        String contentId = SYNTHETIC_TOKEN_PREFIX + UUID.randomUUID();
        StreamToken streamToken = StreamToken.newBuilder().setContentId(contentId).build();
        UpdatableModelChild modelChild = new UpdatableModelChild(contentId, r.getContentId());
        modelChild.bindToken(new UpdatableModelToken(streamToken, true));
        return modelChild;
      }
    }
  }

  private void handleRemoveOperation(MutationHandler mutationHandler, StreamStructure removeChild) {
    // remove everything from the remove list first
    String parentKey = removeChild.getParentContentId();
    synchronized (lock) {
      List<UpdatableModelChild> childList = containers.get(parentKey);
      if (childList == null) {
        Logger.w(TAG, "Parent of removed item is not found");
        return;
      }

      // For FEATURE children, add the remove to the mutation handler to create the
      // StreamFeatureChange.  We skip this for TOKENS.
      String childKey = removeChild.getContentId();
      UpdatableModelChild targetChild = contents.get(childKey);
      if (targetChild == null) {
        Logger.e(TAG, "Child %s not found in the ModelProvider contents", childKey);
        return;
      }
      if (targetChild.getType() == Type.FEATURE) {
        Logger.i(TAG, "Removing a child");
        mutationHandler.removeChild(parentKey, targetChild);
      }

      // This walks the child list backwards because the most common removal item is a
      // token which is always the last item in the list.  removeScanCount tracks if we are
      // walking the list too much
      ListIterator<UpdatableModelChild> li = childList.listIterator(childList.size());
      UpdatableModelChild removed = null;
      while (li.hasPrevious()) {
        removeScanCount++;
        UpdatableModelChild child = li.previous();
        if (child.getContentId().equals(childKey)) {
          removed = child;
          break;
        }
      }

      if (removed != null) {
        childList.remove(removed);
        removedChildrenCount++;
      } else {
        Logger.w(TAG, "Child to be removed was not found");
      }
    }
  }

  /**
   * This {@link MutationHandler} handles the initial mutation populating the model. No update
   * events are triggered. When the model is updated, we trigger a Session Started event.
   */
  @VisibleForTesting
  class InitializeModel extends MutationHandler {
    @Override
    public void postMutation() {
      synchronized (lock) {
        currentState = State.READY;
      }
      List<ModelProviderObserver> observers = getObserversToNotify();
      mainThreadRunner.execute(
          TAG + " - onSessionStart",
          () -> {
            for (ModelProviderObserver observer : observers) {
              observer.onSessionStart();
            }
          });
    }
  }

  /**
   * This {@link MutationHandler} handles a mutation based upon a continuation token. For a token we
   * will not generate changes for the parent updated by the token. Instead, the new children are
   * appended and a {@link TokenCompleted} will be triggered.
   */
  @VisibleForTesting
  class TokenMutation extends MutationHandler {
    private final StreamToken mutationSourceToken;
    /*@Nullable*/ TokenTracking token = null;
    int newCursorStart = -1;

    TokenMutation(StreamToken mutationSourceToken) {
      this.mutationSourceToken = mutationSourceToken;
    }

    @VisibleForTesting
    TokenTracking getTokenTrackingForTest() {
      synchronized (lock) {
        return Validators.checkNotNull(tokens.get(mutationSourceToken.getNextPageToken()));
      }
    }

    @Override
    public void preMutation() {
      synchronized (lock) {
        token = tokens.remove(mutationSourceToken.getNextPageToken());
        if (token == null) {
          Logger.e(TAG, "Token was not found, positioning to end of list");
          return;
        }
        // adjust the location because we will remove the token
        newCursorStart = token.location.size() - 1;
      }
    }

    @Override
    public void postMutation() {
      if (token == null) {
        Logger.e(TAG, "Token was not found, mutation is being ignored");
        return;
      }
      ModelCursorImpl cursor =
          new ModelCursorImpl(
              token.parentContentId,
              token.location.subList(newCursorStart, token.location.size()),
              threadUtils);
      TokenCompleted tokenCompleted = new TokenCompleted(cursor);
      List<TokenCompletedObserver> observers = token.tokenChild.getObserversToNotify();
      mainThreadRunner.execute(
          TAG + " - onTokenChange",
          () -> {
            for (TokenCompletedObserver observer : observers) {
              observer.onTokenCompleted(tokenCompleted);
            }
          });
    }
  }

  /**
   * {@code MutationHandler} which handles updates. All changes are tracked for the UI through
   * {@link FeatureChange}. One will be created for each {@link ModelFeature} that changed. There
   * are two types of changes, the content and changes to the children (structure).
   */
  @VisibleForTesting
  class UpdateMutation extends MutationHandler {

    private final List<StreamStructure> updates;
    private final Map<String, FeatureChangeImpl> changes = new HashMap<>();
    private final Set<String> newParents = new HashSet<>();

    UpdateMutation(List<StreamStructure> updates) {
      this.updates = updates;
    }

    @Override
    public void preMutation() {
      Logger.i(TAG, "Updating %s items", updates.size());
      // Walk all the updates and update the values, creating changes to track these
      for (StreamStructure update : updates) {
        FeatureChangeImpl change = getChange(update.getContentId());
        if (change != null) {
          change.setFeatureChanged(true);
        }
      }
    }

    @Override
    public void removeChild(String parentKey, UpdatableModelChild child) {
      Logger.i(TAG, "Removing child %s from %s", child.getContentId(), parentKey);
      FeatureChangeImpl change = getChange(parentKey);
      if (change != null) {
        change.getChildChangesImpl().removeChild(child);
      }
    }

    @Override
    public void appendChild(String parentKey, UpdatableModelChild child) {
      // Is this a child of a node that is new to the model?  We only report changes
      // to existing ModelFeatures.
      String childKey = child.getContentId();
      if (newParents.contains(parentKey)) {
        // Don't create a change the child of a new child
        newParents.add(childKey);
        return;
      }

      newParents.add(childKey);
      FeatureChangeImpl change = getChange(parentKey);
      if (change != null) {
        Logger.i(TAG, "Add child %s to a parent %s", childKey, parentKey);
        change.getChildChangesImpl().addAppendChild(child);
      }
    }

    @Override
    public void postMutation() {
      synchronized (lock) {
        // Update the cursors before we notify the UI
        List<WeakReference<ModelCursorImpl>> removeList = new ArrayList<>();
        for (WeakReference<ModelCursorImpl> cursorRef : cursors) {
          ModelCursorImpl cursor = cursorRef.get();
          if (cursor != null) {
            FeatureChange change = changes.get(cursor.getParentContentId());
            if (change != null) {
              cursor.updateIterator(change);
            }
          } else {
            removeList.add(cursorRef);
          }
        }
        cursorsRemoved += removeList.size();
        cursors.removeAll(removeList);
      }

      // Update the Observers on the UI Thread
      mainThreadRunner.execute(
          TAG + " - onFeatureChange",
          () -> {
            for (FeatureChangeImpl change : changes.values()) {
              // TODO: This cast is because StreamFeatureChange is part of the API, cleanup?
              List<FeatureChangeObserver> observers =
                  ((UpdatableModelFeature) change.getModelFeature()).getObserversToNotify();
              for (FeatureChangeObserver observer : observers) {
                observer.onChange(change);
              }
            }
          });
    }

    /*@Nullable*/
    private FeatureChangeImpl getChange(String contentIdKey) {
      FeatureChangeImpl change = changes.get(contentIdKey);
      if (change == null) {
        UpdatableModelChild modelChild;
        synchronized (lock) {
          modelChild = contents.get(contentIdKey);
        }
        if (modelChild == null) {
          Logger.e(TAG, "Didn't find '%s' in content", contentIdKey);
          return null;
        }
        change = new FeatureChangeImpl(modelChild.getModelFeature());
        changes.put(contentIdKey, change);
      }
      return change;
    }
  }

  // This method will return true if it sets/updates root
  private boolean createRoot(UpdatableModelChild child) {
    synchronized (lock) {
      // this must be a root
      if (child.getType() == Type.FEATURE || child.getType() == Type.UNBOUND) {
        if (root != null) {
          // TODO: What do we do about multiple roots?
          Logger.e(
              TAG,
              "Found multiple roots, this will overwrite the previous definition: %s",
              child.getContentId());
        }
        root = child;
        return true;
      } else {
        // continuation tokens can not be roots.
        Logger.e(TAG, "Invalid Root, type %s", child.getType());
      }
      return false;
    }
  }

  // Lazy creation of containers
  private ArrayList<UpdatableModelChild> getChildList(String parentKey) {
    synchronized (lock) {
      if (!containers.containsKey(parentKey)) {
        containers.put(parentKey, new ArrayList<>());
      }
      return containers.get(parentKey);
    }
  }

  /** Track the continuation token location and model */
  @VisibleForTesting
  static class TokenTracking {
    final UpdatableModelToken tokenChild;
    final String parentContentId;
    final ArrayList<UpdatableModelChild> location;

    TokenTracking(
        UpdatableModelToken tokenChild,
        String parentContentId,
        ArrayList<UpdatableModelChild> location) {
      this.tokenChild = tokenChild;
      this.parentContentId = parentContentId;
      this.location = location;
    }
  }

  // test only method for returning a copy of the tokens map
  @VisibleForTesting
  Map<ByteString, TokenTracking> getTokensForTest() {
    synchronized (lock) {
      return new HashMap<>(tokens);
    }
  }

  // test only method for verifying the synthetic tokens
  Map<String, SyntheticTokenTracker> getSyntheticTokensForTest() {
    synchronized (lock) {
      return new HashMap<>(syntheticTokens);
    }
  }
}
