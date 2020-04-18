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

import com.google.android.libraries.feed.api.common.PayloadWithId;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelChild.Type;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Bind the {@link StreamPayload} objects to the {@link ModelChild} instances. If the model child is
 * UNBOUND, we will bind the initial type. For FEATURE model child instances we will update the
 * data.
 */
public class ModelChildBinder {
  private static final String TAG = "ModelChildBinder";

  private final SessionManager sessionManager;
  private final CursorProvider cursorProvider;
  private final TimingUtils timingUtils;

  public ModelChildBinder(
      SessionManager sessionManager, CursorProvider cursorProvider, TimingUtils timingUtils) {
    this.sessionManager = sessionManager;
    this.cursorProvider = cursorProvider;
    this.timingUtils = timingUtils;
  }

  public void bindChildren(List<UpdatableModelChild> childrenToBind, Consumer<Integer> consumer) {
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    Map<String, UpdatableModelChild> bindingChildren = new HashMap<>();
    List<String> contentIds = new ArrayList<>();
    for (UpdatableModelChild child : childrenToBind) {
      String key = child.getContentId();
      contentIds.add(key);
      bindingChildren.put(key, child);
    }
    sessionManager.getStreamFeatures(
        contentIds,
        payloads -> {
          if (contentIds.size() > payloads.size()) {
            Logger.e(
                TAG,
                "Didn't find all of the unbound content, found %s, expected %s",
                payloads.size(),
                contentIds.size());
          }
          for (PayloadWithId childPayload : payloads) {
            String key = childPayload.contentId;
            UpdatableModelChild child = bindingChildren.get(key);
            if (child != null) {
              StreamPayload payload = childPayload.payload;
              if (child.getType() == Type.UNBOUND) {
                if (payload.hasStreamFeature()) {
                  child.bindFeature(
                      new UpdatableModelFeature(payload.getStreamFeature(), cursorProvider));
                }
                if (payload.hasStreamToken()) {
                  child.bindToken(new UpdatableModelToken(payload.getStreamToken(), false));
                  continue;
                }
                child.updateFeature(childPayload.payload);
              } else {
                child.updateFeature(payload);
              }
            }
          }
          timeTracker.stop("", "bindingChildren", "childrenToBind", childrenToBind.size());
          consumer.accept(payloads.size());
        });
  }
}
