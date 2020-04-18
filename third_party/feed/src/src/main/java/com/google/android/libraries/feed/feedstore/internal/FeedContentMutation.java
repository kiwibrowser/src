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

import com.google.android.libraries.feed.api.common.PayloadWithId;
import com.google.android.libraries.feed.api.store.ContentMutation;
import com.google.android.libraries.feed.common.functional.Committer;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import java.util.ArrayList;
import java.util.List;

/** This class will mutate the Content stored in the FeedStore. */
public class FeedContentMutation implements ContentMutation {

  private final List<PayloadWithId> mutations = new ArrayList<>();
  private final Committer<CommitResult, List<PayloadWithId>> committer;

  public FeedContentMutation(Committer<CommitResult, List<PayloadWithId>> committer) {
    this.committer = committer;
  }

  @Override
  public ContentMutation add(String contentId, StreamPayload payload) {
    mutations.add(new PayloadWithId(contentId, payload));
    return this;
  }

  @Override
  public CommitResult commit() {
    return committer.commit(mutations);
  }
}
