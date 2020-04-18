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

package com.google.android.libraries.feed.host.network;

import static com.google.common.truth.Truth.assertThat;

import com.google.android.libraries.feed.host.network.HttpHeader.HttpHeaderName;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Test class for {@link HttpHeader} */
@RunWith(RobolectricTestRunner.class)
public class HttpHeaderTest {

  private static final String HEADER_VALUE = "Hello world";

  @Test
  public void testConstructor() {
    HttpHeader header = new HttpHeader(HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD, HEADER_VALUE);

    assertThat(header.getName()).isEqualTo(HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD);
    assertThat(header.getValue()).isEqualTo(HEADER_VALUE);
  }
}
