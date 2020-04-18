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

package com.google.android.libraries.feed.feedsessionmanager.internal;

import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.Collection;
import java.util.List;

/** A {@code Session} is a instance which supports update and optionally a {@link ModelProvider}. */
public interface Session {

  /** Called to update the Session, this is responsible for updating the Stored session instance. */
  void updateSession(
      List<StreamStructure> streamStructures, /*@Nullable*/ StreamToken mutationSourceToken);

  /** The unique identifier created by the storage layer for the session. */
  StreamSession getStreamSession();

  /** Returns a ModelProvider for a Session, if one is defined. */
  /*@Nullable*/
  ModelProvider getModelProvider();

  /** Cause the access time to be updated. */
  void updateAccessTime(long time);

  /** Returns an unmodifiable set of the StreamContentIds used by the session. */
  Collection<String> getContentInSession();
}
