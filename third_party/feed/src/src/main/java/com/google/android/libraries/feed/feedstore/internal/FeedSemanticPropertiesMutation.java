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

import com.google.android.libraries.feed.api.store.SemanticPropertiesMutation;
import com.google.android.libraries.feed.common.functional.Committer;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.protobuf.ByteString;
import java.util.HashMap;
import java.util.Map;

/** Implementation of the {@link SemanticPropertiesMutation}. */
public class FeedSemanticPropertiesMutation implements SemanticPropertiesMutation {
  private final Map<String, ByteString> semanticPropertiesMap = new HashMap<>();
  private final Committer<CommitResult, Map<String, ByteString>> committer;

  public FeedSemanticPropertiesMutation(
      Committer<CommitResult, Map<String, ByteString>> committer) {
    this.committer = committer;
  }

  @Override
  public SemanticPropertiesMutation add(String contentId, ByteString semanticData) {
    semanticPropertiesMap.put(contentId, semanticData);
    return this;
  }

  @Override
  public CommitResult commit() {
    return committer.commit(semanticPropertiesMap);
  }
}
