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

import android.net.Uri;
import android.support.annotation.StringDef;
import java.net.HttpURLConnection;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Representation of an HTTP request. */
public class HttpRequest {

  /**
   * These string values line up with HTTP method values, see {@link
   * HttpURLConnection#setRequestMethod(String)}.
   */
  @StringDef({
    HttpMethod.GET,
    HttpMethod.POST,
    HttpMethod.PUT,
    HttpMethod.DELETE,
  })
  public @interface HttpMethod {
    String GET = "GET";
    String POST = "POST";
    String PUT = "PUT";
    String DELETE = "DELETE";
  }

  private final Uri uri;
  private final byte[] body;
  private final @HttpMethod String method;
  private final List<HttpHeader> headers;

  public HttpRequest(Uri uri, @HttpMethod String method, List<HttpHeader> headers, byte[] body) {
    this.uri = uri;
    this.body = body;
    this.method = method;
    this.headers = Collections.unmodifiableList(new ArrayList<>(headers));
  }

  public HttpRequest(Uri uri, @HttpMethod String method, List<HttpHeader> headers) {
    this(uri, method, headers, new byte[] {});
  }

  public HttpRequest(Uri uri, @HttpMethod String method, byte[] body) {
    this(uri, method, Collections.emptyList(), body);
  }

  public HttpRequest(Uri uri, @HttpMethod String method) {
    this(uri, method, Collections.emptyList(), new byte[] {});
  }

  public Uri getUri() {
    return uri;
  }

  public byte[] getBody() {
    return body;
  }

  public @HttpMethod String getMethod() {
    return method;
  }

  public List<HttpHeader> getHeaders() {
    return headers;
  }
}
