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

import com.google.android.libraries.feed.api.store.SessionMutation;
import com.google.android.libraries.feed.common.functional.Committer;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import java.util.ArrayList;
import java.util.List;

/** Implementation of the {@link SessionMutation}. */
public class FeedSessionMutation implements SessionMutation {
  private final List<StreamStructure> streamStructures = new ArrayList<>();
  private final Committer<Boolean, List<StreamStructure>> committer;

  public FeedSessionMutation(Committer<Boolean, List<StreamStructure>> committer) {
    this.committer = committer;
  }

  @Override
  public SessionMutation add(StreamStructure streamStructure) {
    streamStructures.add(streamStructure);
    return this;
  }

  @Override
  public Boolean commit() {
    return committer.commit(streamStructures);
  }
}
