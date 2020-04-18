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

import android.support.annotation.StringDef;

public class HttpHeader {

  /** These string values correspond with the actual header name. */
  @StringDef({
    HttpHeaderName.X_PROTOBUFFER_REQUEST_PAYLOAD,
  })
  public @interface HttpHeaderName {
    String X_PROTOBUFFER_REQUEST_PAYLOAD = "X-Protobuffer-Request-Payload";
  }

  public HttpHeader(@HttpHeaderName String name, String value) {
    this.name = name;
    this.value = value;
  }

  public final @HttpHeaderName String name;
  public final String value;

  public @HttpHeaderName String getName() {
    return name;
  }

  public String getValue() {
    return value;
  }
}
