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

import android.database.Observable;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.ArrayList;
import java.util.List;

/** Implementation of the {@link ModelToken}. */
public class UpdatableModelToken extends Observable<TokenCompletedObserver> implements ModelToken {
  private final StreamToken token;
  private final boolean isSynthetic;

  public UpdatableModelToken(StreamToken token, boolean isSynthetic) {
    this.token = token;
    this.isSynthetic = isSynthetic;
  }

  public boolean isSynthetic() {
    return isSynthetic;
  }

  @Override
  public StreamToken getStreamToken() {
    return token;
  }

  public List<TokenCompletedObserver> getObserversToNotify() {
    synchronized (mObservers) {
      return new ArrayList<>(mObservers);
    }
  }
}
