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

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;

import android.net.Uri;
import com.google.android.libraries.feed.host.network.HttpHeader.HttpHeaderName;
import com.google.android.libraries.feed.host.network.HttpRequest.HttpMethod;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Test class for {@link HttpRequest} */
@RunWith(RobolectricTestRunner.class)
public class HttpRequestTest {

  public static final String HELLO_WORLD = "hello world";

  @Test
  public void testConstructor() throws Exception {
    Uri uri = Uri.EMPTY;
    String httpMethod = HttpMethod.GET;
    HttpRequest httpRequest = new HttpRequest(uri, httpMethod);

    assertThat(httpRequest.getUri()).isEqualTo(uri);
    assertThat(httpRequest.getMethod()).isEqualTo(httpMethod);
  }

  @Test
  public void testConstructor_withHeaders() throws Exception {
    Uri uri = Uri.EMPTY;
    String httpMethod = HttpMethod.POST;
    List<HttpHeader> headers =
        Collections.singletonList(
            new HttpHeader(HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD, HELLO_WORLD));
    HttpRequest httpRequest = new HttpRequest(uri, httpMethod, headers);

    assertThat(httpRequest.getUri()).isEqualTo(uri);
    assertThat(httpRequest.getMethod()).isEqualTo(httpMethod);
    assertThat(httpRequest.getHeaders()).isEqualTo(headers);
  }

  @Test
  public void testConstructor_withBody() throws Exception {
    Uri uri = Uri.EMPTY;
    byte[] bytes = new byte[] {};
    String httpMethod = HttpMethod.POST;
    HttpRequest httpRequest = new HttpRequest(uri, httpMethod, bytes);

    assertThat(httpRequest.getUri()).isEqualTo(uri);
    assertThat(httpRequest.getBody()).isEqualTo(bytes);
    assertThat(httpRequest.getMethod()).isEqualTo(httpMethod);
  }

  @Test
  public void testConstructor_withHeadersAndBody() throws Exception {
    Uri uri = Uri.EMPTY;
    byte[] bytes = new byte[] {};
    String httpMethod = HttpMethod.POST;
    List<HttpHeader> headers =
        Collections.singletonList(
            new HttpHeader(HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD, HELLO_WORLD));
    HttpRequest httpRequest = new HttpRequest(uri, httpMethod, headers, bytes);

    assertThat(httpRequest.getUri()).isEqualTo(uri);
    assertThat(httpRequest.getBody()).isEqualTo(bytes);
    assertThat(httpRequest.getMethod()).isEqualTo(httpMethod);
    assertThat(httpRequest.getHeaders()).isEqualTo(headers);
  }

  @Test
  public void immutableHeaders() {
    Uri uri = Uri.EMPTY;
    byte[] bytes = new byte[] {};
    String httpMethod = HttpMethod.POST;
    HttpHeader header = new HttpHeader(HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD, HELLO_WORLD);
    List<HttpHeader> headers = Collections.singletonList(header);
    HttpRequest httpRequest = new HttpRequest(uri, httpMethod, headers, bytes);

    List<HttpHeader> requestHeaders = httpRequest.getHeaders();
    assertThatRunnable(() -> requestHeaders.add(new HttpHeader("test", "testValue")))
        .throwsAnExceptionOfType(UnsupportedOperationException.class);
    assertThatRunnable(() -> requestHeaders.remove(header))
        .throwsAnExceptionOfType(UnsupportedOperationException.class);
  }
}
