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

package com.google.android.libraries.feed.feedstore.internal;

import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * Class which holds the state associated with a Session. This is a support class used by {@link
 * com.google.android.libraries.feed.feedstore.FeedStore}.
 */
public class SessionState {
  private final List<StreamStructure> streamStructures = new ArrayList<>();

  public void addAll(Collection<StreamStructure> operations) {
    streamStructures.addAll(operations);
  }

  public List<StreamStructure> getStreamStructures() {
    return streamStructures;
  }
}
