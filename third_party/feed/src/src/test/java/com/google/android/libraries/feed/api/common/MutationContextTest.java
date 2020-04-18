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

package com.google.android.libraries.feed.api.common;

import static com.google.common.truth.Truth.assertThat;

import com.google.android.libraries.feed.api.common.MutationContext.Builder;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.nio.charset.Charset;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link MutationContext} class. */
@RunWith(RobolectricTestRunner.class)
public class MutationContextTest {

  @Test
  public void testBuilder() {
    MutationContext.Builder builder = new Builder();
    ByteString tokenBytes = ByteString.copyFrom("token", Charset.defaultCharset());
    StreamToken token = StreamToken.newBuilder().setNextPageToken(tokenBytes).build();
    builder.setContinuationToken(token);
    StreamSession streamSession = StreamSession.newBuilder().setStreamToken("session:1").build();
    builder.setRequestingSession(streamSession);
    MutationContext mutationContext = builder.build();
    assertThat(mutationContext).isNotNull();
    assertThat(mutationContext.getContinuationToken()).isEqualTo(token);
    assertThat(mutationContext.getRequestingSession()).isEqualTo(streamSession);
  }
}
