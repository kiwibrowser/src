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

/** Representation of an HTTP response. */
public final class HttpResponse {
  private final int responseCode;
  private final byte[] responseBody;

  public HttpResponse(int responseCode, byte[] responseBody) {
    this.responseCode = responseCode;
    this.responseBody = responseBody;
  }

  public HttpResponse(int responseCode) {
    this(responseCode, new byte[] {});
  }

  /**
   * Gets the response code for the response.
   *
   * <p>Note: this does not have to correspond to an HTTP response code, e.g. if there is a network
   * issue and no request was able to be sent.
   */
  public int getResponseCode() {
    return responseCode;
  }

  /** Gets the body for the response. */
  public byte[] getResponseBody() {
    return responseBody;
  }
}
