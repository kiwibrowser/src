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

package com.google.android.libraries.feed.feedmodelprovider.internal;

import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange.ChildChanges;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelChild.Type;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.common.Validators;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import javax.annotation.concurrent.GuardedBy;

/** Implementation of {@link ModelCursor}. */
public class ModelCursorImpl implements ModelCursor, Dumpable {
  private static final String TAG = "ModelCursorImpl";

  private final Object lock = new Object();
  private final ThreadUtils threadUtils;

  private final String parentContentId;

  @GuardedBy("lock")
  private final List<UpdatableModelChild> childList;

  /*@Nullable*/
  @GuardedBy("lock")
  private CursorIterator iterator;

  // #dump() operation counts
  private int updatesAtEnd = 0;
  private int appendCount = 0;
  private int removeCount = 0;

  /**
   * Create a new ModelCursorImpl. The {@code childList} needs to be a copy of the original list to
   * prevent {@link java.util.ConcurrentModificationException} for changes to the Model. The cursor
   * is informed of changes through the {@link #updateIterator(FeatureChange featureChang)}.
   */
  public ModelCursorImpl(
      String parentContentId, List<UpdatableModelChild> childList, ThreadUtils threadUtils) {
    this.parentContentId = parentContentId;
    this.childList = childList;
    this.iterator = new CursorIterator();
    this.threadUtils = threadUtils;
  }

  public String getParentContentId() {
    return parentContentId;
  }

  public void updateIterator(FeatureChange featureChange) {
    // if the state has been released then ignore the change
    if (isAtEnd()) {
      Logger.i(TAG, "Ignoring Update on cursor currently at end");
      updatesAtEnd++;
      return;
    }
    synchronized (lock) {
      ChildChanges childChanges = featureChange.getChildChanges();
      Logger.i(
          TAG,
          "Update Cursor, removes %s, appends %s",
          childChanges.getRemovedChildren().size(),
          childChanges.getAppendedChildren().size());

      removeChildren(childChanges.getRemovedChildren());

      for (ModelChild modelChild : featureChange.getChildChanges().getAppendedChildren()) {
        if (modelChild instanceof UpdatableModelChild) {
          childList.add(((UpdatableModelChild) modelChild));
          appendCount++;
        } else {
          Logger.e(TAG, "non-UpdateableModelChild found, ignored");
        }
      }
    }
  }

  private void removeChildren(List<ModelChild> children) {
    if (children.isEmpty()) {
      return;
    }
    // Remove only needs to remove all children that are beyond the current position because we
    // have visited everything before and can't revisit them with this cursor.
    synchronized (lock) {
      CursorIterator cursorIterator = Validators.checkNotNull(iterator);
      int currentPosition = cursorIterator.getPosition();
      List<UpdatableModelChild> realRemoves = new ArrayList<>();
      for (ModelChild modelChild : children) {
        String childKey = modelChild.getContentId();
        // This assumes removes are rare so we can walk the list for each deleted child.
        for (int i = currentPosition; i < childList.size(); i++) {
          UpdatableModelChild child = childList.get(i);
          if (child.getContentId().equals(childKey)) {
            realRemoves.add(child);
            break;
          }
        }
      }
      removeCount += realRemoves.size();
      childList.removeAll(realRemoves);
      Logger.i(TAG, "Removed %s children from the Cursor", realRemoves.size());
    }
  }

  @Override
  /*@Nullable*/
  public ModelChild getNextItem() {
    // This should only be called on the UI thread.
    threadUtils.checkMainThread();
    ModelChild nextChild;
    synchronized (lock) {
      if (iterator == null || !iterator.hasNext()) {
        release();
        return null;
      }
      nextChild = iterator.next();
      // If we just hit the last element in the iterator, free all the resources for this cursor.
      if (!iterator.hasNext()) {
        release();
      }
      // If we have a synthetic token, this is the end of the cursor.
      if (nextChild.getType() == Type.TOKEN) {
        ModelToken token = nextChild.getModelToken();
        if (token instanceof UpdatableModelToken) {
          UpdatableModelToken updatableModelToken = (UpdatableModelToken) token;
          if (updatableModelToken.isSynthetic()) {
            Logger.i(TAG, "Releasing token due to synthetic");
            release();
          }
        }
      }
    }
    return nextChild;
  }

  /** Release all the state assocated with this cursor */
  public void release() {
    // This could be called on a background thread.
    synchronized (lock) {
      iterator = null;
    }
  }

  @Override
  public boolean isAtEnd() {
    synchronized (lock) {
      return iterator == null || !this.iterator.hasNext();
    }
  }

  @VisibleForTesting
  class CursorIterator implements Iterator<UpdatableModelChild> {
    private int cursor = 0;

    @Override
    public boolean hasNext() {
      synchronized (lock) {
        return cursor < childList.size();
      }
    }

    @Override
    public UpdatableModelChild next() {
      synchronized (lock) {
        if (cursor >= childList.size()) {
          throw new NoSuchElementException();
        }
        return childList.get(cursor++);
      }
    }

    int getPosition() {
      return cursor;
    }
  }

  @VisibleForTesting
  List<UpdatableModelChild> getChildListForTesting() {
    synchronized (lock) {
      return new ArrayList<>(childList);
    }
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("atEnd").value(isAtEnd());
    dumper.forKey("updatesPostAtEnd").value(updatesAtEnd).compactPrevious();
    dumper.forKey("appendCount").value(appendCount).compactPrevious();
    dumper.forKey("removeCount").value(removeCount).compactPrevious();
  }
}
